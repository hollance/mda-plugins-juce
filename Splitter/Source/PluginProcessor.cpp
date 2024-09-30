#include "PluginProcessor.h"

MDASplitterAudioProcessor::MDASplitterAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDASplitterAudioProcessor::~MDASplitterAudioProcessor()
{
}

const juce::String MDASplitterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDASplitterAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDASplitterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDASplitterAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDASplitterAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDASplitterAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDASplitterAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    resetState();
}

void MDASplitterAudioProcessor::releaseResources()
{
}

void MDASplitterAudioProcessor::reset()
{
    resetState();
}

bool MDASplitterAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDASplitterAudioProcessor::resetState()
{
    env = a0 = a1 = b0 = b1 = 0.0f;
}

void MDASplitterAudioProcessor::update()
{
    // The frequency goes from 100 Hz to 10 kHz.
    // The `freq` variable is the coefficient for a filter.
    float param1 = apvts.getRawParameterValue("Freq")->load();
    float fdisp = std::pow(10.0f, 2.0f + 2.0f * param1);
    freq = 5.5f * fdisp / getSampleRate();
    if (freq > 1.0f) { freq = 1.0f; }

    // Frequency switching. This determines whether the filter is low-pass
    // (BELOW), high-pass (ABOVE), or disabled (ALL).
    float param2 = int(apvts.getRawParameterValue("Freq SW")->load());
    ff = -1.0f;                          // above
    if (param2 == 0) { ff = 0.0f; }      // below
    if (param2 == 1) { freq = 0.001f; }  // all

    // The level threshold goes from -40 dB to 0 dB. Convert to linear gain.
    // The + 0.3 adds 6 dB, so if the parameter is set to 0 dB, the `level`
    // variable is actually 1.9953. This is done because computing the signal
    // amplitude adds up the left and right channels, which can boost the
    // combined amplitude by 2x or 6 dB.
    float param3 = apvts.getRawParameterValue("Level")->load();
    float ldisp = 40.0f * param3 - 40.0f;
    level = std::pow(10.0f, 0.05f * ldisp + 0.3f);

    // Level switching. This determines whether the gate lets through low
    // levels only (BELOW), high levels only (ABOVE), or everything (ALL).
    float param4 = int(apvts.getRawParameterValue("Level SW")->load());
    ll = 0.0f;                          // above
    if (param4 == 0) { ll = -1.0f; }    // below
    if (param4 == 1) { level = 0.0f; }  // all

    // Phase (polarity) correction. This is used with the envelope follower
    // and depends on the frequency switching and level switching modes.
    // This is just a helper variable that avoids branching.
    pp = -1.0f;
    if (ff == ll) { pp = 1.0f; }
    if (ff == 0.0f && ll == -1.0f) { ll *= -1.0f; }

    // The envelope goes from 10 ms to 1000 ms. This sets an approximate
    // attack and release time.
    float param5 = int(apvts.getRawParameterValue("Envelope")->load());
    att = 0.05f - 0.05f * param5;
    rel = 1.0f - std::exp(-6.0f - 4.0f * param5);
    if (att > 0.02f) { att = 0.02f; }
    if (rel < 0.9995f) { rel = 0.9995f; }

    // The output level goes from -20 dB to 20 dB. Convert to linear gain.
    float param6 = apvts.getRawParameterValue("Output")->load();
    i2l = i2r = o2l = o2r = std::pow(10.0f, 2.0f * param6 - 1.0f);

    // Output routing. This changes the gain levels for the dry (`i2l`, `i2r`)
    // and wet (`o2l`, `o2r`) signals for the left and right channels.
    int mode = int(apvts.getRawParameterValue("Mode")->load());
    switch (mode) {
        case  0: i2l  =  0.0f;  i2r  =  0.0f; break;  // NORMAL
        case  1: o2l *= -1.0f;  o2r *= -1.0f; break;  // INVERSE
        case  2: i2l  =  0.0f;  o2r *= -1.0f; break;  // NORM INV
        default: o2l *= -1.0f;  i2r  =  0.0f; break;  // INV NORM
    }
}

void MDASplitterAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        // Frequency split. This applies a basic low-pass filter. Analyzing
        // it in PluginDoctor shows this has 12 dB/octave slope and a bit of
        // a resonant bump. Note that that filter coefficient `freq` only
        // roughly corresponds to the Hz from the parameter.
        a0 += freq * (a - a0 - a1);
        a1 += freq * a0;

        // If the setting is BELOW, `ff` is 0.0f. If the setting is ABOVE,
        // `ff` is -1.0f, which subtracts the original signal from the filtered
        // signal to create a high-pass filter instead. The phase response for
        // the high pass filter is a little strange because subtracting the
        // original input flips its polarity.
        // If the setting is ALL, the filter coefficient is fixed to a small
        // value giving the high-pass filter's a cutoff of 10 Hz, effectively
        // disabling the filter.
        float aa = a1 + ff * a;

        // Also frequency split the other channel.
        b0 += freq * (b - b0 - b1);
        b1 += freq * b0;
        float bb = b1 + ff * b;

        // Combine the two channels into a single amplitude value
        // and make it positive.
        float ee = aa + bb;
        if (ee < 0.0f) { ee = -ee; }

        // Level split. This is an envelope follower that is calculated on the
        // filtered sound. If the new amplitude exceeds the previous envelope
        // level, use the attack. Also always apply decay.
        if (ee > level) {
            env += att * (pp - env);
        }
        env *= rel;

        // The `pp` variable is the target for the envelope follower when in
        // attack mode. It's either -1.0 or 1.0, according to this table:
        //
        //    Freq mode | Level mode |  ff  |  ll  |  pp
        //    --------- | ---------- | ---- | ---- | ----
        //    ABOVE     | ABOVE      | -1.0 |  0.0 | -1.0
        //              | BELOW      | -1.0 | -1.0 |  1.0
        //              | ALL        | -1.0 |  0.0 | -1.0
        //    BELOW     | ABOVE      |  0.0 |  0.0 |  1.0
        //              | BELOW      |  0.0 |  1.0 | -1.0
        //              | ALL        |  0.0 |  0.0 |  1.0
        //    ALL       | ABOVE      | -1.0 |  0.0 | -1.0
        //              | BELOW      | -1.0 | -1.0 |  1.0
        //              | ALL        | -1.0 |  0.0 | -1.0
        //
        // The `ll` variable determines whether we select low levels (below the
        // `level` threshold) or high levels (above the `level` threshold).
        //
        // The envelope follower will create a positive envelope (between 0.0
        // and 1.0) for some combinations of the freq mode and level mode, and
        // a negative envelope (between -1.0 and 0.0) for other combinations.
        // In level mode ALL, `level` is 0.0 so that the amplitude always exceeds
        // it, making the envelope a flat line.
        //
        // The reason for using these `ff`, `ll`, `pp` helper variables is so
        // that we can support the BELOW & ABOVE modes without having to branch.

        // Calculate the final sample values. This first applies the envelope
        // to the filtered (wet) signal and then mixes it with the dry signal.
        // The dry/wet mix is determined by the mode:
        //  - NORMAL: wet only
        //  - INVERSE: wet is subtracted from dry
        //  - NORM INV: left channel wet only, right wet subtracted from dry
        //  - INV NORM: left wet subtracted from dry, right channel wet only
        //
        // Note that using freq ABOVE in NORMAL mode is the same as freq BELOW
        // in INVERSE mode, and vice versa (except maybe a polarity flip). Same
        // for the level mode.

        a = i2l * a + o2l * aa * (env + ll);
        b = i2r * b + o2r * bb * (env + ll);

        out1[i] = a;
        out2[i] = b;
    }
}

juce::AudioProcessorEditor *MDASplitterAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDASplitterAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDASplitterAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDASplitterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::StringArray { "NORMAL", "INVERSE", "NORM/INV", "INV/NORM" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Freq", 1),
        "Freq",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([this](float value, int)
            {
                float fdisp = std::pow(10.0f, 2.0f + 2.0f * value);
                return juce::String(int(fdisp));
            })));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Freq SW", 1),
        "Freq SW",
        juce::StringArray { "BELOW", "ALL", "ABOVE" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Level", 1),
        "Level",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(40.0f * value - 40.0f));
            })));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Level SW", 1),
        "Level SW",
        juce::StringArray { "BELOW", "ALL", "ABOVE" },
        1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Envelope", 1),
        "Envelope",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(std::pow(10.0f, 1.0f + 2.0f * value)));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(40.0f * value - 20.0f, 1);
            })));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDASplitterAudioProcessor();
}
