#include "PluginProcessor.h"

MDAEnvelopeAudioProcessor::MDAEnvelopeAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDAEnvelopeAudioProcessor::~MDAEnvelopeAudioProcessor()
{
}

const juce::String MDAEnvelopeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDAEnvelopeAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDAEnvelopeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDAEnvelopeAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDAEnvelopeAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDAEnvelopeAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAEnvelopeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDAEnvelopeAudioProcessor::releaseResources()
{
}

void MDAEnvelopeAudioProcessor::reset()
{
    resetState();
}

bool MDAEnvelopeAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAEnvelopeAudioProcessor::resetState()
{
    env = 0.0f;
    releaseRate = 0.0f;
}

void MDAEnvelopeAudioProcessor::update()
{
    mode = apvts.getRawParameterValue("Output")->load();

    // Convert attack time to filter coefficient.
    float fParam2 = apvts.getRawParameterValue("Attack")->load();
    attack = std::pow(10.0f, -0.002f - 4.0f * fParam2);

    // Convert release time to filter coefficient.
    float fParam3 = apvts.getRawParameterValue("Release")->load();
    release = 1.0f - std::pow(10.0f, -2.0f - 3.0f * fParam3);

    // Convert decibels to linear gain.
    float fParam4 = apvts.getRawParameterValue("Gain")->load();
    gain = std::pow(10.f, 0.05f * fParam4);

    // Constant gain across modes.
    if (mode == 0) {
        gain *= 2.0f;
    } else {
        gain *= 0.5f;
    }
}

void MDAEnvelopeAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    if (releaseRate > 1.0f - release) {  // can be unstable if release changed
        releaseRate = 1.0f - release;
    }

    bool flat = (mode == 2);  // flatten output audio?

    if (mode > 0) {  // envelope follower mode
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Combine both channels into a single signal (make mono).
            float b = in1[i] + in2[i];

            // Basic envelope follower: If the input signal exceeds the current
            // envelope value, then increase the envelope. The attack determines
            // how fast this happens, using a one-pole filter. Otherwise, if the
            // signal is below the current envelope value, decrease the envelope
            // level using the release coefficient.

            float x = (b < 0.0f) ? -b : b;     // rectify
            if (x > env) {
                env += attack * (x - env);     // attack
                releaseRate = 1.0f - release;  // reset rate
            } else {
                env *= release + releaseRate;  // release
                releaseRate *= 0.9999f;        // increase release rate
            }

            // Audio output is the combined signal.
            x = gain * b;

            // Flatten audio signal by dividing by envelope value.
            if (flat) {
                if (env < 0.01f) {  // don't divide by zero
                    x *= 100.0f;
                } else {
                    x /= env;
                }
            }

            out1[i] = x;           // audio on left channel
            out2[i] = 0.5f * env;  // envelope on right channel
	    }
    } else {  // VCA mode
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float a = in1[i];
            float b = in2[i];

            // Same envelope follower as above, but only on right channel.
            float x = (b < 0.0f) ? -b : b;
            if (x > env) {
                env += attack * (x - env);
                releaseRate = 1.0f - release;
            } else {
                env *= release + releaseRate;
                releaseRate *= 0.9999f;
            }

            // Apply the envelope to the audio from the left channel.
            x = gain * a * env;

            // Output the same audio signal on both channels.
            out1[i] = x;
            out2[i] = x;
        }
    }
}

juce::AudioProcessorEditor *MDAEnvelopeAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDAEnvelopeAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAEnvelopeAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAEnvelopeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::StringArray { "L x |R|", "IN/ENV", "FLAT/ENV" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Attack", 1),
        "Attack",
        juce::NormalisableRange<float>(),
        0.25f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(std::pow(10.0f, 3.0f * value)));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Release", 1),
        "Release",
        juce::NormalisableRange<float>(),
        0.4f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(std::pow(10.0f, 4.0f * value)));
            })));

    // Comment from original code:
    // attack & release times not accurate - included to make user happy!

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gain", 1),
        "Gain",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDAEnvelopeAudioProcessor();
}
