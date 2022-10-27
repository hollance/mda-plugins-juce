#include "PluginProcessor.h"

JX10Program::JX10Program()
{
    param[0]  = 0.00f;  // OSC Mix
    param[1]  = 0.25f;  // OSC Tune
    param[2]  = 0.50f;  // OSC Fine
    param[3]  = 0.00f;  // Mode
    param[4]  = 0.35f;  // Glide Rate
    param[5]  = 0.50f;  // Glide Bend
    param[6]  = 1.00f;  // VCF Freq
    param[7]  = 0.15f;  // VCF Reso
    param[8]  = 0.75f;  // VCF <Env
    param[9]  = 0.00f;  // VCF <LFO
    param[10] = 0.50f;  // VCF <Vel
    param[11] = 0.00f;  // VCF Att
    param[12] = 0.30f;  // VCF Dec
    param[13] = 0.00f;  // VCF Sus
    param[14] = 0.25f;  // VCF Rel
    param[15] = 0.00f;  // ENV Att
    param[16] = 0.50f;  // ENV Dec
    param[17] = 1.00f;  // ENV Sus
    param[18] = 0.30f;  // ENV Rel
    param[19] = 0.81f;  // LFO Rate
    param[20] = 0.50f;  // Vibrato
    param[21] = 0.00f;  // Noise
    param[22] = 0.50f;  // Octave
    param[23] = 0.50f;  // Tuning

    strcpy(name, "Empty Patch");
}

JX10Program::JX10Program(const char *name,
                         float p0,  float p1,  float p2,  float p3,
                         float p4,  float p5,  float p6,  float p7,
                         float p8,  float p9,  float p10, float p11,
                         float p12, float p13, float p14, float p15,
                         float p16, float p17, float p18, float p19,
                         float p20, float p21, float p22, float p23)
{
    strcpy(this->name, name);
    param[0]  = p0;  param[1]  = p1;  param[2]  = p2;  param[3]  = p3;
    param[4]  = p4;  param[5]  = p5;  param[6]  = p6;  param[7]  = p7;
    param[8]  = p8;  param[9]  = p9;  param[10] = p10; param[11] = p11;
    param[12] = p12; param[13] = p13; param[14] = p14; param[15] = p15;
    param[16] = p16; param[17] = p17; param[18] = p18; param[19] = p19;
    param[20] = p20; param[21] = p21; param[22] = p22; param[23] = p23;
}

JX10AudioProcessor::JX10AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    _sampleRate = 44100.0f;
    _inverseSampleRate = 1.0f / _sampleRate;

    createPrograms();
    setCurrentProgram(0);
}

JX10AudioProcessor::~JX10AudioProcessor()
{
}

const juce::String JX10AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int JX10AudioProcessor::getNumPrograms()
{
    return int(_programs.size());
}

int JX10AudioProcessor::getCurrentProgram()
{
    return _currentProgram;
}

void JX10AudioProcessor::setCurrentProgram(int index)
{
    _currentProgram = index;

    const char *paramNames[] = {
        "OSC Mix",
        "OSC Tune",
        "OSC Fine",
        "Mode",
        "Gld Rate",
        "Gld Bend",
        "VCF Freq",
        "VCF Reso",
        "VCF Env",
        "VCF LFO",
        "VCF Vel",
        "VCF Att",
        "VCF Dec",
        "VCF Sus",
        "VCF Rel",
        "ENV Att",
        "ENV Dec",
        "ENV Sus",
        "ENV Rel",
        "LFO Rate",
        "Vibrato",
        "Noise",
        "Octave",
        "Tuning",
    };

    for (int i = 0; i < NPARAMS; ++i) {
        apvts.getParameter(paramNames[i])->setValueNotifyingHost(_programs[index].param[i]);
    }
}

const juce::String JX10AudioProcessor::getProgramName(int index)
{
    return { _programs[index].name };
}

void JX10AudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    // not implemented
}

void JX10AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    _sampleRate = sampleRate;
    _inverseSampleRate = 1.0f / _sampleRate;

    resetState();
}

void JX10AudioProcessor::releaseResources()
{
}

void JX10AudioProcessor::reset()
{
    resetState();
}

