#include "PluginProcessor.h"

// Lookup table of filter coefficients.
static float loudness[14][3] =
{
    {402.f,  0.0025f,  0.00f},  // -60 dB
    {334.f,  0.0121f,  0.00f},
    {256.f,  0.0353f,  0.00f},
    {192.f,  0.0900f,  0.00f},
    {150.f,  0.2116f,  0.00f},
    {150.f,  0.5185f,  0.00f},
    { 1.0f,     0.0f,  0.00f},  // 0 dB
    {33.7f,     5.5f,  1.00f},
    {92.0f,     8.7f,  0.62f},
    {63.7f,    18.4f,  0.44f},
    {42.9f,    48.2f,  0.30f},
    {37.6f,   116.2f,  0.18f},
    {22.9f,   428.7f,  0.09f},  // +60 dB
    { 0.0f,     0.0f,  0.00f}
};

MDALoudnessAudioProcessor::MDALoudnessAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDALoudnessAudioProcessor::~MDALoudnessAudioProcessor()
{
}

const juce::String MDALoudnessAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDALoudnessAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDALoudnessAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDALoudnessAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDALoudnessAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDALoudnessAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDALoudnessAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDALoudnessAudioProcessor::releaseResources()
{
}

void MDALoudnessAudioProcessor::reset()
{
    resetState();
}

bool MDALoudnessAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDALoudnessAudioProcessor::resetState()
{
    z0 = z1 = z2 = z3 = 0.0f;
}

void MDALoudnessAudioProcessor::update()
{
    float igain = apvts.getRawParameterValue("Loudness")->load();
    float ogain = apvts.getRawParameterValue("Output")->load();

    // Convert the decibel value to an index in the `loudness` table.
    float f = 0.1f * igain + 6.0f;
    int i = int(f);  // coefficient index
    f -= float(i);   // fractional part

    // Do a linear interpolation between the entries in the table.
    a0 = loudness[i][0] + f * (loudness[i + 1][0] - loudness[i][0]);
    a1 = loudness[i][1] + f * (loudness[i + 1][1] - loudness[i][1]);
    a2 = loudness[i][2] + f * (loudness[i + 1][2] - loudness[i][2]);

    // The first entry in the table is a frequency; convert it to a
    // low-pass filter coefficient.
    a0 = 1.0f - std::exp(-6.283153f * a0 / getSampleRate());

    // Cutting or boosting?
    mode = (igain > 0.0f);

    // Calculate the output gain, convert from dB to linear value.
    float tmp = ogain;
    if (apvts.getRawParameterValue("Link")->load()) {  // linked gain
        tmp -= igain;
        if (tmp > 0.0f) {  // limit max gain
            tmp = 0.0f;
        }
    }
    gain = std::pow(10.0f, 0.05f * tmp);
}

void MDALoudnessAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    if (mode == 0) {  // cut
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float a = in1[i];
            float b = in2[i];

            // Filter for the left channel.
            z0 += a0 * (a - z0 + 0.3f * z1);
            a -= z0;
            z1 += a0 * (a - z1);
            a -= z1;
            a -= z0 * a1;

            // Filter for the right channel.
            z2 += a0 * (b - z2 + 0.3f * z1);
            b -= z2;
            z3 += a0 * (b - z3);
            b -= z3;
            b -= z2 * a1;

            out1[i] = a * gain;
            out2[i] = b * gain;
        }
    } else {  // boost
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float a = in1[i];
            float b = in2[i];

            // This is two one-pole low-pass filters in a row.
            z0 += a0 * (a - z0);
            z1 += a0 * (z0 - z1);

            // Add the low-pass filtered values to the input to boost it.
            a += a1 * (z1 - a2 * z0);

            // Same filtering but for the right channel.
            z2 += a0 * (b - z2);
            z3 += a0 * (z2 - z3);
            b += a1 * (z3 - a2 * z2);

            out1[i] = a * gain;
            out2[i] = b * gain;
        }
    }
}

juce::AudioProcessorEditor *MDALoudnessAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDALoudnessAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDALoudnessAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDALoudnessAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Loudness", 1),
        "Loudness",
        juce::NormalisableRange<float>(-60.0f, 60.0f, 0.01f, 0.5f, true),
        9.6f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-60.0f, 60.0f, 0.01f, 0.5f, true),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("Link", 1),
        "Link",
        false));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDALoudnessAudioProcessor();
}
