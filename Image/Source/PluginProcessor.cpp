#include "PluginProcessor.h"

MDAImageAudioProcessor::MDAImageAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDAImageAudioProcessor::~MDAImageAudioProcessor()
{
}

const juce::String MDAImageAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDAImageAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDAImageAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDAImageAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDAImageAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDAImageAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAImageAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDAImageAudioProcessor::releaseResources()
{
}

void MDAImageAudioProcessor::reset()
{
    resetState();
}

bool MDAImageAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAImageAudioProcessor::resetState()
{
    l2l = 1.0f;
    r2r = 1.0f;
    l2r = 0.0f;
    r2l = 0.0f;
}

void MDAImageAudioProcessor::update()
{
    float w = apvts.getRawParameterValue("S Width")->load() * 0.01f;       // -2 .. 2
    float k = apvts.getRawParameterValue("S Pan")->load() * 0.01f + 1.0f;  //  0 .. 2
    float c = apvts.getRawParameterValue("M Level")->load() * 0.01f;       // -2 .. 2
    float b = apvts.getRawParameterValue("M Pan")->load() * 0.01f + 1.0f;  //  0 .. 2

    // Convert decibels to linear gain.
    float g = std::pow(10.0, 0.05f * apvts.getRawParameterValue("Output")->load());

    /*
        Calculate gain coefficients, where:

        l2l = gain for left input going into left output
        r2l = gain for right input going into left output
        r2r = gain for right input going into right output
        l2r = gain for left input going into right output

        ┌────────────┐       l2l                         ┌────────────┐
        │            ├──────────────────────────────────▶│            │
        │    L In    ├─────────────────────┐             │   L Out    │
        │            │       l2r        ┌──┼────────────▶│            │
        └────────────┘                  │  │             └────────────┘
                                        │  │
        ┌────────────┐                  │  │             ┌────────────┐
        │            │       r2l        │  └────────────▶│            │
        │    R In    ├──────────────────┘                │   R Out    │
        │            ├──────────────────────────────────▶│            │
        └────────────┘       r2r                         └────────────┘
    */

    int mode = int(apvts.getRawParameterValue("Mode")->load());
    switch (mode) {
        case 0:  // SM->LR
            r2l =  g * c * (2.0f - b);
            l2l =  g * w * (2.0f - k);
            r2r =  g * c * b;
            l2r = -g * w * k;
            break;

        case 1:  // MS->LR
            l2l =  g * c * (2.0f - b);
            r2l =  g * w * (2.0f - k);
            l2r =  g * c * b;
            r2r = -g * w * k;
            break;

        case 2:  // LR->LR
            g *= 0.5f;
            l2l = g * (c * (2.0f - b) + w * (2.0f - k));
            r2l = g * (c * (2.0f - b) - w * (2.0f - k));
            l2r = g * (c * b - w * k);
            r2r = g * (c * b + w * k);
            break;

        case 3:  // LR->MS
            g *= 0.5f;
            l2l =  g * (2.0f - b) * (2.0f - k);
            r2l =  g * (2.0f - b) * k;
            l2r = -g * b * (2.0f - k);
            r2r =  g * b * k;
            break;
    }
}

void MDAImageAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

        float c = l2l * a + r2l * b;
        float d = r2r * b + l2r * a;

        out1[i] = c;
        out2[i] = d;
    }
}

juce::AudioProcessorEditor *MDAImageAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDAImageAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAImageAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAImageAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::StringArray { "SM->LR", "MS->LR", "LR->LR", "LR->MS" },
        2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("S Width", 1),
        "S Width",
        juce::NormalisableRange<float>(-200.0f, 200.0f, 0.01f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("S Pan", 1),
        "S Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("L<->R")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("M Level", 1),
        "M Level",
        juce::NormalisableRange<float>(-200.0f, 200.0f, 0.01f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("M Pan", 1),
        "M Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("L<->R")));

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
    return new MDAImageAudioProcessor();
}
