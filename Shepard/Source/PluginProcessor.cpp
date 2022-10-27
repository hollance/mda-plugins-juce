#include "PluginProcessor.h"

MDAShepardAudioProcessor::MDAShepardAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    const float twopi = 6.2831853f;
    const int max = bufferSize - 1;

    // Generate wavetables. We only need to do this once.
    for (int j = 0; j < max; ++j) {
        float pos = twopi * float(j) / float(max);

        // buf2 contains one period of a sine wave.
        _buf2[j] = std::sin(pos);

        // buf1 also contains higher octaves of this sine wave.
        float x = 0.0f;
        float a = 1.0f;
        for (int i = 0; i < 8; ++i) {
            x += a * std::sin(std::fmod(pos, twopi));
            a *= 0.5f;
            pos *= 2.0f;
        }
        _buf1[j] = x;
    }

    // Make last value the same as the first, for easier interpolation.
    _buf1[max] = 0.0f;
    _buf2[max] = 0.0f;
}

MDAShepardAudioProcessor::~MDAShepardAudioProcessor()
{
}

const juce::String MDAShepardAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDAShepardAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDAShepardAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDAShepardAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDAShepardAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDAShepardAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAShepardAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDAShepardAudioProcessor::releaseResources()
{
}

void MDAShepardAudioProcessor::reset()
{
    resetState();
}

bool MDAShepardAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAShepardAudioProcessor::resetState()
{
    _pos = 0.0f;
    _rate = 1.0f;
}

void MDAShepardAudioProcessor::update()
{
    _mode = int(apvts.getRawParameterValue("Mode")->load());

    // The rate is a percentage from -100% to +100%. Convert this into a curve
    // that is 1.0 at 0% and goes gently downward to the left (to make a falling
    // sound) and upward to the right (for a rising sound).
    float fParam1 = int(apvts.getRawParameterValue("Rate")->load());
    fParam1 = (fParam1 + 100) / 200.0f;
    _delta = 1.0f + 10.0f * std::pow(fParam1 - 0.5f, 3.0f) / float(getSampleRate());

    // Convert the output level from decibels to a linear gain.
    float fParam2 = int(apvts.getRawParameterValue("Output")->load());
    _level = 0.4842f * juce::Decibels::decibelsToGain(fParam2);
}

void MDAShepardAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    const float *buf1 = _buf1;
    const float *buf2 = _buf2;
    const float max = float(bufferSize - 1);
    const int mode = _mode;
    const float delta = _delta;
    const float level = _level;

    float rate = _rate;
    float pos = _pos;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        // The illusion is a sound that appears to be rising (or falling) in pitch
        // forever. We make the pitch go up by stepping through the wavetable at an
        // increasing rate. The rate goes up from 1.0 to 2.0 and then wraps back to
        // 1.0. That wrap-around by itself would give a jarring effect. To hide
        // this, we don't just play a plain sine wave (buf2) but also 1, 2, 4, 8
        // octaves higher (buf1). As the pitch goes up, the higher notes fade out
        // while lower notes fade in. In the case of a falling tone, we do it the
        // other way around (the pitch goes down because the wavetable read pos is
        // incremented using a rate that becomes gradually smaller).

        rate *= delta;
        if (rate > 2.0f) {  // rising tone
            rate *= 0.5f;
            pos *= 0.5f;
        } else if (rate < 1.0f) {  // falling tone
            rate *= 2.0f;
            pos *= 2.0f;
            if (pos >= max) pos -= max;
        }

        // Increment the read position in the wavetables and wrap around when we
        // get to the end.
        pos += rate;
        if (pos > max) pos -= max;

        // Calculate position interpolation:
        int i1 = int(pos);           // current sample
        int i2 = i1 + 1;             // next sample
        float di = float(i2) - pos;  // fractional part between the two

        // di and (1 - di) are used for interpolating between two successive
        // positions in the wavetable.
        // buf1 contains the sine waves in the higher octaves, with each higher
        // octave having half the amplitude as the previous octave.
        // buf2 contains a plain sine wave. We multiply this by (rate - 2) in
        // order to fade it in or out as the pitch changes.
        float b =           di  * (buf1[i1] + (rate - 2.0f) * buf2[i1])
                  + (1.0f - di) * (buf1[i2] + (rate - 2.0f) * buf2[i2]);

        // Dividing by the rate means we fade out the sound as the pitch goes
        // up, or fade it in when the pitch goes down.
        b *= level / rate;

        // Do we need to combine the tone with incoming audio?
        if (mode > 0) {
            float a = in1[i] + in2[i];  // stereo to mono
            if (mode == 1) {
                b *= a;                 // ring modulation with input
            } else if (mode == 2) {
                b += 0.5f * a;          // mix with input (at -6 dB)
            }
        }

        out1[i] = b;
        out2[i] = b;
    }

    _pos = pos;
    _rate = rate;
}

juce::AudioProcessorEditor *MDAShepardAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDAShepardAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAShepardAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAShepardAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::StringArray({ "TONES", "RING MOD", "TONES+IN" }),
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Rate", 1),
        "Rate",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDAShepardAudioProcessor();
}