bool JX10AudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void JX10AudioProcessor::createPrograms()
{
    _programs.emplace_back("5th Sweep Pad", 1.0f, 0.37f, 0.25f, 0.3f, 0.32f, 0.5f, 0.9f, 0.6f, 0.12f, 0.0f, 0.5f, 0.9f, 0.89f, 0.9f, 0.73f, 0.0f, 0.5f, 1.0f, 0.71f, 0.81f, 0.65f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Echo Pad [SA]", 0.88f, 0.51f, 0.5f, 0.0f, 0.49f, 0.5f, 0.46f, 0.76f, 0.69f, 0.1f, 0.69f, 1.0f, 0.86f, 0.76f, 0.57f, 0.3f, 0.8f, 0.68f, 0.66f, 0.79f, 0.13f, 0.25f, 0.45f, 0.5f);
    _programs.emplace_back("Space Chimes [SA]", 0.88f, 0.51f, 0.5f, 0.16f, 0.49f, 0.5f, 0.49f, 0.82f, 0.66f, 0.08f, 0.89f, 0.85f, 0.69f, 0.76f, 0.47f, 0.12f, 0.22f, 0.55f, 0.66f, 0.89f, 0.34f, 0.0f, 1.0f, 0.5f);
    _programs.emplace_back("Solid Backing", 1.0f, 0.26f, 0.14f, 0.0f, 0.35f, 0.5f, 0.3f, 0.25f, 0.7f, 0.0f, 0.63f, 0.0f, 0.35f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.5f, 0.5f, 0.5f, 0.5f);
    _programs.emplace_back("Velocity Backing [SA]", 0.41f, 0.5f, 0.79f, 0.0f, 0.08f, 0.32f, 0.49f, 0.01f, 0.34f, 0.0f, 0.93f, 0.61f, 0.87f, 1.0f, 0.93f, 0.11f, 0.48f, 0.98f, 0.32f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Rubber Backing [ZF]", 0.29f, 0.76f, 0.26f, 0.0f, 0.18f, 0.76f, 0.35f, 0.15f, 0.77f, 0.14f, 0.54f, 0.0f, 0.42f, 0.13f, 0.21f, 0.0f, 0.56f, 0.0f, 0.32f, 0.2f, 0.58f, 0.22f, 0.53f, 0.5f);
    _programs.emplace_back("808 State Lead", 1.0f, 0.65f, 0.24f, 0.4f, 0.34f, 0.85f, 0.65f, 0.63f, 0.75f, 0.16f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.17f, 0.5f, 1.0f, 0.03f, 0.81f, 0.5f, 0.0f, 0.68f, 0.5f);
    _programs.emplace_back("Mono Glide", 0.0f, 0.25f, 0.5f, 1.0f, 0.46f, 0.5f, 0.51f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.25f, 0.37f, 0.5f, 1.0f, 0.38f, 0.81f, 0.62f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Detuned Techno Lead", 0.84f, 0.51f, 0.15f, 0.45f, 0.41f, 0.42f, 0.54f, 0.01f, 0.58f, 0.21f, 0.67f, 0.0f, 0.09f, 1.0f, 0.25f, 0.2f, 0.85f, 1.0f, 0.3f, 0.83f, 0.09f, 0.4f, 0.49f, 0.5f);
    _programs.emplace_back("Hard Lead [SA]", 0.71f, 0.75f, 0.53f, 0.18f, 0.24f, 1.0f, 0.56f, 0.52f, 0.69f, 0.19f, 0.7f, 1.0f, 0.14f, 0.65f, 0.95f, 0.07f, 0.91f, 1.0f, 0.15f, 0.84f, 0.33f, 0.0f, 0.49f, 0.5f);
    _programs.emplace_back("Bubble", 0.0f, 0.25f, 0.43f, 0.0f, 0.71f, 0.48f, 0.23f, 0.77f, 0.8f, 0.32f, 0.63f, 0.4f, 0.18f, 0.66f, 0.14f, 0.0f, 0.38f, 0.65f, 0.16f, 0.48f, 0.5f, 0.0f, 0.67f, 0.5f);
    _programs.emplace_back("Monosynth", 0.62f, 0.26f, 0.51f, 0.79f, 0.35f, 0.54f, 0.64f, 0.39f, 0.51f, 0.65f, 0.0f, 0.07f, 0.52f, 0.24f, 0.84f, 0.13f, 0.3f, 0.76f, 0.21f, 0.58f, 0.3f, 0.0f, 0.36f, 0.5f);
    _programs.emplace_back("Moogcury Lite", 0.81f, 1.0f, 0.21f, 0.78f, 0.15f, 0.35f, 0.39f, 0.17f, 0.69f, 0.4f, 0.62f, 0.0f, 0.47f, 0.19f, 0.37f, 0.0f, 0.5f, 0.2f, 0.33f, 0.38f, 0.53f, 0.0f, 0.12f, 0.5f);
    _programs.emplace_back("Gangsta Whine", 0.0f, 0.51f, 0.52f, 0.96f, 0.44f, 0.5f, 0.41f, 0.46f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 0.15f, 0.5f, 1.0f, 0.32f, 0.81f, 0.49f, 0.0f, 0.83f, 0.5f);
    _programs.emplace_back("Higher Synth [ZF]", 0.48f, 0.51f, 0.22f, 0.0f, 0.0f, 0.5f, 0.5f, 0.47f, 0.73f, 0.3f, 0.8f, 0.0f, 0.1f, 0.0f, 0.07f, 0.0f, 0.42f, 0.0f, 0.22f, 0.21f, 0.59f, 0.16f, 0.98f, 0.5f);
    _programs.emplace_back("303 Saw Bass", 0.0f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.0f, 0.56f, 0.0f, 0.56f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f);
    _programs.emplace_back("303 Square Bass", 0.75f, 0.51f, 0.5f, 0.83f, 0.49f, 0.5f, 0.55f, 0.75f, 0.69f, 0.35f, 0.5f, 0.14f, 0.49f, 0.0f, 0.39f, 0.0f, 0.8f, 1.0f, 0.24f, 0.26f, 0.49f, 0.0f, 0.07f, 0.5f);
    _programs.emplace_back("Analog Bass", 1.0f, 0.25f, 0.2f, 0.81f, 0.19f, 0.5f, 0.3f, 0.51f, 0.85f, 0.09f, 0.0f, 0.0f, 0.88f, 0.0f, 0.21f, 0.0f, 0.5f, 1.0f, 0.46f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f);
    _programs.emplace_back("Analog Bass 2", 1.0f, 0.25f, 0.2f, 0.72f, 0.19f, 0.86f, 0.48f, 0.43f, 0.94f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.61f, 1.0f, 0.32f, 0.81f, 0.5f, 0.0f, 0.27f, 0.5f);
    _programs.emplace_back("Low Pulses", 0.97f, 0.26f, 0.3f, 0.0f, 0.35f, 0.5f, 0.8f, 0.4f, 0.52f, 0.0f, 0.5f, 0.0f, 0.77f, 0.0f, 0.25f, 0.0f, 0.5f, 1.0f, 0.3f, 0.81f, 0.16f, 0.0f, 0.0f, 0.5f);
    _programs.emplace_back("Sine Infra-Bass", 0.0f, 0.25f, 0.5f, 0.65f, 0.35f, 0.5f, 0.33f, 0.76f, 0.53f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.0f, 0.55f, 0.25f, 0.3f, 0.81f, 0.52f, 0.0f, 0.14f, 0.5f);
    _programs.emplace_back("Wobble Bass [SA]", 1.0f, 0.26f, 0.22f, 0.64f, 0.82f, 0.59f, 0.72f, 0.47f, 0.34f, 0.34f, 0.82f, 0.2f, 0.69f, 1.0f, 0.15f, 0.09f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f);
    _programs.emplace_back("Squelch Bass", 1.0f, 0.26f, 0.22f, 0.71f, 0.35f, 0.5f, 0.67f, 0.7f, 0.26f, 0.0f, 0.5f, 0.48f, 0.69f, 1.0f, 0.15f, 0.0f, 0.5f, 1.0f, 0.07f, 0.81f, 0.46f, 0.0f, 0.24f, 0.5f);
    _programs.emplace_back("Rubber Bass [ZF]", 0.49f, 0.25f, 0.66f, 0.81f, 0.35f, 0.5f, 0.36f, 0.15f, 0.75f, 0.2f, 0.5f, 0.0f, 0.38f, 0.0f, 0.25f, 0.0f, 0.6f, 1.0f, 0.22f, 0.19f, 0.5f, 0.0f, 0.17f, 0.5f);
    _programs.emplace_back("Soft Pick Bass", 0.37f, 0.51f, 0.77f, 0.71f, 0.22f, 0.5f, 0.33f, 0.47f, 0.71f, 0.16f, 0.59f, 0.0f, 0.0f, 0.0f, 0.25f, 0.04f, 0.58f, 0.0f, 0.22f, 0.15f, 0.44f, 0.33f, 0.15f, 0.5f);
    _programs.emplace_back("Fretless Bass", 0.5f, 0.51f, 0.17f, 0.8f, 0.34f, 0.5f, 0.51f, 0.0f, 0.58f, 0.0f, 0.67f, 0.0f, 0.09f, 0.0f, 0.25f, 0.2f, 0.85f, 0.0f, 0.3f, 0.81f, 0.7f, 0.0f, 0.0f, 0.5f);
    _programs.emplace_back("Whistler", 0.23f, 0.51f, 0.38f, 0.0f, 0.35f, 0.5f, 0.33f, 1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.29f, 0.0f, 0.25f, 0.68f, 0.39f, 0.58f, 0.36f, 0.81f, 0.64f, 0.38f, 0.92f, 0.5f);
    _programs.emplace_back("Very Soft Pad", 0.39f, 0.51f, 0.27f, 0.38f, 0.12f, 0.5f, 0.35f, 0.78f, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 0.25f, 0.35f, 0.5f, 0.8f, 0.7f, 0.81f, 0.5f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Pizzicato", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.23f, 0.2f, 0.75f, 0.0f, 0.5f, 0.0f, 0.22f, 0.0f, 0.25f, 0.0f, 0.47f, 0.0f, 0.3f, 0.81f, 0.5f, 0.8f, 0.5f, 0.5f);
    _programs.emplace_back("Synth Strings", 1.0f, 0.51f, 0.24f, 0.0f, 0.0f, 0.35f, 0.42f, 0.26f, 0.75f, 0.14f, 0.69f, 0.0f, 0.67f, 0.55f, 0.97f, 0.82f, 0.7f, 1.0f, 0.42f, 0.84f, 0.67f, 0.3f, 0.47f, 0.5f);
    _programs.emplace_back("Synth Strings 2", 0.75f, 0.51f, 0.29f, 0.0f, 0.49f, 0.5f, 0.55f, 0.16f, 0.69f, 0.08f, 0.2f, 0.76f, 0.29f, 0.76f, 1.0f, 0.46f, 0.8f, 1.0f, 0.39f, 0.79f, 0.27f, 0.0f, 0.68f, 0.5f);
    _programs.emplace_back("Leslie Organ", 0.0f, 0.5f, 0.53f, 0.0f, 0.13f, 0.39f, 0.38f, 0.74f, 0.54f, 0.2f, 0.0f, 0.0f, 0.55f, 0.52f, 0.31f, 0.0f, 0.17f, 0.73f, 0.28f, 0.87f, 0.24f, 0.0f, 0.29f, 0.5f);
    _programs.emplace_back("Click Organ", 0.5f, 0.77f, 0.52f, 0.0f, 0.35f, 0.5f, 0.44f, 0.5f, 0.65f, 0.16f, 0.0f, 0.0f, 0.0f, 0.18f, 0.0f, 0.0f, 0.75f, 0.8f, 0.0f, 0.81f, 0.49f, 0.0f, 0.44f, 0.5f);
    _programs.emplace_back("Hard Organ", 0.89f, 0.91f, 0.37f, 0.0f, 0.35f, 0.5f, 0.51f, 0.62f, 0.54f, 0.0f, 0.0f, 0.0f, 0.37f, 0.0f, 1.0f, 0.04f, 0.08f, 0.72f, 0.04f, 0.77f, 0.49f, 0.0f, 0.58f, 0.5f);
    _programs.emplace_back("Bass Clarinet", 1.0f, 0.51f, 0.51f, 0.37f, 0.0f, 0.5f, 0.51f, 0.1f, 0.5f, 0.11f, 0.5f, 0.0f, 0.0f, 0.0f, 0.25f, 0.35f, 0.65f, 0.65f, 0.32f, 0.79f, 0.49f, 0.2f, 0.35f, 0.5f);
    _programs.emplace_back("Trumpet", 0.0f, 0.51f, 0.51f, 0.82f, 0.06f, 0.5f, 0.57f, 0.0f, 0.32f, 0.15f, 0.5f, 0.21f, 0.15f, 0.0f, 0.25f, 0.24f, 0.6f, 0.8f, 0.1f, 0.75f, 0.55f, 0.25f, 0.69f, 0.5f);
    _programs.emplace_back("Soft Horn", 0.12f, 0.9f, 0.67f, 0.0f, 0.35f, 0.5f, 0.5f, 0.21f, 0.29f, 0.12f, 0.6f, 0.0f, 0.35f, 0.36f, 0.25f, 0.08f, 0.5f, 1.0f, 0.27f, 0.83f, 0.51f, 0.1f, 0.25f, 0.5f);
    _programs.emplace_back("Brass Section", 0.43f, 0.76f, 0.23f, 0.0f, 0.28f, 0.36f, 0.5f, 0.0f, 0.59f, 0.0f, 0.5f, 0.24f, 0.16f, 0.91f, 0.08f, 0.17f, 0.5f, 0.8f, 0.45f, 0.81f, 0.5f, 0.0f, 0.58f, 0.5f);
    _programs.emplace_back("Synth Brass", 0.4f, 0.51f, 0.25f, 0.0f, 0.3f, 0.28f, 0.39f, 0.15f, 0.75f, 0.0f, 0.5f, 0.39f, 0.3f, 0.82f, 0.25f, 0.33f, 0.74f, 0.76f, 0.41f, 0.81f, 0.47f, 0.23f, 0.5f, 0.5f);
    _programs.emplace_back("Detuned Syn Brass [ZF]", 0.68f, 0.5f, 0.93f, 0.0f, 0.31f, 0.62f, 0.26f, 0.07f, 0.85f, 0.0f, 0.66f, 0.0f, 0.83f, 0.0f, 0.05f, 0.0f, 0.75f, 0.54f, 0.32f, 0.76f, 0.37f, 0.29f, 0.56f, 0.5f);
    _programs.emplace_back("Power PWM", 1.0f, 0.27f, 0.22f, 0.0f, 0.35f, 0.5f, 0.82f, 0.13f, 0.75f, 0.0f, 0.0f, 0.24f, 0.3f, 0.88f, 0.34f, 0.0f, 0.5f, 1.0f, 0.48f, 0.71f, 0.37f, 0.0f, 0.35f, 0.5f);
    _programs.emplace_back("Water Velocity [SA]", 0.76f, 0.51f, 0.35f, 0.0f, 0.49f, 0.5f, 0.87f, 0.67f, 1.0f, 0.32f, 0.09f, 0.95f, 0.56f, 0.72f, 1.0f, 0.04f, 0.76f, 0.11f, 0.46f, 0.88f, 0.72f, 0.0f, 0.38f, 0.5f);
    _programs.emplace_back("Ghost [SA]", 0.75f, 0.51f, 0.24f, 0.45f, 0.16f, 0.48f, 0.38f, 0.58f, 0.75f, 0.16f, 0.81f, 0.0f, 0.3f, 0.4f, 0.31f, 0.37f, 0.5f, 1.0f, 0.54f, 0.85f, 0.83f, 0.43f, 0.46f, 0.5f);
    _programs.emplace_back("Soft E.Piano", 0.31f, 0.51f, 0.43f, 0.0f, 0.35f, 0.5f, 0.34f, 0.26f, 0.53f, 0.0f, 0.63f, 0.0f, 0.22f, 0.0f, 0.39f, 0.0f, 0.8f, 0.0f, 0.44f, 0.81f, 0.51f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Thumb Piano", 0.72f, 0.82f, 1.0f, 0.0f, 0.35f, 0.5f, 0.37f, 0.47f, 0.54f, 0.0f, 0.5f, 0.0f, 0.45f, 0.0f, 0.39f, 0.0f, 0.39f, 0.0f, 0.48f, 0.81f, 0.6f, 0.0f, 0.71f, 0.5f);
    _programs.emplace_back("Steel Drums [ZF]", 0.81f, 0.76f, 0.19f, 0.0f, 0.18f, 0.7f, 0.4f, 0.3f, 0.54f, 0.17f, 0.4f, 0.0f, 0.42f, 0.23f, 0.47f, 0.12f, 0.48f, 0.0f, 0.49f, 0.53f, 0.36f, 0.34f, 0.56f, 0.5f);

    _programs.emplace_back("Car Horn", 0.57f, 0.49f, 0.31f, 0.0f, 0.35f, 0.5f, 0.46f, 0.0f, 0.68f, 0.0f, 0.5f, 0.46f, 0.3f, 1.0f, 0.23f, 0.3f, 0.5f, 1.0f, 0.31f, 1.0f, 0.38f, 0.0f, 0.5f, 0.5f);
    _programs.emplace_back("Helicopter", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.08f, 0.36f, 0.69f, 1.0f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.96f, 0.5f, 1.0f, 0.92f, 0.97f, 0.5f, 1.0f, 0.0f, 0.5f);
    _programs.emplace_back("Arctic Wind", 0.0f, 0.25f, 0.5f, 0.0f, 0.35f, 0.5f, 0.16f, 0.85f, 0.5f, 0.28f, 0.5f, 0.37f, 0.3f, 0.0f, 0.25f, 0.89f, 0.5f, 1.0f, 0.89f, 0.24f, 0.5f, 1.0f, 1.0f, 0.5f);
    _programs.emplace_back("Thip", 1.0f, 0.37f, 0.51f, 0.0f, 0.35f, 0.5f, 0.0f, 1.0f, 0.97f, 0.0f, 0.5f, 0.02f, 0.2f, 0.0f, 0.2f, 0.0f, 0.46f, 0.0f, 0.3f, 0.81f, 0.5f, 0.78f, 0.48f, 0.5f);
    _programs.emplace_back("Synth Tom", 0.0f, 0.25f, 0.5f, 0.0f, 0.76f, 0.94f, 0.3f, 0.33f, 0.76f, 0.0f, 0.68f, 0.0f, 0.59f, 0.0f, 0.59f, 0.1f, 0.5f, 0.0f, 0.5f, 0.81f, 0.5f, 0.7f, 0.0f, 0.5f);
    _programs.emplace_back("Squelchy Frog", 0.5f, 0.41f, 0.23f, 0.45f, 0.77f, 0.0f, 0.4f, 0.65f, 0.95f, 0.0f, 0.5f, 0.33f, 0.5f, 0.0f, 0.25f, 0.0f, 0.7f, 0.65f, 0.18f, 0.32f, 1.0f, 0.0f, 0.06f, 0.5f);
}

void JX10AudioProcessor::resetState()
{
    // Turn off all playing voices.
    for (int v = 0; v < NVOICES; ++v) {
        _voices[v].dp1   = 1.0f;
        _voices[v].dp2   = 1.0f;
        _voices[v].saw   = 0.0f;
        _voices[v].p1    = 0.0f;
        _voices[v].p2    = 0.0f;
        _voices[v].env   = 0.0f;
        _voices[v].envd  = 0.0f;
        _voices[v].envl  = 0.0f;
        _voices[v].fenv  = 0.0f;
        _voices[v].fenvd = 0.0f;
        _voices[v].fenvl = 0.0f;
        _voices[v].f0    = 0.0f;
        _voices[v].f1    = 0.0f;
        _voices[v].f2    = 0.0f;
        _voices[v].ff    = 0.0f;
        _voices[v].note  = 0;
    }
    _numActiveVoices = 0;

    // Clear out any pending MIDI events.
    _notes[0] = EVENTS_DONE;

    // These variables are changed by MIDI CC, reset to defaults.
    _volume = 0.0005f;
    _sustain = 0;
    _modWheel = 0.0f;
    _filterCtl = 0.0f;
    _resonanceCtl = 1.0f;
    _pressure = 0.0f;
    _pitchBend = 1.0f;
    _inversePitchBend = 1.0f;

    // Reset other state.
    _lfo = 0.0f;
    _lfoStep = 0;
    _filterZip = 0.0f;
    _lastNote = 0;
    _noiseSeed = 22222;
}

void JX10AudioProcessor::update()
{
    // Oscillator mix. Keep this as a value between 0 and 1.
    _oscMix = apvts.getRawParameterValue("OSC Mix")->load();

    // Detune up or down by max 24 semitones, in steps of 1 semitone.
    float param1 = apvts.getRawParameterValue("OSC Tune")->load();
    float semi = std::floor(48.0f * param1) - 24.0f;

    // Fine-tune by ±50 cents, in steps of 0.1 cent. This parameter is skewed.
    float param2 = apvts.getRawParameterValue("OSC Fine")->load();
    float cent = 15.876f * param2 - 7.938f;
    cent = 0.1f * std::floor(cent * cent * cent);

    /*
      Calculate the multiplication factor for detuning oscillator 2. This is
      the same as 2^(N/12) where N is the number of (fractional) semitones.

      The maximum to detune downwards is 4.12, which is -24 semitones and -50
      cents: 2^(24.5/12). The maximum to detune upwards is 0.242, or +24 semi
      and +50 cents, or 2^(-24.5/12).

      This value will be multiplied with the oscillator period, which is why
      detuning down is greater than 1, as lowering the pitch means the period
      becomes longer. And vice versa for going up in pitch.
     */
    _detune = std::pow(1.059463094359f, -semi - 0.01f * cent);

    // Mono / poly / glide mode. This is an integer value from 0 to 7.
    float param3 = apvts.getRawParameterValue("Mode")->load();
    _mode = int(7.9f * param3);

    // Use a lower update rate for the glide and filter envelope, 32 times
    // (= LFO_MAX) slower than the sample rate.
    const float inverseUpdateRate = _inverseSampleRate * LFO_MAX;

    // Just like the envelope, glide is implemented using a one-pole filter that
    // is updated every 32 samples. Here we set the filter coefficient. A smaller
    // coefficient means the glide takes longer.
    float param4 = apvts.getRawParameterValue("Gld Rate")->load();
    if (param4 < 0.02f) {
        _glideRate = 1.0f;  // no glide
    } else {
        _glideRate = 1.0f - std::exp(-inverseUpdateRate * std::exp(6.0f - 7.0f * param4));
    }

    // Glide bend goes from -36 semitones to +36 semitones. The value is cubed
    // to skew the curve so that you can more easily choose small steps around
    // the center (e.g. steps of 0.01 semitones).
    float param5 = apvts.getRawParameterValue("Gld Bend")->load();
    _glideBend = 6.604f * param5 - 3.302f;
    _glideBend *= _glideBend * _glideBend;

    /*
      Adjusts the low-pass filter's cutoff frequency. The actual cutoff is set
      in noteOn() using the note's pitch and velocity. Inside the audio loop,
      we add up all the modulator values for the filter, including this one, and
      take the exp() to get the final multiplier for the cutoff frequency.

      The range for this parameter is [-1.5, 6.5], so this modulator by itself
      can multiply the cutoff by 0.22x up to 665x (which seems excessive). When
      at 18.75%, this parameter has no effect.

      By default this is 100%, which effectively turns off filter key tracking
      (but not the other modulators if they modulate downwards).
     */
    float param6 = apvts.getRawParameterValue("VCF Freq")->load();
    _filterMultiplier = 8.0f * param6 - 1.5f;

    // Filter Q. Convert into a parabolic curve from 1 down to 0.
    float param7 = apvts.getRawParameterValue("VCF Reso")->load();
    _filterQ = (1.0f - param7) * (1.0f - param7);  // + 0.02f;

    // Filter envelope intensity. Linear curve from -6.0 to +6.0.
    float param8 = apvts.getRawParameterValue("VCF Env")->load();
    _filterEnvDepth = 12.0f * param8 - 6.0f;

    // Filter LFO intensity. Parabolic curve from 0 to 2.5.
    float param9 = apvts.getRawParameterValue("VCF LFO")->load();
    _filterLFODepth = 2.5f * param9 * param9;

    // Filter velocity sensitivity, a value between -0.05 and +0.05.
    // If disabled, the velocity is completely ignored.
    float param10 = apvts.getRawParameterValue("VCF Vel")->load();
    if (param10 < 0.05f) {
        _velocitySensitivity = 0.0f;  // turn off velocity
        _ignoreVelocity = true;
    } else {
        _velocitySensitivity = 0.1f * param10 - 0.05f;
        _ignoreVelocity = false;
    }

    /*
      The envelope is implemented using a simple one-pole filter, which creates
      an analog-style exponential curve. The formulas below calculate the filter
      coefficients for the attack, decay, and release stages.

      The knobs for these parameters let you set the time, but only indirectly.
      For decay and release, they describe how long it takes to drop from full
      level down to -80 dB (SILENCE = 0.0001).

      When set to 0%, the decay time is roughly 37 ms (or -ln(0.0001)/exp(5.5)).
      When set to 100%, it is ~68 seconds (this is -ln(0.0001)/exp(-2.0)).
      These seem like fairly arbitrary values; my guess is that the original
      author just picked a "good enough" curve that was simple to implement.

      The attack coefficient uses the same formula. However, the attack time is
      much shorter than the decay time because the attack curve is sharper.

      The VCF envelope uses the same times as the envelope amplitude but its
      filter is updated at a lower rate (once every 32 samples), which is why
      we use a different sample rate in the exponential.
     */

    float param11 = apvts.getRawParameterValue("VCF Att")->load();
    _filterAttack = 1.0f - std::exp(-inverseUpdateRate * exp(5.5f - 7.5f * param11));

    float param12 = apvts.getRawParameterValue("VCF Dec")->load();
    _filterDecay = 1.0f - std::exp(-inverseUpdateRate * exp(5.5f - 7.5f * param12));

    // The sustain level for the filter envelope is exponential(ish) because
    // frequencies are logarithmic.
    float param13 = apvts.getRawParameterValue("VCF Sus")->load();
    _filterSustain = param13 * param13;

    float param14 = apvts.getRawParameterValue("VCF Rel")->load();
    _filterRelease = 1.0f - std::exp(-inverseUpdateRate * std::exp(5.5f - 7.5f * param14));

    float param15 = apvts.getRawParameterValue("ENV Att")->load();
    _envAttack = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param15));

    float param16 = apvts.getRawParameterValue("ENV Dec")->load();
    _envDecay = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param16));

    float param17 = apvts.getRawParameterValue("ENV Sus")->load();
    _envSustain = param17;

    float param18 = apvts.getRawParameterValue("ENV Rel")->load();
    _envRelease = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param18));
    if (param18 < 0.01f) { _envRelease = 0.1f; }  // extra fast release

    // The LFO rate is an exponentional curve that maps the 0 - 1 parameter value
    // to 0.0183 Hz - 20.086 Hz. We use this to calculate the phase increment for
    // a sine wave running at 1/32th the sample rate.
    float param19 = apvts.getRawParameterValue("LFO Rate")->load();
    float lfoRate = std::exp(7.0f * param19 - 4.0f);
    _lfoInc = lfoRate * inverseUpdateRate * float(TWOPI);

    // The vibrato / PWM setting is a parabolic curve going from 0.0 for 0% up to
    // 0.05 for 100%. You can choose between PWM mode (to the left) and vibrato
    // mode (to the right). These values are used as the amplitude of the LFO sine
    // wave that modulates the oscillator period. In PWM mode, the oscillators are
    // set up to form a pulse wave with a verying duty cycle. Note that to get the
    // PWM effect, the oscMix must be larger than 0.
    float param20 = apvts.getRawParameterValue("Vibrato")->load();
    _vibrato = 0.2f * (param20 - 0.5f) * (param20 - 0.5f);
    _pwmDepth = _vibrato;
    if (param20 < 0.5f) { _vibrato = 0.0f; }

    // How much noise to mix into the signal. This is a parabola from 0 to 1
    // (similar to skew = 2 in JUCE).
    float param21 = apvts.getRawParameterValue("Noise")->load();
    _noiseMix = param21 * param21;

    // When using both oscillators, and/or noise or more filter resonance, the
    // overall gain increases. This variable tries to compensate for that.
    _volumeTrim = (3.2f - _oscMix - 1.5f * _noiseMix) * (1.5f - 0.5f * param7);

    // Lower the noise level so that the maximum is roughly -24 dB.
    _noiseMix *= 0.06f;

    /*
      Master tuning. The octave parameter is ±2 octaves, while tuning is ±100%
      cents. We calculate a multiplier to change the frequency by that amount of
      tuning. This multiplier is 1.0594^semitones because each semitone of tuning
      shifts the pitch up by 2^(1/12) = 1.0594, or down by 2^(-1/12) = 1/1.0594.

      When a note is played, rather than the pitch frequency, we will actually
      calculate its *period* in samples. That's why `_tune` is multiplied by the
      sample rate. The higher the tuning, the smaller `_tune` will become because
      higher notes have smaller periods.

      Why the extra -23.376? Note that if the tuning is 0 octaves and 0 cents,
      `_tune` is actually -48.376. This offset is used to turn MIDI note numbers
      into the correct pitch. More about this in noteOn().
     */
    float param22 = apvts.getRawParameterValue("Octave")->load();
    float param23 = apvts.getRawParameterValue("Tuning")->load();
    _tune = -23.376f - 2.0f * param23 - 12.0f * std::floor(param22 * 4.9f);
    _tune = _sampleRate * std::pow(1.059463094359f, _tune);
}

