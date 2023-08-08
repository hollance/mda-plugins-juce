#include "PluginProcessor.h"

MDARezFilterAudioProcessor::MDARezFilterAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDARezFilterAudioProcessor::~MDARezFilterAudioProcessor()
{
}

const juce::String MDARezFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDARezFilterAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDARezFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDARezFilterAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDARezFilterAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDARezFilterAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDARezFilterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDARezFilterAudioProcessor::releaseResources()
{
}

void MDARezFilterAudioProcessor::reset()
{
    resetState();
}

bool MDARezFilterAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDARezFilterAudioProcessor::resetState()
{
    env = 0.0f;
    lfo = 0.0f;
    lfoPhase = 0.0f;
    buf0 = 0.0f;
    buf1 = 0.0f;
    triggerEnv = 0.0f;
    triggered = false;
    triggerAttack = false;
}

void MDARezFilterAudioProcessor::update()
{
    // Map [0%, 100%] to [-0.15, 1.35]
    float param0 = apvts.getRawParameterValue("Freq")->load() * 0.01f;
    cutoff = 1.5f * param0 * param0 - 0.15f;

    // Map [0%, 100%] to [0.0, 0.99]
    float param1 = apvts.getRawParameterValue("Res")->load() * 0.01f;
    q = 0.99f * std::pow(param1, 0.3f);

    // Convert decibels to linear value
    float param2 = apvts.getRawParameterValue("Output")->load();
    gain = 0.5f * std::pow(10.0f, param2 * 0.05f);

    // Map [-100, 100] to [-0.5, 0.5]
    float param3 = (apvts.getRawParameterValue("Env->VCF")->load() + 100.0f) / 200.0f;
    envDepth = 2.0f * (0.5f - param3) * (0.5f - param3);
    envDepth = (param3 > 0.5f) ? envDepth : -envDepth;

    // Filter coefficient for envelope attack
    float param4 = apvts.getRawParameterValue("Attack")->load();
    attack = std::pow(10.0f, -0.01f - 4.0f * param4);

    // Filter coefficient for envelope release
    float param5 = apvts.getRawParameterValue("Release")->load();
    release = 1.0f - std::pow(10.0f, -2.0f - 4.0f * param5);

    // Map [-100, 100] to [-0.5, 0.5]
    float param6 = (apvts.getRawParameterValue("LFO->VCF")->load() + 100.0f) / 200.0f;
    lfoDepth = 2.0f * (param6 - 0.5f) * (param6 - 0.5f);

    // Map [0, 1] to [0.01 Hz, 100 Hz] and calculate phase increment
    float param7 = apvts.getRawParameterValue("LFO Rate")->load();
    lfoInc = 6.2832f * std::pow(10.0f, 4.0f * param7 - 2.0f) / getSampleRate();

    // Need to enable sample&hold mode?
    sampleHold = false;
    if (param6 < 0.5f) {
        sampleHold = true;
        lfoInc *= 0.15915f;  // divide by 2π so lfoPhase counts from 0 to 1
        lfoDepth *= 0.001f;  // random numbers are between ±1000
    }

    // Map [0, 1] to [0, 3]
    float param8 = apvts.getRawParameterValue("Trigger")->load();
    if (param8 < 0.1f) {
        threshold = 0.0f;
    } else {
        threshold = 3.0f * param8 * param8;
    }

    // For this filter, the allowed maximum frequency is higher when Q is
    // larger, but make sure it doesn't exceed the value set by the user.
    float param9 = apvts.getRawParameterValue("Max Freq")->load() * 0.01f;
    filterMax = 0.99f + 0.3f * param1;
    if (filterMax > 1.3f * param9) {
        filterMax = 1.3f * param9;
    }
}

void MDARezFilterAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    float b0 = buf0, b1 = buf1;

    if (threshold == 0.0f) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Process as mono
            float a = in1[i] + in2[i];

            // Simple envelope follower
            float level = (a > 0.0f) ? a : -a;
            env = (level > env) ? env + attack * (level - env) : env * release;

            // LFO
            if (!sampleHold) {
                lfo = lfoDepth * std::sin(lfoPhase);
            } else if (lfoPhase > 1.0f) {
                lfo = lfoDepth * (rand() % 2000 - 1000);
                lfoPhase = 0.0f;
            }
            lfoPhase += lfoInc;

            // Calculate modulated frequency
            float f = cutoff + envDepth * env + lfo;
            if (f < 0.0f) {
                f = 0.0f;
            } else if (f > filterMax) {
                f = filterMax;
            }

            // Apply the filter
            // I didn't study this filter in detail but it's likely some kind
            // of variation of the Chamberlin SVF, which can be unstable for
            // cutoff frequencies close to Nyquist. Probably shouldn't use it
            // in modern audio code!
            float tmp = q + q * (1.0f + f * (1.0f + 1.1f * f));
            b0 += f * (gain * a - b0 + tmp * (b0 - b1));
            b1 += f * (b0 - b1);

            /*
            // Alternative implementation also found in the original code:
            float o = 1.0f - f;
            b0 = o * b0 + f * (gain*a + q*(1.0f + (1.0f/o)) * (b0 - b1));
            b1 = o * b1 + f * b0;
            b2 = o * b2 + f * b1;
            */

            out1[i] = b1;
            out2[i] = b1;
        }
    } else {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Process as mono.
            float a = in1[i] + in2[i];

            // This envelope follower has instant attack.
            float level = (a > 0.0f) ? a : -a;
            env = (level > env) ? level : env * release;

            // Envelope level exceeds threshold?
            if (env > threshold) {
                if (!triggered) {
                    triggerAttack = true;
                    if (sampleHold) {  // trigger new S&H
                        lfoPhase = 2.0f;
                    }
                }
                triggered = true;
            } else {
                triggered = false;
            }

            // NOTE: Not sure what the point is of the second envelope, as
            // it's not actually used anywhere. When the main envelope hits
            // the threshold, nothing actually happens to the audio signal.
            // (It's possible I missed something when converting the code,
            // or that this part of the plug-in wasn't finished.)

            if (triggerAttack) {
                triggerEnv += attack * (1.0f - triggerEnv);
                if (triggerEnv > 0.999f) {
                    triggerAttack = false;
                }
            } else {
                triggerEnv *= release;
            }

            // The code below is the same as before.

            if (!sampleHold) {
                lfo = lfoDepth * std::sin(lfoPhase);
            } else if (lfoPhase > 1.0f) {
                lfo = lfoDepth * (rand() % 2000 - 1000);
                lfoPhase = 0.0f;
            }
            lfoPhase += lfoInc;

            float f = cutoff + envDepth * env + lfo;
            if (f < 0.0f) {
                f = 0.0f;
            } else if (f > filterMax) {
                f = filterMax;
            }

            float tmp = q + q * (1.0f + f * (1.0f + 1.1f * f));
            b0 += f * (gain * a - b0 + tmp * (b0 - b1));
            b1 += f * (b0 - b1);

            out1[i] = b1;
            out2[i] = b1;
        }
    }

    if (std::abs(b0) < 1.0e-10f) {
        buf0 = 0.0f;
        buf1 = 0.0f;
    } else {
        buf0 = b0;
        buf1 = b1;
    }

    lfoPhase = std::fmod(lfoPhase, 6.2831853f);
}

juce::AudioProcessorEditor *MDARezFilterAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDARezFilterAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDARezFilterAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDARezFilterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Freq", 1),
        "Freq",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        33.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Res", 1),
        "Res",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        70.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Env->VCF", 1),
        "Env->VCF",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        70.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Not sure if the displayed attack and release times actually make sense!
    // With JUCE it's much simpler to define the actual range in milliseconds
    // and convert that into the filter coefficient, instead of the other way
    // around like it was done here.

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Attack", 1),
        "Attack",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                float attack = std::pow(10.0f, -0.01f - 4.0f * value);
                float time = -301.0301 / (44100.0f * log10(1.0f - attack));
                return juce::String(time, 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Release", 1),
        "Release",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float value, int)
            {
                float release = 1.0f - std::pow(10.0f, -2.0f - 4.0f * value);
                float time = -301.0301 / (44100.0f * log10(release));
                return juce::String(time, 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LFO->VCF", 1),
        "LFO->VCF",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
        70.0f,
        juce::AudioParameterFloatAttributes().withLabel("S+H | Sin")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LFO Rate", 1),
        "LFO Rate",
        juce::NormalisableRange<float>(),
        0.4f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([](float value, int)
            {
                // 0.01 Hz to 100 Hz
                return juce::String(std::pow(10.0f, 4.0f * value - 2.0f), 2);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Trigger", 1),
        "Trigger",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float value, int)
            {
                // -36 dB to +3 dB
                float threshold = (value < 0.1f) ? 0.0f : 3.0f * value * value;
                if (threshold == 0.0f) {
                    return juce::String("FREE RUN");
                } else {
                    return juce::String(int(20.0f * std::log10(0.5f * threshold)));
                }
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Max Freq", 1),
        "Max Freq",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        75.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDARezFilterAudioProcessor();
}
