#include "PluginProcessor.h"

MDADetuneAudioProcessor::MDADetuneAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDADetuneAudioProcessor::~MDADetuneAudioProcessor()
{
}

const juce::String MDADetuneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDADetuneAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDADetuneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDADetuneAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDADetuneAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDADetuneAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDADetuneAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = float(newSampleRate);
    resetState();
}

void MDADetuneAudioProcessor::releaseResources()
{
}

void MDADetuneAudioProcessor::reset()
{
    resetState();
}

bool MDADetuneAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDADetuneAudioProcessor::resetState()
{
    std::memset(buf, 0, sizeof(buf));
    std::memset(win, 0, sizeof(win));
    pos0 = 0;
    pos1 = pos2 = 0.0f;
}

void MDADetuneAudioProcessor::update()
{
    // Number of semitones expressed as a value between 0 and 300 cents.
    // This is skewed using a x^3 curve, putting ~38 cents at the middle
    // of the slider and 300 cents at the rightmost position.
    float param0 = apvts.getRawParameterValue("Detune")->load();
    float semi = 3.0f * param0 * param0 * param0;

    // 1.0594631^semi is the same as 2^(semi/12) and gives the step size
    // used to pitch the sound down by this number of semitones.
    dpos2 = std::pow(1.0594631f, semi);

    // This is the same as 2^(-semi/12) and is the delta used to pitch up
    // the sound by the given number of semitones.
    dpos1 = 1.0f / dpos2;

    // Output gain is -20 to +20 dB. Convert decibels to linear.
    float param2 = apvts.getRawParameterValue("Output")->load();
    float gain = juce::Decibels::decibelsToGain(param2);

    // Dry/wet curve of (1 - x^2) for dry and (2x - x^2) for wet, with the
    // output gain amount already multiplied into it.
    float param1 = apvts.getRawParameterValue("Mix")->load();
    dry = gain - gain * param1 * param1;
    wet = (gain + gain - gain * param1) * param1;

    // The latency parameter determines the length of the delay line.
    // Since this parameter is a value between 0.0f and 1.0f, the expression
    // (8 + int(4.9f * param)) produces a discrete set of values: 8, 9, 10, 11,
    // and 12. The bit shift converts this into buffer lengths of respectively
    // 256, 512, 1024, 2048, and 4096 samples. Note that `buflen` should always
    // be a power of two.
    float param3 = apvts.getRawParameterValue("Latency")->load();
    int tmp = 1 << (8 + int(4.9f * param3));

    // Recalculate the crossfade window (hanning half-overlap-and-add).
    if (tmp != buflen) {
        buflen = tmp;
        if (buflen > BUFMAX) { buflen = BUFMAX; }

        double phase = 0.0;
        double step = 6.28318530718 / buflen;
        for (int i = 0; i < buflen; i++) {
            win[i] = float(0.5 - 0.5 * std::cos(phase));
            phase += step;
        }
    }
}

void MDADetuneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    update();

    const float *in1 = buffer.getReadPointer(0);
    const float *in2 = buffer.getReadPointer(1);
    float *out1 = buffer.getWritePointer(0);
    float *out2 = buffer.getWritePointer(1);

    /*
        How this works:

        If you have a delay line where you keep steadily increasing the delay
        length on every timestep, this will pitch down the sound because it
        slows down the reading of the waveform. Conversely, if you decrease
        the delay length by the same amount on every timestep, the sound gets
        pitched up (it skips parts of the waveform).

        There is a practical problem with this approach: It's not possible to
        have a delay length smaller than zero and so we can't keep decreasing
        it forever. Likewise, we can't keep increasing the delay length either
        as that requires an infinitely long delay line.

        A compromise is to use a short delay length and simply reset the read
        pointer when it catches up with the write head. The Latency parameter
        determines the length of the delay, with a minimum of 256 samples and
        a maximum of 4096 samples. (A shorter delay has less latency but sounds
        worse.)

        However, resetting the read pointer will produce a nasty discontinuity.
        The solution is to read from two places at once and crossfade between
        them. The crossfading is done using a Hann window, which tapers off at
        the edges, so that the discontinuity is suppressed.
     */

    // For wrapping around the write pointer in the delay line.
    // This only works correctly if `buflen` is a power of two!
    const int mask = buflen - 1;

    // For wrapping around the read pointers, which are floats.
    const float wrapAround = float(buflen);

    // We'll read the second sample half the delay length ahead.
    const int halfLength = buflen >> 1;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        // Read the input samples.
        float a = in1[i];
        float b = in2[i];

        // Put the dry signal into the output variables already.
        float c = dry * a;
        float d = dry * b;

        // Update the write position. For some reason this plug-in counts
        // backwards, but that shouldn't matter. The wrap-around is handled
        // by bitwise-and with the mask.
        --pos0 &= mask;

        // Write the input as a mono signal into the delay line. This already
        // applies the wet gain, so we don't have to do this later.
        buf[pos0] = wet * (a + b);

        // Update the read position for the left channel, wrapping around
        // if necessary. Note that this is a float because `dpos1` is the
        // speed at which to step through the delay line, which will be a
        // fractional number of samples. (In fact, dpos1 will be less than
        // 1.0 so that this read index moves slower than the write index,
        // which results in the sound being pitched down.)
        pos1 -= dpos1;
        if (pos1 < 0.0f) { pos1 += wrapAround; }

        // Read the delay line using linear interpolation. This involves
        // taking a weighted average between the sample ahead and behind.
        int p1i = int(pos1);            // integer position
        float p1f = pos1 - float(p1i);  // fraction
        float u = buf[p1i];             // read first sample
        ++p1i &= mask;                  // move to next sample, maybe wrap
        u += p1f * (buf[p1i] - u);      // read next sample and blend

        // Also read the sample half the delay length away (i.e. offset by
        // 180 degrees). This also uses linear interpolation, same fraction.
        int p2i = (p1i + halfLength) & mask;
        float v = buf[p2i];
        ++p2i &= mask;
        v += p1f * (buf[p2i] - v);

        // Crossfade between the two interpolated samples, and add to the
        // left channel output. The `u` sample is multiplied by the window,
        // and the `v` sample by (1 - window), so this is another linear
        // interpolation. The window index is chosen so that the closer the
        // read pointer gets to the write pointer, the less the `u` sample
        // and the more `v` sample will contribute, and vice versa.
        int xi = (p1i - pos0) & mask;
        c += v + win[xi] * (u - v);

        // Apply the same logic to the right channel. This will pitch up the
        // sound because `dpos2` is larger than 1.0, so that this read index
        // will move faster than the write index and shorten the waveform.
        pos2 -= dpos2;
        if (pos2 < 0.0f) { pos2 += wrapAround; }

        // First sample.
        p1i = int(pos2);
        p1f = pos2 - float(p1i);
        u = buf[p1i];
        ++p1i &= mask;
        u += p1f * (buf[p1i] - u);

        // Second sample offset by half delay length.
        p2i = (p1i + halfLength) & mask;
        v = buf[p2i];
        ++p2i &= mask;
        v += p1f * (buf[p2i] - v);

        // Crossfade.
        p2i = (p1i - pos0) & mask;
        d += v + win[p2i] * (u - v);

        // Write the output samples.
        out1[i] = c;
        out2[i] = d;
    }
}

juce::AudioProcessorEditor *MDADetuneAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDADetuneAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDADetuneAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDADetuneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Detune", 1),
        "Detune",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.2f,
        juce::AudioParameterFloatAttributes()
            .withLabel("cents")
            .withStringFromValueFunction([](float value, int)
            {
                float semi = 3.0f * value * value * value;
                return juce::String(100.0f * semi, 1);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.9f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(100.0f * value));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Latency", 1),
        "Latency",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([this](float value, int)
            {
                // Calculate the length of the delay in samples, then convert
                // this to a time in milliseconds for displaying to the user.
                int buflen = 1 << (8 + int(4.9f * value));
                if (buflen > BUFMAX) { buflen = BUFMAX; }
                float bufres = 1000.0f * float(buflen) / sampleRate;
                return juce::String(bufres, 1);
            })));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDADetuneAudioProcessor();
}