void JX10AudioProcessor::processEvents(juce::MidiBuffer &midiMessages)
{
    // There are different ways a synth can handle MIDI events. This plug-in does
    // it by copying the events it cares about -- note on and note off -- into an
    // array. In the render loop, we step through this array to process the events
    // one-by-one. Controllers such as the sustain pedal are processed right away,
    // at the start of the block, so these are not sample accurate.

    int npos = 0;
    for (const auto metadata : midiMessages) {
        if (metadata.numBytes != 3) continue;

        const auto data0 = metadata.data[0];
        const auto data1 = metadata.data[1];
        const auto data2 = metadata.data[2];
        const int deltaFrames = metadata.samplePosition;

        switch (data0 & 0xf0) {  // status byte (all channels)
            // Note off
            case 0x80:
                _notes[npos++] = deltaFrames;   // time offset
                _notes[npos++] = data1 & 0x7F;  // note
                _notes[npos++] = 0;             // vel
                break;

            // Note on
            case 0x90:
                _notes[npos++] = deltaFrames;   // time offset
                _notes[npos++] = data1 & 0x7F;  // note
                _notes[npos++] = data2 & 0x7F;  // vel
                break;

            // Controller
            case 0xB0:
                switch (data1) {
                    case 0x01:  // mod wheel
                        // This maps the position of the mod wheel to a
                        // parabolic curve starting at 0.0 (position 0)
                        // up to 0.0806 (position 127). This amount is added
                        // to the LFO intensity for vibrato / PWM.
                        _modWheel = 0.000005f * float(data2 * data2);
                        break;

                    case 0x02:  // filter +
                    case 0x4A:
                    //case 21:  // for testing
                        // Maps the position of the controller from 0 to 2.54.
                        _filterCtl = 0.02f * float(data2);
                        break;

                    case 0x03:  // filter -
                    //case 22:  // for testing
                        // Maps the position of the controller from 0 to -3.81.
                        _filterCtl = -0.03f * float(data2);
                        break;

                    case 0x07:  // volume
                        // Map the position of the volume control to a
                        // parabolic curve starting at 0.0 (position 0)
                        // up to 0.000806 (position 127).
                        _volume = 0.00000005f * float(data2 * data2);
                        break;

                    case 0x10:  // resonance
                    case 0x47:
                    //case 23:  // for testing
                        // This maps the position of the controller to a
                        // linear curve from 1.001 (position 0) down to
                        // 0.1755 (position 127).
                        _resonanceCtl = 0.0065f * float(154 - data2);
                        break;

                    case 0x40:  // sustain pedal
                        // Make the variable 64 when the pedal is pressed
                        // and 0 when released.
                        _sustain = data2 & 0x40;

                        // Pedal released? Then end all sustained notes.
                        // This sends a note-off event with note = -1, meaning
                        // all sustained notes will be moved into their envelope
                        // release stage.
                        if (_sustain == 0) {
                            _notes[npos++] = deltaFrames;
                            _notes[npos++] = SUSTAIN;
                            _notes[npos++] = 0;
                        }
                        break;

                    default:  // all notes off
                        if (data1 > 0x7A) {
                            for (int v = 0; v < NVOICES; ++v) {
                                // Setting the envelope to 0 immediately
                                // turns off the voice.
                                _voices[v].env  = 0.0f;
                                _voices[v].envd = 0.0f;
                                _voices[v].envl = 0.0f;
                                _voices[v].note = 0;

                                // Comment from the original code:
                                // Could probably reset some more stuff here for safety!
                            }
                            _sustain = 0;
                        }
                        break;
                }
                break;

            // Program change
            case 0xC0:
                if (data1 < _programs.size()) {
                    setCurrentProgram(data1);
                }
                break;

            // Channel aftertouch
            case 0xD0:
                // This maps the pressure value to a parabolic curve starting
                // at 0.0 (position 0) up to 0.161 (position 127).
                _pressure = 0.00001f * float(data1 * data1);
                break;

            // Pitch bend
            case 0xE0:
                // This maps the pitch bend value from [-8192, 8191] to an exponential
                // curve from 0.89 to 1.12 and its reciprocal from 1.12 down to 0.89.
                // When the pitch wheel is centered, both values are 1.0. This value
                // is used to multiply the oscillator period, a shift up or down of 2
                // semitones (note: 2^(-2/12) = 0.89 and 2^(2/12) = 1.12).
                _inversePitchBend = std::exp(0.000014102 * double(data1 + 128 * data2 - 8192));
                _pitchBend = 1.0f / _inversePitchBend;
                break;

            default: break;
        }

        // Discard events if buffer full!
        if (npos > EVENTBUFFER) npos -= 3;
    }
    _notes[npos] = EVENTS_DONE;
    midiMessages.clear();
}

