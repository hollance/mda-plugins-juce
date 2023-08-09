#include "PluginProcessor.h"

MDADynamicsAudioProcessor::MDADynamicsAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDADynamicsAudioProcessor::~MDADynamicsAudioProcessor()
{
}

const juce::String MDADynamicsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDADynamicsAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDADynamicsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDADynamicsAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDADynamicsAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDADynamicsAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDADynamicsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDADynamicsAudioProcessor::releaseResources()
{
}

void MDADynamicsAudioProcessor::reset()
{
    resetState();
}

bool MDADynamicsAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDADynamicsAudioProcessor::resetState()
{
    env = 0.0f;
    limiterEnv = 0.0f;
    gateEnv = 0.0f;
}

void MDADynamicsAudioProcessor::update()
{
    compressOnly = true;

    // Convert decibels to linear value
    float param1 = apvts.getRawParameterValue("Thresh")->load();
    threshold = std::pow(10.0f, param1 * 0.05f);

    // The different values that `ratio` can take on are:
    //    -0.3 - 0.0  = ratio of 0.5:1 to 0.99:1 (upwards expand)
    //     0.0 - 0.95 = ratio 1:1 to 20:1 (regular compression)
    //    0.95 - 1.04 = "Limit" (essentially infinite ratio)
    //    1.04 - 17.0 = ratio -1:1 to -17:1 (compress upside down?)
    float param2 = apvts.getRawParameterValue("Ratio")->load();
    ratio = 2.5f * param2 - 0.5f;
    if (ratio > 1.0f) {
        ratio = 1.0f + 16.0f*(ratio - 1.0f)*(ratio - 1.0f);
        compressOnly = false;
    }
    if (ratio < 0.0f) {
        ratio = 0.6f * ratio;
        compressOnly = false;
    }
    // When threshold is below -20 dB and we're upwards expanding,
    // make the ratio more extreme. (Not sure why this is done.)
    if (ratio < 0.0f && threshold < 0.1f) {
        ratio *= threshold * 15.0f;
    }

    // Convert decibels to linear value
    float param3 = apvts.getRawParameterValue("Output")->load();
    trim = std::pow(10.0f, param3 * 0.05f);

    // Filter coefficient for envelope attack
    float param4 = apvts.getRawParameterValue("Attack")->load();
    attack = std::pow(10.0f, -0.002f - 2.0f * param4);

    // Filter coefficient for envelope release
    float param5 = apvts.getRawParameterValue("Release")->load();
    release = 1.0f - std::pow(10.0f, -2.0f - 3.0f * param5);

    // Limiter threshold to linear gain
    float param6 = apvts.getRawParameterValue("Limiter")->load();
    if (param6 > 0.98f) {
        limiterThreshold = 1000.0f;  // limiter off
    } else {
        limiterThreshold = 0.99f * std::pow(10.0f, std::floor(30.0*param6 - 20.0)/20.f);
        compressOnly = false;
    }

    // Gate threshold to linear gain
    float param7 = apvts.getRawParameterValue("Gate Thr")->load();
    if (param7 < 0.02f) {
        gateThreshold = 0.0f;   // expander
    } else {
        gateThreshold = std::pow(10.0f, 3.0f * param7 - 3.0f);
        compressOnly = false;
    }

    // Filter coefficient for gate attack
    float param8 = apvts.getRawParameterValue("Gate Att")->load();
    gateAttack = std::pow(10.0f, -0.002f - 3.0f * param8);

    // Filter coefficient for envelope release
    float param9 = apvts.getRawParameterValue("Gate Rel")->load();
    gateRelease = 1.0f - std::pow(10.0f, -2.0f - 3.3f * param9);

    // Dry/wet mix
    float param10 = apvts.getRawParameterValue("Mix")->load() * 0.01f;
    dry = 1.0f - param10;
    trim *= param10;
}

void MDADynamicsAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    if (compressOnly) {
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            float a = in1[s];
            float b = in2[s];

            // Rectify the level so it's always a positive value.
            float i = (a < 0.0f) ? -a : a;
            float j = (b < 0.0f) ? -b : b;

            // Get peak level. There is only one envelope follower that works
            // on both channels, using whichever channel is loudest.
            i = (j > i) ? j : i;

            // Simple envelope follower.
            env = (i > env) ? env + attack * (i - env) : env * release;

            // Calculate the gain. If the envelope level is over the threshold,
            // the ratio kicks in to reduce the gain. `trim` is makeup gain.
            float g = (env > threshold) ? trim / (1.0f + ratio * (env/threshold - 1.0f)) : trim;

            // Apply the same gain to both channels and mix in the dry signal.
            out1[s] = a * (g + dry);
            out2[s] = b * (g + dry);
        }
    } else {
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            float a = in1[s];
            float b = in2[s];

            // Calculate the compressor's gain (same code as above).
            float i = (a < 0.0f) ? -a : a;
            float j = (b < 0.0f) ? -b : b;
            i = (j > i) ? j : i;
            env = (i > env) ? env + attack * (i - env) : env * release;
            float g = (env > threshold) ? trim / (1.0f + ratio * (env/threshold - 1.0f)) : trim;

            // Limiter envelope, this has no attack and uses the same
            // release time as the compressor.
            limiterEnv = (i > env) ? i : limiterEnv * release;

            // Limit the gain. This applies the limiter envelope to the
            // gain from the compressor.
            if (g < 0.0f) {
                g = 0.0f;
            }
            if (g * limiterEnv > limiterThreshold) {
                g = limiterThreshold / limiterEnv;
            }

            // Gate. When the current envelope level exceeds the threshold,
            // the gate envelope increases towards 1.0 using the attack rate.
            // When the current envelope level falls below the threshold,
            // the gate envelope decays towards 0.0 using the release rate.
            gateEnv = (env > gateThreshold) ? gateEnv + gateAttack * (1.0f - gateEnv) : gateEnv * gateRelease;

            // Apply the gated gain from the compressor and limiter.
            out1[s] = a * (g * gateEnv + dry);
            out2[s] = b * (g * gateEnv + dry);
        }
    }
}

juce::AudioProcessorEditor *MDADynamicsAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDADynamicsAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDADynamicsAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDADynamicsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Thresh", 1),
        "Thresh",
        juce::NormalisableRange<float>(-40.0f, 0.0f, 0.01f),
        -16.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Ratio", 1),
        "Ratio",
        juce::NormalisableRange<float>(),
        0.4f,
        juce::AudioParameterFloatAttributes()
            .withLabel(":1")
            .withStringFromValueFunction([](float value, int)
            {
                float ratio = 2.5f * value - 0.5f;
                if (ratio > 1.0f) {
                    ratio = 1.0f + 16.0f*(ratio - 1.0f)*(ratio - 1.0f);
                }
                if (ratio < 0.0f) {
                    ratio = 0.6f * ratio;
                }
                if (value > 0.58f) {
                    if (value < 0.62f) {
                        return juce::String("Limit");
                    } else {
                        return juce::String(-ratio, 2);
                    }
                } else {
                    if (value < 0.2f) {
                        return juce::String(0.5f + 2.5f * value, 2);
                    } else {
                        return juce::String(1.0f/(1.0f - ratio), 2);
                    }
                }
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(0.0f, 40.0f, 0.01f),
        4.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Not sure if the displayed attack and release times actually make sense!
    // With JUCE it's much simpler to define the actual range in milliseconds
    // and convert that into the filter coefficient, instead of the other way
    // around like it was done here. (Also, microseconds?!)

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Attack", 1),
        "Attack",
        juce::NormalisableRange<float>(),
        0.18f,
        juce::AudioParameterFloatAttributes()
            .withLabel("us")
            .withStringFromValueFunction([](float value, int)
            {
                float attack = std::pow(10.0f, -0.002f - 2.0f * value);
                float time = -301030.1f / (44100.0f * std::log10(1.0f - attack));
                return juce::String(time, 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Release", 1),
        "Release",
        juce::NormalisableRange<float>(),
        0.55f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                float release = 1.0f - std::pow(10.0f, -2.0f - 3.0f * value);
                float time = -301.0301f / (44100.0f * std::log10(release));
                return juce::String(time, 2);
            })));

    // -20 to +10 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Limiter", 1),
        "Limiter",
        juce::NormalisableRange<float>(),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                if (value > 0.98f) {
                    return juce::String("OFF");
                } else {
                    return juce::String(int(30.0*value - 20.0));
                }
            })));

    // -60 to 0 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gate Thr", 1),
        "Gate Thr",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                if (value < 0.02f) {
                    return juce::String("OFF");
                } else {
                    return juce::String(int(60.0*value - 60.0));
                }
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gate Att", 1),
        "Gate Att",
        juce::NormalisableRange<float>(),
        0.10f,
        juce::AudioParameterFloatAttributes()
            .withLabel("us")
            .withStringFromValueFunction([](float value, int)
            {
                float attack = std::pow(10.0f, -0.002f - 3.0f * value);
                float time = -301030.1f / (44100.0f * std::log10(1.0f - attack));
                return juce::String(time, 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gate Rel", 1),
        "Gate Rel",
        juce::NormalisableRange<float>(),
        0.50f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                float release = 1.0f - std::pow(10.0f, -2.0f - 3.3f * value);
                float time = -1806.0f / (44100.0f * std::log10(release));
                return juce::String(time, 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDADynamicsAudioProcessor();
}
