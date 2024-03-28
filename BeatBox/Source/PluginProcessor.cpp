#include "PluginProcessor.h"

MDABeatBoxAudioProcessor::MDABeatBoxAudioProcessor()
    : AudioProcessor(BusesProperties()
                .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    sampleRate = 44100.0f;
}

MDABeatBoxAudioProcessor::~MDABeatBoxAudioProcessor()
{
}

const juce::String MDABeatBoxAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDABeatBoxAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDABeatBoxAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDABeatBoxAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDABeatBoxAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDABeatBoxAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDABeatBoxAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = float(newSampleRate);

    hbuflen = 20000;
    kbuflen = 20000;
    sbuflen = 60000;

    // See the note in synth() on higher sampling rates.
    if (sampleRate > 49000.0f) {
        hbuflen *= 2;
        kbuflen *= 2;
        sbuflen *= 2;
    }

    hbuf.reset(new float[hbuflen]);
    kbuf.reset(new float[kbuflen]);
    sbufL.reset(new float[sbuflen]);
    sbufR.reset(new float[sbuflen]);

    synth();

    // These variables store after how many samples the kick and snare
    // are allowed to repeat. For the hi-hat this is a parameter.
    kdel = int(0.10f * sampleRate);
    sdel = int(0.12f * sampleRate);

    // Fixed attack and release times for envelope follower.
    dyna = std::pow(10.0f, -1000.0f / sampleRate);
    dynr = std::pow(10.0f, -6.0f / sampleRate);
    dyne = 0.0f;

    resetState();
}

void MDABeatBoxAudioProcessor::releaseResources()
{
}

void MDABeatBoxAudioProcessor::reset()
{
    resetState();
}

bool MDABeatBoxAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDABeatBoxAudioProcessor::synth()
{
    float o = 0.0f;
    float p = 0.2f;

    // Generate hi-hat. This is a burst of noise with an exponentially decaying
    // envelope, and some basic filtering applied. This filter cuts around 5k
    // and boosts around 10k to make it brighter.
    // Note that std::rand() is bad and should be avoided in modern C++ code.
    // The original plug-in never seeded the random number generator, so the
    // hi-hat sounds different every time you run the plug-in. For even more
    // varied results, a new sound could be synthesized on-the-fly every time
    // the hi-hat is triggered.
    {
        std::memset(hbuf.get(), 0, hbuflen * sizeof(float));
        float e = 0.00012f;
        float de = std::pow(10.0f, -36.0f/sampleRate);
        float o1 = 0.0f;
        float o2 = 0.0f;
        for (int t = 0; t < 5000; ++t) {
            o = float((std::rand() % 2000) - 1000);
            hbuf[t] = e * (2.0f*o1 - o2 - o);
            e *= de;
            o2 = o1;
            o1 = o;
        }
    }

    // Generate kick sample. This is a sine wave that decays exponentially
    // in amplitude as well as in frequency.
    {
        std::memset(kbuf.get(), 0, kbuflen * sizeof(float));
        float e = 0.5f;
        float de = std::pow(10.0f, -3.8f/sampleRate);
        float dp = 1588.0f / sampleRate;
        for (int t = 0; t < 14000; ++t) {
            kbuf[t] = e * std::sin(p);
            e *= de;
            p = std::fmod(p + dp * e, 6.2831853f);
        }
    }

    // Generate snare. This is a sine wave with an exponentially decaying
    // envelope and a small amount of added (filtered) noise. The snare
    // buffer is stereo, although that feature was only used for recording
    // your own sounds, which I did not include in the JUCE version.
    {
        std::memset(sbufL.get(), 0, sbuflen * sizeof(float));
        std::memset(sbufR.get(), 0, sbuflen * sizeof(float));
        float e = 0.38f;
        float de = std::pow(10.0f, -15.0f/sampleRate);
        for (int t = 0; t < 7000; ++t) {
            o = 0.3f * o + float((std::rand() % 2000) - 1000);
            sbufL[t] = e * (std::sin(p) + 0.0004f * o);
            sbufR[t] = sbufL[t];
            e *= de;
            p = std::fmod(p + 0.025f, 6.2831853f);
        }
    }

    // Note that the synthesized sounds are not independent of the sample
    // rate! The higher the sample rate, the higher the pitch. Additionally,
    // the durations are wrong for higher sample rates. Also not sure why
    // the snare code uses the `p` and `o` variables from the kick.
}

void MDABeatBoxAudioProcessor::resetState()
{
    hbufpos = hbuflen - 1;
    kbufpos = kbuflen - 1;
    sbufpos = sbuflen - 1;

    hfil = 0.0f;

    kww = 0.0f;
    ksfx = 0;
    ksb1 = 0.0f;
    ksb2 = 0.0f;

    ww = 0.0f;
    sfx = 0;
    sb1 = 0.0f;
    sb2 = 0.0f;
}