void JX10AudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    update();

    processEvents(midiMessages);

    int sampleFrames = buffer.getNumSamples();

    float *out1 = buffer.getWritePointer(0);
    float *out2 = buffer.getWritePointer(1);

    // Q value for the filter. This ranges from 1.0 (no Q) down to 0.0 (full Q).
    const float fq = _filterQ * _resonanceCtl;

    // The SVF filter this synth uses may have stability issues when the cutoff
    // frequency is too high, so we set an upper limit on the cutoff frequency.
    // This also depends on the amount of Q. where more Q means the upper limit
    // is raised, not lowered, as `fq` becomes smaller then.
    const float fx = 1.97f - 0.85f * fq;

    // Calculate the LFO-modulated things. We need to do this at the start of
    // the block, and also do this every 32 samples inside the loop (see below).
    const float sine = std::sin(_lfo);
    float fmod = _filterMultiplier + _filterCtl + (_filterLFODepth + _pressure) * sine;
    float pwm = 1.0f + sine * (_modWheel + _pwmDepth);
    float vib = 1.0f + sine * (_modWheel + _vibrato);

    int event = 0;
    int frame = 0;  // how many samples are already rendered

    // Is there at least one active voice, or any pending MIDI event?
    if (_numActiveVoices > 0 || _notes[event] < sampleFrames) {
        while (frame < sampleFrames) {
            // Get the timestamp of the next note on/off event. This is usually in the
            // future, i.e. a number of samples after the current sample.
            int frames = _notes[event++];

            // This catches the EVENTS_DONE special event. There are no events left.
            if (frames > sampleFrames) frames = sampleFrames;

            // The timestamp for the event is relative to the start of the block.
            // Make it relative to the previous event; this tells us how many samples
            // to process until the new event actually happens.
            frames -= frame;

            // When we're done with this event, this is how many samples we will have
            // processed in total.
            frame += frames;

            // Until it's time to process the upcoming event, render the active voices.
            while (--frames >= 0) {
                Voice *V = _voices;

                // This variable adds up the output values of all the active voices.
                // JX10 is a mono synth, so there is only one channel.
                float o = 0.0f;

                // Generate the next integer pseudorandom number.
                _noiseSeed = _noiseSeed * 196314165 + 907633515;

                // Convert the integer to a float, to get a number between 2 and 4.
                // That's because 32-bit floating point numbers from 2.0 to 4.0 have
                // the hexadecimal values 0x40000000 - 0x407fffff.
                unsigned int r = (_noiseSeed & 0x7FFFFF) + 0x40000000;
                float noise = *(float *)&r;

                // Subtract 3 to get the float into the range [-1, 1], then multiply
                // by the noise level setting.
                noise = _noiseMix * (noise - 3.0f);

                // The LFO and any things it modulates are updated every 32 samples.
                if (--_lfoStep < 0) {
                    _lfo += _lfoInc;
                    if (_lfo > PI) { _lfo -= TWOPI; }

                    // The LFO is a basic sine wave.
                    const float sine = std::sin(_lfo);

                    // The low-pass filter cutoff is modulated by the combination of the
                    // VCF Freq parameter set by the user, the MIDI CC, aftertouch, and
                    // the LFO intensity. This value swings between approx -7.97 and 11.7.
                    // Below, we will also add the filter envelope to this. (The reason
                    // we don't add the envelope here is that we'll be smoothing `fmod`.)
                    fmod = _filterMultiplier + _filterCtl + (_filterLFODepth + _pressure) * sine;

                    // The modulation intensity for vibrato / PWM is set by the parameter
                    // and by the modulation wheel. The `pwm` and `vib` values are used
                    // to directly modulate the oscillator period. They are multipliers
                    // that range between 0.869 and 1.131, so that's a bit more than two
                    // semitones up and down. For some reason, the modulation wheel has a
                    // slightly larger range than the vibrato / PWM intensity parameter.
                    pwm = 1.0f + sine * (_modWheel + _pwmDepth);
                    vib = 1.0f + sine * (_modWheel + _vibrato);

                    _lfoStep = LFO_MAX;  // reset the counter
                }

                // Loop through all the voices...
                for (int v = 0; v < NVOICES; ++v) {
                    // ...but only render the voices that have an active envelope.
                    if (V->env > SILENCE) {

                        // Used by the oscillators.
                        const float hpf = 0.997f;
                        const float min = 1.0f;

                        // Oscillator 1. This creates a sinc pulse every `period*2` samples.
                        // This is why in noteOn() we calculate the half period rather than
                        // the full period corresponding to the note's pitch.
                        float x1 = V->p1 + V->dp1;
                        if (x1 > min) {
                            if (x1 > V->pmax1) {
                                x1 = V->pmax1 + V->pmax1 - x1;
                                V->dp1 = -V->dp1;
                            }
                            V->p1 = x1;

                            // Sine wave approximation.
                            x1 = V->sin01 * V->sinx1 - V->sin11;
                            V->sin11 = V->sin01;
                            V->sin01 = x1;

                            // Sinc function: y = sin(x) / x.
                            x1 = x1 / V->p1;
                        } else {
                            // This is executed the very first time and after every cycle.
                            // Set the period for the next cycle. Even though the period can
                            // be modulated (vibrato, pitch bend, glide), it's only changed
                            // for the next cycle, never in the middle of an ongoing cycle.
                            V->dp1 = V->period * vib * _pitchBend;
                            V->p1 = x1 = -x1;
                            V->pmax1 = std::floor(0.5f + V->dp1) - 0.5f;
                            V->dc1 = -0.5f * V->lev1 / V->pmax1;
                            V->pmax1 *= PI;
                            V->dp1 = V->pmax1 / V->dp1;
                            V->sin01 = V->lev1 * std::sin(x1);
                            V->sin11 = V->lev1 * std::sin(x1 - V->dp1);
                            V->sinx1 = 2.0f * std::cos(V->dp1);

                            // Output the peak of the sinc pulse.
                            if (x1*x1 > 0.1f) {
                                x1 = V->sin01 / x1;
                            } else {
                                x1 = V->lev1;
                            }
                        }

                        // Oscillator 2. This is the same algorithm as for osc 1, except
                        // this uses PWM instead of vibrato, and can be slightly detuned.
                        // Bit of code duplication going on here. This should really be a
                        // method on Voice or a special Oscillator class.
                        float x2 = V->p2 + V->dp2;
                        if (x2 > min) {
                            if (x2 > V->pmax2) {
                                x2 = V->pmax2 + V->pmax2 - x2;
                                V->dp2 = -V->dp2;
                            }
                            V->p2 = x2;
                            x2 = V->sin02 * V->sinx2 - V->sin12;
                            V->sin12 = V->sin02;
                            V->sin02 = x2;
                            x2 = x2 / V->p2;
                        } else {
                            V->dp2 = V->period * V->detune * pwm * _pitchBend;
                            V->p2 = x2 = -x2;
                            V->pmax2 = std::floor(0.5f + V->dp2) - 0.5f;
                            V->dc2 = -0.5f * V->lev2 / V->pmax2;
                            V->pmax2 *= PI;
                            V->dp2 = V->pmax2 / V->dp2;
                            V->sin02 = V->lev2 * std::sin(x2);
                            V->sin12 = V->lev2 * std::sin(x2 - V->dp2);
                            V->sinx2 = 2.0f * std::cos(V->dp2);
                            if (x2*x2 > 0.1f) {
                                x2 = V->sin02 / x2;
                            } else {
                                x2 = V->lev2;
                            }
                        }

                        /*
                          By adding up the sinc pulses over time, i.e. by integrating them,
                          we create a bandlimited saw wave without much aliasing.

                          Oscillator 2 is subtracted. In PWM mode, osc 2 is also flipped
                          (and phase-locked with osc 1) to get a pulse wave.

                          Note: It can be a little unpredictable how these two oscillators
                          interact. The oscillator state is not reset when an old voice is
                          reused for a new note, and so the phase difference between osc 1
                          and 2 is never the same (I guess that's part of the fun).

                          Also, if you don't detune osc 2, it eventually will completely
                          cancel out with osc 1 and you end up with silence.
                         */
                        V->saw = V->saw * hpf + V->dc1 + x1 - V->dc2 - x2;

                        // Combine the output from the oscillators with the noise.
                        float x = V->saw + noise;

                        // Update the amplitude envelope. This is basically a one-pole
                        // filter creating an analog-style exponential envelope curve.
                        // It does the same as: `env = (1 - envd)*env + envd*envl`.
                        V->env += V->envd * (V->envl - V->env);

                        // Do the following updates at the LFO update rate.
                        if (_lfoStep == LFO_MAX) {
                            // Done with the attack portion? Then go into decay. Notice that
                            // envl is 2.0 when the envelope is in the attack stage; that is
                            // how we tell apart the different stages.
                            if (V->env + V->envl > 3.0f) {
                                V->envd = _envDecay;
                                V->envl = _envSustain;
                            }

                            // Update the filter envelope. This is the same equation as for
                            // the amplitude envelope, but only performed every LFO_MAX steps.
                            V->fenv += V->fenvd * (V->fenvl - V->fenv);

                            // Done with the filter attack portion? Then go into decay.
                            if (V->fenv + V->fenvl > 3.0f) {
                                V->fenvd = _filterDecay;
                                V->fenvl = _filterSustain;
                            }

                            // Use a basic one-pole smoothing filter to de-zipper changes to
                            // the amount of filter modulation.
                            _filterZip += 0.005f * (fmod - _filterZip);

                            /*
                              Calculate the filter cutoff. We multiply the base coefficient,
                              `fc`, by the total amount of modulation. This also includes the
                              filter envelope and any pitch bending.

                              We use an exponent because frequencies are logarithmic, and so
                              modulating them works best exponentially. Note that the exp()
                              gives a multiplier with a possible range from 1e-6 to 1e+7,
                              which seems excessive! The pitch bend adds another 2 semitones
                              up or down.

                              The final filter coefficient should be a value between 0.0 and
                              2.0 (= Nyquist), but the filter is only stable up to 1.0 or so
                              (depending on Q). The value of `y` may be larger than 2.0 but
                              we'll limit this before actually applying the filter.
                             */
                            float y = V->fc * std::exp(_filterZip + _filterEnvDepth * V->fenv) * _inversePitchBend;

                            // Don't set the cutoff too low either.
                            if (y < 0.005f) { y = 0.005f; }

                            // For debugging: print out the actual cutoff frequency.
                            //if (y < 2.0f) { printf("cutoff = %f\n", std::asin(y / 2) * getSampleRate() / PI); }

                            V->ff = y;

                            /*
                              Like so many things in this synth, glide between pitches is
                              implemented as an exponential curve using a simple one-pole
                              smoothing filter. If the voice's current period is not yet
                              equal to the target value, this equation brings it a little
                              closer with every update step.

                              We always perform this calculation, even if glide is disabled.
                              In that case, the `_glideRate` is 1, and so the voice's period
                              is immediately set to the target value. (Note that this logic
                              is only performed once every 32 samples, so there could be one
                              or more cycles that get rendered using the old period length).
                             */
                            V->period += _glideRate * (V->target - V->period);
                        }

                        if (V->ff > fx) { V->ff = fx; }  // stability limit

                        // State variable filter for low-pass filtering the sound.
                        // This appears to be a modification of a Chamberlin SVF. I'm not
                        // quite sure where this variation comes from but no doubt it's
                        // done to make the filter behave better at higher frequencies.
                        V->f0 += V->ff * V->f1;
                        V->f1 -= V->ff * (V->f0 + fq * V->f1 - x - V->f2);
                        V->f1 -= 0.2f * V->f1 * V->f1 * V->f1;  // soft limit
                        V->f2 = x;

                        // The output for this voice is the amplitude envelope times the
                        // output from the filter.
                        o += V->env * V->f0;
                    }

                    // Go to the next active voice.
                    V++;
                }

                // Write the result into the output buffer.
                *out1++ = o;
                *out2++ = o;
            }

            // It's time to handle the event. This starts the new note, or stops the
            // note if velocity is 0. Also handles the sustain pedal being lifted.
            if (frame < sampleFrames) {
                int note = _notes[event++];
                int vel  = _notes[event++];
                noteOn(note, vel);
            }
        }

        // Turn off voices whose envelope has dropped below the minimum level.
        _numActiveVoices = NVOICES;
        for (int v = 0; v < NVOICES; ++v) {
            if (_voices[v].env < SILENCE) {
                _voices[v].env = 0.0f;
                _voices[v].envl = 0.0f;
                _voices[v].f0 = 0.0f;
                _voices[v].f1 = 0.0f;
                _voices[v].f2 = 0.0f;
                _voices[v].ff = 0.0f;
                _numActiveVoices--;
            }
        }
    } else {
        // No voices playing and no events, so render an empty block.
        while (--sampleFrames >= 0) {
            *out1++ = 0.0f;
            *out2++ = 0.0f;
        }
    }

    // Mark the events buffer as done.
    _notes[0] = EVENTS_DONE;
}

void JX10AudioProcessor::noteOn(int note, int velocity)
{
    if (velocity > 0) {  // note on
        if (_ignoreVelocity) { velocity = 80; }

        int held = 0;  // how many notes playing that are not released yet
        int v = 0;     // index of the voice to use

        if (_mode & 4) {  // monophonic

            // We get here when in MONO, M-LEGATO, or M-GLIDE mode and the user is
            // playing legato-style, i.e. they pressed a new key before releasing
            // the previous key or keys.
            if (_voices[0].note > 0) {

                // Queue any held notes. This puts the previous note values into the
                // other voices, but it won't actually play these voices. This is used
                // during the next note-off event to determine which note to restore.
                // For example, if you press and hold E, the synth will play the E.
                // Then also press F, now the synth will play an F. When you release
                // the F, the synth sees that E is still held down and it will change
                // the mono voice to play E again, until that is released too.
                for (int tmp = NVOICES - 1; tmp > 0; tmp--) {
                    _voices[tmp].note = _voices[tmp - 1].note;
                }

                // Calculate the oscillator period. These formulas are explained below.
                float p = _tune * std::exp(-0.05776226505f * (float(note) + ANALOG * float(v)));
                while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }
                _voices[v].target = p;

                // Not in M-LEGATO or M-GLIDE? Then no portamento. Otherwise, glide
                // from whatever was the previous period for this voice. Note that this
                // does not use the additional glide bend parameter.
                if ((_mode & 2) == 0) { _voices[v].period = p; }

                // See below for explanations of these.
                _voices[v].fc = std::exp(_velocitySensitivity * float(velocity - 64)) / p;
                _voices[v].env += SILENCE + SILENCE;
                _voices[v].note = note;
                return;
            }

        } else {  // polyphonic
            float l = 100.0f;  // louder than any envelope!

            // Find a voice to use.
            for (int tmp = 0; tmp < NVOICES; tmp++) {
                // Count how many playing voices are for keys that are still held down,
                // i.e. that did not get a note-off event yet. If note is 0, this voice
                // is not playing; if it's -1, the note is sustained by the pedal.
                if (_voices[tmp].note > 0) { held++; }

                // Replace quietest voice not in attack. Recall that envl is set to 2.0
                // for the attack portion of the envelope, but for decay and sustain it
                // is set to the sustain level and for release it is 0; both are < 2.0.
                // This will first use any voices that are not playing (with env = 0.0).
                // If all are in use, steal the voice with the lowest envelope value.
                if (_voices[tmp].env < l && _voices[tmp].envl < 2.0f) {
                    l = _voices[tmp].env;
                    v = tmp;
                }
            }
        }

        /*
          The formula below will calculate the period in samples. For example,
          a 100 Hz tone at 44100 Hz sample rate has a period of 441 samples.

          Note that `p` is actually *half* the period. So, if the tone is 100 Hz,
          `p` is only 220.5 samples instead of 441. This is simply because of how
          the oscillators are implemented: one cycle of the waveform consists of a
          counter counting up for `p` samples and then counting down for another
          `p` samples. The true cycle length is `p*2` samples.

          How this formula works:

          The typical formula for calculating a pitch in Hz is:
                freq = 440 * 2^((note - 69)/12)

          We do (note - 69) because 440 Hz is the pitch for A4, which is note 69.
          If we pull out that factor 2^(-69/12), the formula becomes:
                freq = 8.1758 * 2^(note/12)

          However, we want a period, not a frequency, so we take the reciprocal:
                period = sampleRate / (8.1758 * 2^(note/12))

          But we want half the period, so divide by 2:
                half period = sampleRate / (16.3516 * 2^(note/12))

          We can rewrite this as:
                half period = (sampleRate / 16.3516) * 2^(-note/12)

          Earlier when we calculated `_tune`, we did:
                _tune = sampleRate * 1.0594^(semitones)

          We can fold that 1/16.3516 factor into the number of semitones like so:

                1.0594^(semitones) = 2^(semitones/12) = 1 / 16.3516
                semitones = 12 * log(1/16.3516) / log(2) = -48.376

          So that's where that factor -48.376 comes from. If no additional tuning
          is applied, then:
                _tune = sampleRate * 1.0594^(-48.376)

          When tuning is used, the value of _tune simply becomes smaller or larger
          depending on the number of semitones we need to shift up or down. Tuning
          higher means pitches become higher and so the period becomes smaller.

            The formula for the half period can now be written as:
                half period = _tune * 2^(-note / 12)

          which is the same as:
                half period = _tune * exp(-0.05776 * note)

          It appears kind of complicated but that's because all the math has been
          combined into just a couple of formulas.

          The ANALOG term adds a small amount of detuning based on the current
          voice number. For moar analog!
         */
        float p = _tune * std::exp(-0.05776226505f * (float(note) + ANALOG * float(v)));

        // Make sure the period does not become too small. This lowers the pitch an
        // octave at a time until `p` is at least 3 samples long. It seems likely
        // that this is a requirement of the oscillator algorithm. You can experience
        // this pitch drop for yourself by playing notes in the highest octave.
        while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }

        // Set the period as the target that we'll glide to (if glide enabled).
        _voices[v].target = p;
        _voices[v].detune = _detune;

        // In LEGATO or GLIDE mode, perform a portamento from the previous note's
        // pitch to the new one. The difference between LEGATO and GLIDE is that
        // GLIDE will always perform the portamento, while LEGATO only does it when
        // playing legato-style (at least one previous key is still held down).
        // Note that `_mode & 2` means the mode is 2 or 3 (poly), or 6 or 7 (mono),
        // which are the LEGATO and GLIDE modes. If `_mode & 1`, then the mode is
        // 3 or 7, which is only GLIDE. Note that legato-style playing in MONO mode
        // is handled separately above (regardless of GLIDE or LEGATO modes).
        int noteDistance = 0;
        if ((_mode & 2) && ((_mode & 1) || (held > 0))) {
            noteDistance = note - _lastNote;
        }

        // Make the starting period equal to the period of the previous note (only
        // in LEGATO or GLIDE modes), but also offset it by an additional amount of
        // glide bending (± semitones). Note that `_glideBend` is always used, even
        // if you're not in a LEGATO or GLIDE mode.
        // This again is the familiar formula 1.0594^semitones or 2^(semitones/12).
        _voices[v].period = p * std::pow(1.059463094359f, float(noteDistance) - _glideBend);

        // Make sure the starting period does not become too small. Unlike the
        // target period, this doesn't need to be exact, so we can simply limit
        // it to the minimum of 3 samples.
        if (_voices[v].period < 3.0f) { _voices[v].period = 3.0f; }

        _voices[v].note = _lastNote = note;

        /*
          Set the base cutoff frequency for the low-pass filter, based on the pitch
          of the note and its velocity.

          Note that `fc` is not in Hz but is the actual filter coefficient, which
          is between 0.0 and 2.0 (= Nyquist) for this kind of filter (variation on
          Chamberlin SVF).

          The usual formula for finding the coefficient is 2*sin(PI*freq/sampleRate)
          or the simplified approximation: 2*PI*freq/sampleRate.

          We don't have the pitch for the current note but we do have `p`, half its
          period in samples. Then `freq = sampleRate/(2*p)` and the formula becomes
          `fc = 2*PI*sampleRate/(2*p*sampleRate) = PI / p`.

          That is the pitch part. Since there is no factor PI in the formula used
          in this synth, the cutoff frequency is actually about 3x lower than the
          note's pitch.

          To see this for yourself: Choose a patch with a full sound (has lots of
          harmonics), put VCF Freq at 18%, disable all other filter modulations,
          and set VCF Reso to full resonance. At this setting, VCF Freq does not
          affect the cutoff frequency (i.e. exp(_filterMultiplier) = 1.0).

          Now you can see in a spectrum analyzer that there is a peak at 1/3rd the
          expected pitch (if it's not clear, twiddle VCF Reso to see the peak come
          and go). Because of this "missing" factor PI, you actually need set the
          VCF Freq to 33% to put the cutoff frequency exactly on the note's pitch.

          Of course, there is also the velocity. With velocity sensitivity disabled
          the formula is simply: `fc = 1 / p`.

          The sensitivity can be positive or negative. The note's velocity is also
          made positive or negative, so that soft notes have the opposite effect on
          the frequency than loud notes do.

          The formula `exp(sensitivity * (velocity - 64))` gives a multiplier from
          1/24 up to 24. This suspiciously looks like two octaves up or down -- but
          these are not semitones, and it's more like ±55 semitones.
         */
        _voices[v].fc = std::exp(_velocitySensitivity * float(velocity - 64)) / p;

        // The loudness of the tone uses the MIDI velocity but you cannot set the
        // sensitivity other than on/off. We convert the linear velocity curve into
        // a parabolic curve that goes from 8.9 (velocity = 1) to 137.924 (v = 127).
        float vel = 0.004f * float((velocity + 64) * (velocity + 64)) - 8.0f;

        // Use the different volume controls to set the output level for both
        // oscillators. This is a value between 0 and 1 -- because even though
        // `vel` is large-ish, `_volume` is quite small (0.0005 by default).
        _voices[v].lev1 = _volumeTrim * _volume * vel;
        _voices[v].lev2 = _voices[v].lev1 * _oscMix;

        // In PWM mode, the second oscillator should be flipped around in order to
        // make a square wave from two saw waves.
        float param20 = apvts.getRawParameterValue("Vibrato")->load();
        if (param20 < 0.5f) {
            float p = 0.0f;
            if (_voices[v].dp1 > 0.0f) {
                p = _voices[v].pmax1 + _voices[v].pmax1 - _voices[v].p1;
                _voices[v].dp2 = -_voices[v].dp1;
            } else {
                p = _voices[v].p1;
                _voices[v].dp2 = _voices[v].dp1;
            }
            _voices[v].p2 = _voices[v].pmax2 = p + PI * _voices[v].period;
            _voices[v].dc2 = 0.0f;
            _voices[v].sin02 = 0.0f;
            _voices[v].sin12 = 0.0f;
            _voices[v].sinx2 = 0.0f;
        }

        // Make the envelope greater than SILENCE, otherwise this voice will not be
        // seen as active. The original code called this an "anti-glitching trick"
        // but not sure why. It also had some commented-out code here for resetting
        // the filter state and the filter envelope, but I removed that as it was
        // not used for anything.
        _voices[v].env += SILENCE + SILENCE;

        // Start the attack portion of the envelope. The target level is not 1.0
        // but 2.0 in order to make the attack steeper than a regular exponential
        // curve. The attack ends when the envelope level exceeds 1.0.
        _voices[v].envl  = 2.0f;
        _voices[v].envd  = _envAttack;
        _voices[v].fenvl = 2.0f;
        _voices[v].fenvd = _filterAttack;
    }

    // Note off
    else {
        // In one of the MONO modes and the currently playing note is released?
        if ((_mode & 4) && (_voices[0].note == note)) {

            // Are there any older notes queued? Note that some of these may have
            // been released in the mean time, in which case `voice.note` was set
            // to 0 or SUSTAIN (in the loop from the else clause below). This means
            // notes kept alive only by the sustain pedal are not restored.
            int held = 0;
            int v;
            for (v = NVOICES - 1; v > 0; v--) {
                if (_voices[v].note > 0) { held = v; }
            }

            // Did we find an older note whose key is still held down?
            if (held > 0) {
                // Put this note into voice 0 (note: v is always 0 here).
                _voices[v].note = _voices[held].note;
                _voices[held].note = 0;

                // Calculate the new period based on this note number. These are the
                // same formulas as above.
                float p = _tune * std::exp(-0.05776226505f * (float(_voices[v].note) + ANALOG * float(v)));
                while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }
                _voices[v].target = p;

                // Don't glide unless in M-LEGATO or M-GLIDE modes.
                if ((_mode & 2) == 0) { _voices[v].period = p; }

                // Set the low-pass filter cutoff. This is simpler than before because
                // we do not have the velocity anymore, so we just ignore that part.
                _voices[v].fc = 1.0f / p;
            } else {
                // The last note was released, so turn off the mono voice completely.
                _voices[v].envl  = 0.0f;
                _voices[v].envd  = _envRelease;
                _voices[v].fenvl = 0.0f;
                _voices[v].fenvd = _filterRelease;
                _voices[v].note  = 0;
            }
        } else {
            // We get here in polyphonic mode, or when a key was released that is
            // not currently playing in monophonic mode.

            // We also get here when the sustain pedal is released. In that case,
            // the note number is -1 (SUSTAIN). The sustain pedal does not work in
            // any of the MONO modes, by the way.

            for (int v = 0; v < NVOICES; v++) {
                // Any voices playing this note?
                if (_voices[v].note == note) {
                    // If the sustain pedal is not pressed, then start envelope release.
                    if (_sustain == 0) {
                        _voices[v].envl  = 0.0f;
                        _voices[v].envd  = _envRelease;
                        _voices[v].fenvl = 0.0f;
                        _voices[v].fenvd = _filterRelease;
                        _voices[v].note  = 0;
                    } else {
                        // Sustain pedal is pressed, so put the note in sustain mode.
                        _voices[v].note = SUSTAIN;
                    }
                }
            }
        }
    }
}