void MDABeatBoxAudioProcessor::update()
{
    // Convert from decibels (-40 dB ... 0 dB) to a linear value.
    float param1 = apvts.getRawParameterValue("Hat Thr")->load();
    hthr = std::pow(10.0f, 2.0f * param1 - 2.0f);

    // After how many samples the hi-hat is allowed to repeat.
    float param2 = apvts.getRawParameterValue("Hat Rate")->load();
    hdel = int((0.04f + 0.2f * param2) * sampleRate);

    // Convert from decibels (-80 dB ... +12 dB) to a linear value.
    float param3 = apvts.getRawParameterValue("Hat Mix")->load();
    hlev = 0.0001f + param3 * param3 * 4.0f;

    // Convert from decibels (-40 dB ... 0 dB) to a linear value.
    float param4 = apvts.getRawParameterValue("Kik Thr")->load();
    kthr = 220.0f * std::pow(10.0f, 2.0f * param4 - 2.0f);

    // Keep track of the old values so we can see if they changed.
    float kwwx = kww;
    float wwx = ww;

    // The frequency for the kick filter.
    float param5 = apvts.getRawParameterValue("Kik Trig")->load();
    kww = std::pow(10.0f, -3.0f + 2.2f * param5);
    ksf1 = std::cos(3.1415927f * kww);  // p
    ksf2 = std::sin(3.1415927f * kww);  // q

    // Convert from decibels (-80 dB ... +12 dB) to a linear value.
    float param6 = apvts.getRawParameterValue("Kik Mix")->load();
    klev = 0.0001f + param6 * param6 * 4.0f;

    // Convert from decibels (-40 dB ... 0 dB) to a linear value.
    float param7 = apvts.getRawParameterValue("Snr Thr")->load();
    sthr = 40.0f * std::pow(10.0f, 2.0f * param7 - 2.0f);

    // The frequency for the snare filter.
    float param8 = apvts.getRawParameterValue("Snr Trig")->load();
    ww = std::pow(10.0f, -3.0f + 2.2f * param8);
    sf1 = std::cos(3.1415927f * ww);  // p
    sf2 = std::sin(3.1415927f * ww);  // q
    sf3 = 0.991f;  // r

    // The plug-in switches to "key listen" mode while the Kik Trig
    // or Snr Trig parameter is being adjusted. This sets the counter
    // to two seconds.
    if (kwwx != kww) {
        ksfx = int(2.0f * sampleRate);
    }
    if (wwx != ww) {
        sfx = int(2.0f * sampleRate);
    }

    // Convert from decibels (-80 dB ... +12 dB) to a linear value.
    float param9 = apvts.getRawParameterValue("Snr Mix")->load();
    slev = 0.0001f + param9 * param9 * 4.0f;

    // This parameter is a percentage.
    dynm = apvts.getRawParameterValue("Dynamics")->load();

    // This parameter is displayed as -inf dB ... 0 dB but is already
    // 0 - 1, so we don't need to convert anything.
    mix = apvts.getRawParameterValue("Thru Mix")->load();
}

void MDABeatBoxAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    int hbufmax = hbuflen - 1;
    int kbufmax = kbuflen - 1;
    int sbufmax = sbuflen - 1;

    float mix3 = 0.0f;

    // Key listen (snare). This turns off everything except the snare filter
    // output. This continues until two seconds worth of samples have elapsed.
    if (sfx > 0) {
        mix3 = 0.08f;
        slev = 0.0f;
        klev = 0.0f;
        hlev = 0.0f;
        mix = 0.0f;
        sfx -= buffer.getNumSamples();
    }

    // Key listen (kick). This also uses the snare filter but swaps the coeffs
    // to those from the kick filter.
    if (ksfx > 0) {
        mix3 = 0.03f;
        slev = 0.0f;
        klev = 0.0f;
        hlev = 0.0f;
        mix = 0.0f;
        ksfx -= buffer.getNumSamples();
        sf1 = ksf1;
        sf2 = ksf2;
    }

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        // Convert the stereo signal to mono. (This is 6 dB too loud.)
        float e = a + b;

        // Envelope follower. If the new sample is lower than the envelope,
        // apply release, otherwise apply attack (using a one-pole filter).
        // Note that in a regular envelope follower the `e` value would be
        // absolute but here it can be negative as well. Not sure if that
        // was an error in the original code or intentional.
        dyne = (e < dyne) ? dyne * dynr : e - dyna * (e - dyne);

        // Basic first order high-pass filter with a cutoff around 10 kHz.
        // This computes the differences between two successive samples.
        // This difference is close to zero at low frequencies but large
        // at high frequencies.
        hfil = e - hfil;

        // Start playing the hi-hat sample when the filter output exceeds
        // the threshold and we've waited long enough since the last hi-hat.
        if ((hbufpos > hdel) && (hfil > hthr)) {
            hbufpos = 0;
        } else if (hbufpos < hbufmax) {  // play until end and hold there
            hbufpos++;
        }
        float o = hlev * hbuf[hbufpos];

        // Low filter. This is a low-pass that gradually turns into a band
        // pass. It has a massive gain for some reason... Unlike the hi-hat
        // filter, which is fixed, the kick and snare filters can be changed.
        float kfil = e + ksb1*ksf1 - ksb2*ksf2;
        ksb2 = sf3 * (ksb1*ksf2 + ksb2*ksf1);
        ksb1 = sf3 * kfil;

        // Start playing the kick sample when the filter output exceeds
        // the threshold and we've waited long enough since the last kick.
        // Note that a short `kdel` time can cause the existing kick sample
        // to be cut off suddenly when the new kick starts, which may glitch
        // a little. It would be better to quickly fade out the old kick.
        if ((kbufpos > kdel) && (kfil > kthr)) {
            kbufpos = 0;
        } else if (kbufpos < kbufmax) {
            kbufpos++;
        }
        o += klev * kbuf[kbufpos];

        // Mid filter. Similar to the kick filter; same coeffs but slightly
        // different formula.
        float sfil = hfil + 0.3f*e + sb1*sf1 - sb2*sf2;
        sb2 = sf3 * (sb1*sf2 + sb2*sf1);
        sb1 = sf3 * sfil;

        // Play the snare sample.
        if ((sbufpos > sdel) && (sfil > sthr)) {
            sbufpos = 0;
        } else if (sbufpos < sbufmax) {
            sbufpos++;
        }
        float c = o + slev*sbufL[sbufpos];
        float d = o + slev*sbufR[sbufpos];

        // Dynamics. This applies the envelope of the original sound
        // to the synthesized samples. It does make things a lot louder!
        float mix4 = 1.0f + dynm * (dyne + dyne - 1.0f);

        // The final output mixes the original input with the synthesized
        // signal. In "key listen" mode, we mix in the snare / kick filter
        // output, and everything else is turned off.
        out1[i] = mix*a + mix3*sfil + mix4*c;
        out2[i] = mix*b + mix3*sfil + mix4*d;

        hfil = e;
    }
}

juce::AudioProcessorEditor *MDABeatBoxAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDABeatBoxAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDABeatBoxAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDABeatBoxAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -40 dB ... 0 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Hat Thr", 1),
        "Hat Thr",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int) {
                return juce::String(40.0f*value - 40.0f, 2);
            })
        ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Hat Rate", 1),
        "Hat Rate",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.45f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(1000.0f * (0.04f + 0.2f * value)));
            })
        ));

    // -80 dB ... +12 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Hat Mix", 1),
        "Hat Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                float hlev = 0.0001f + value * value * 4.0f;
                return juce::String(int(20.0f * std::log10(hlev)));
            })
        ));

    // -40 dB ... 0 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Kik Thr", 1),
        "Kik Thr",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.46f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(40.0f*value - 40.0f, 2);
            })
        ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Kik Trig", 1),
        "Kik Trig",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.15f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([this](float value, int)
            {
                // Take this cutoff frequency with a grain of salt!
                float kww = std::pow(10.0f, -3.0f + 2.2f * value);
                return juce::String(int(0.5f * kww * sampleRate));
            })
        ));

    // -80 dB ... +12 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Kik Mix", 1),
        "Kik Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                float klev = 0.0001f + value * value * 4.0f;
                return juce::String(int(20.0f * std::log10(klev)));
            })
        ));

    // -40 dB ... 0 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Snr Thr", 1),
        "Snr Thr",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int) {
                return juce::String(40.0f*value - 40.0f, 2);
            })
        ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Snr Trig", 1),
        "Snr Trig",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([this](float value, int)
            {
                // Take this cutoff frequency with a grain of salt!
                float ww = std::pow(10.0f, -3.0f + 2.2f * value);
                return juce::String(int(0.5f * ww * sampleRate));
            })
        ));

    // -80 dB ... +12 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Snr Mix", 1),
        "Snr Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                float slev = 0.0001f + value * value * 4.0f;
                return juce::String(int(20.0f * std::log10(slev)));
            })
        ));

    // 0% ... 100%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Dynamics", 1),
        "Dynamics",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(100.0f * value));
            })
        ));

    // -inf dB ... 0 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Thru Mix", 1),
        "Thru Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(20.0f * std::log10(value), 2);
            })
        ));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDABeatBoxAudioProcessor();
}