juce::AudioProcessorEditor *JX10AudioProcessor::createEditor()
{
    auto editor = new juce::GenericAudioProcessorEditor(*this);
    editor->setSize(500, 1000);
    return editor;
}

void JX10AudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void JX10AudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout JX10AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // The original plug-in, being written for VST2, used parameters from 0 to 1.
    // It would be nicer to change the parameters to more natural ranges, which
    // JUCE allows us to do. For example, the octave setting could be -2 to +2,
    // rather than having to map [0, 1] to [-2, +2]. However, the factory presets
    // are specified as 0 - 1 too and I didn't feel like messing with those.
    // This is why we're keeping the parameters as they were originally.

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OSC Mix", 1),
        "OSC Mix",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    char s[16] = { 0 };
                    sprintf(s, "%4.0f:%2.0f", 100.0 - 50.0f * value, 50.0f * value);
                    return juce::String(s);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OSC Tune", 1),
        "OSC Tune",
        juce::NormalisableRange<float>(),
        0.25f,
        juce::AudioParameterFloatAttributes()
            .withLabel("semi")
            .withStringFromValueFunction(
                [](float value, int) {
                    float semi = std::floor(48.0f * value) - 24.0f;
                    return juce::String(semi, 0);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("OSC Fine", 1),
        "OSC Fine",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("cent")
            .withStringFromValueFunction(
                [](float value, int) {
                    float cent = 15.876f * value - 7.938f;
                    cent = 0.1f * std::floor(cent * cent * cent);
                    return juce::String(cent, 1);
                }
            )));

    // If you're wondering why POLY and MONO appear twice, I think this was done
    // so `mode & 4` means any of the mono modes, and `mode & 2` means any of the
    // glide modes (legato or glide). This would have been nicer with an enum and
    // an AudioParameterChoice.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float value, int) {
                int mode = int(7.9f * value);
                switch (mode) {
                    case  0:
                    case  1: return "POLY";
                    case  2: return "P-LEGATO";
                    case  3: return "P-GLIDE";
                    case  4:
                    case  5: return "MONO";
                    case  6: return "M-LEGATO";
                    default: return "M-GLIDE";
                }
            }
        )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gld Rate", 1),
        "Gld Rate",
        juce::NormalisableRange<float>(),
        0.35f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Gld Bend", 1),
        "Gld Bend",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("semi")
            .withStringFromValueFunction(
                [](float value, int) {
                    float semi = 6.604f * value - 3.302f;
                    semi *= semi * semi;
                    return juce::String(semi, 2);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Freq", 1),
        "VCF Freq",
        juce::NormalisableRange<float>(),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(value * 100.0f, 1);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Reso", 1),
        "VCF Reso",
        juce::NormalisableRange<float>(),
        0.15f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Env", 1),
        "VCF Env",
        juce::NormalisableRange<float>(),
        0.75f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(200.0f * value - 100.0f, 1);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF LFO", 1),
        "VCF LFO",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Vel", 1),
        "VCF Vel",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    if (value < 0.05f)
                        return juce::String("OFF");
                    else
                        return juce::String(int(200.0f * value - 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Att", 1),
        "VCF Att",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Dec", 1),
        "VCF Dec",
        juce::NormalisableRange<float>(),
        0.3f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Sus", 1),
        "VCF Sus",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("VCF Rel", 1),
        "VCF Rel",
        juce::NormalisableRange<float>(),
        0.25f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV Att", 1),
        "ENV Att",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV Dec", 1),
        "ENV Dec",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV Sus", 1),
        "ENV Sus",
        juce::NormalisableRange<float>(),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV Rel", 1),
        "ENV Rel",
        juce::NormalisableRange<float>(),
        0.30f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LFO Rate", 1),
        "LFO Rate",
        juce::NormalisableRange<float>(),
        0.81f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction(
                [](float value, int) {
                    float lfoHz = std::exp(7.0f * value - 4.0f);
                    return juce::String(lfoHz, 3);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Vibrato", 1),
        "Vibrato",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    if (value < 0.5f)
                        return "PWM " + juce::String(100.0f - 200.0f * value, 1);
                    else
                        return juce::String(200.0f * value - 100.0f, 1);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Noise", 1),
        "Noise",
        juce::NormalisableRange<float>(),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(int(value * 100.0f));
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Octave", 1),
        "Octave",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float value, int) {
                return juce::String(int(value * 4.9f) - 2);
            }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Tuning", 1),
        "Tuning",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("cent")
            .withStringFromValueFunction(
                [](float value, int) {
                    return juce::String(200.0f * value - 100.0f, 1);
                }
            )));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new JX10AudioProcessor();
}
