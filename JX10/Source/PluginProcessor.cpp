#include "PluginProcessor.h"


void protectYourEars(float *buffer, int frameCount) {
  #ifdef DEBUG
  bool firstWarning = true;
  #endif
  for (int i = 0; i < frameCount; ++i) {
    float x = buffer[i];
    bool silence = false;
    if (std::isnan(x)) {
      #ifdef DEBUG
      printf("!!! WARNING: nan detected in audio buffer, silencing !!!\n");
      #endif
      silence = true;
    } else if (std::isinf(x)) {
      #ifdef DEBUG
      printf("!!! WARNING: inf detected in audio buffer, silencing !!!\n");
      #endif
      silence = true;
    } else if (x < -2.0f || x > 2.0f) {  // screaming feedback
      #ifdef DEBUG
      printf("!!! WARNING: sample out of range (%f), silencing !!!\n", x);
      #endif
      silence = true;
    } else if (x < -1.0f) {
      #ifdef DEBUG
      if (firstWarning) {
        printf("!!! WARNING: sample out of range (%f), clamping !!!\n", x);
        firstWarning = false;
      }
      #endif
      buffer[i] = -1.0f;
    } else if (x > 1.0f) {
      #ifdef DEBUG
      if (firstWarning) {
        printf("!!! WARNING: sample out of range (%f), clamping !!!\n", x);
        firstWarning = false;
      }
      #endif
      buffer[i] = 1.0f;
    }
    if (silence) {
      memset(buffer, 0, frameCount * sizeof(float));
      return;
    }
  }
}



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

  apvts.state.addListener(this);
}

JX10AudioProcessor::~JX10AudioProcessor()
{
  apvts.state.removeListener(this);
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
  _parametersChanged.store(true);
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
    _voices[v].dp   = _voices[v].dp2   = 1.0f;
    _voices[v].saw  = _voices[v].p     = _voices[v].p2    = 0.0f;
    _voices[v].env  = _voices[v].envd  = _voices[v].envl  = 0.0f;
    _voices[v].fenv = _voices[v].fenvd = _voices[v].fenvl = 0.0f;
    _voices[v].f0   = _voices[v].f1    = _voices[v].f2    = 0.0f;
    _voices[v].ff   = 0.0f;
    _voices[v].note = 0;
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

  // Reset the rest.
  _lfo = 0.0f;
  fzip = 0.0f;
  K = 0;
  lastnote = 0;
  _noiseSeed = 22222;
}

void JX10AudioProcessor::update()
{
  // Oscillator mix.
  float param0 = apvts.getRawParameterValue("OSC Mix")->load();
  _oscMix = param0;
//printf("_oscMix %f\n", _oscMix);

  // Tune oscillator up or down by up to 24 semitones.
  float param1 = apvts.getRawParameterValue("OSC Tune")->load();
  float semi = std::floor(48.0f * param1) - 24.0f;

  // Fine-tune by ±50 cents, in steps of 0.1 cent. This parameter is skewed.
  float param2 = apvts.getRawParameterValue("OSC Fine")->load();
  float cent = 15.876f * param2 - 7.938f;
  cent = 0.1f * std::floor(cent * cent * cent);

  // Calculate the multiplication factor for detuning the pitch.
  // This is the same as 2^(N/12) where N is the number of semitones.
  _detune = std::pow(1.059463094359f, -semi - 0.01f * cent);
//printf("_detune %f\n", _detune);

  // Mono / poly / glide mode. This is an integer value from 0 to 7.
  float param3 = apvts.getRawParameterValue("Mode")->load();
  _mode = int(7.9f * param3);
//printf("_mode %d\n", _mode);

  // Use a lower update rate for the glide and filter envelope.
  const float ifs = _inverseSampleRate * KMAX;
//printf("ifs %f\n", ifs);

  float param4 = apvts.getRawParameterValue("Gld Rate")->load();
  if (param4 < 0.02f) {
    _glide = 1.0f;
  } else {
    _glide = 1.0f - std::exp(-ifs * std::exp(6.0f - 7.0f * param4));
  }
//printf("_glide %f\n", _glide);

  float param5 = apvts.getRawParameterValue("Gld Bend")->load();
  _glidedisp = 6.604f * param5 - 3.302f;
  _glidedisp *= _glidedisp * _glidedisp;
//printf("_glidedisp %f\n", _glidedisp);

  // Filter frequency. Convert linearly from [0, 1] to [-1.5, 6.5].
  float param6 = apvts.getRawParameterValue("VCF Freq")->load();
  _filterCutoff = 8.0f * param6 - 1.5f;
//printf("_filterCutoff %f\n", _filterCutoff);

  // Filter Q. Convert into a parabolic curve from 1 down to 0.
  float param7 = apvts.getRawParameterValue("VCF Reso")->load();
  _filterQ = (1.0f - param7) * (1.0f - param7);  // + 0.02f;
//printf("_filterQ %f\n", _filterQ);

  // Filter envelope intensity. Curve from -6.0 to +6.0.
  float param8 = apvts.getRawParameterValue("VCF Env")->load();
  _filterEnv = 12.0f * param8 - 6.0f;
//printf("_filterEnv %f\n", _filterEnv);

  // Filter LFO intensity. Parabolic curve from 0 to 2.5.
  float param9 = apvts.getRawParameterValue("VCF LFO")->load();
  _filterLFO = 2.5f * param9 * param9;
//printf("_filterLFO %f\n", _filterLFO);

  // Filter velocity sensitivity. Value between -0.05 and +0.05.
  // If disabled, we completely ignore the velocity.
  float param10 = apvts.getRawParameterValue("VCF Vel")->load();
  if (param10 < 0.05f) {
    _filterVelocitySensitivity = 0.0f;
    _ignoreVelocity = true;
  } else {
    _filterVelocitySensitivity = 0.1f * param10 - 0.05f;
    _ignoreVelocity = false;
  }
//printf("_filterVelocitySensitivity %f\n", _filterVelocitySensitivity);
//printf("_ignoreVelocity %d\n", _ignoreVelocity);

//TODO: explain these curves (maybe just render the envelope shape?)
  float param11 = apvts.getRawParameterValue("VCF Att")->load();
  _filterAttack = 1.0f - std::exp(-ifs * exp(5.5f - 7.5f * param11));
//printf("_filterAttack %f\n", _filterAttack);

  float param12 = apvts.getRawParameterValue("VCF Dec")->load();
  _filterDecay = 1.0f - std::exp(-ifs * exp(5.5f - 7.5f * param12));
//printf("_filterDecay %f\n", _filterDecay);

  float param13 = apvts.getRawParameterValue("VCF Sus")->load();
  _filterSustain = param13 * param13;
//printf("_filterSustain %f\n", _filterSustain);

  float param14 = apvts.getRawParameterValue("VCF Rel")->load();
  _filterRelease = 1.0f - std::exp(-ifs * std::exp(5.5f - 7.5f * param14));
//printf("_filterRelease %f\n", _filterRelease);

  float param15 = apvts.getRawParameterValue("ENV Att")->load();
  _envAttack = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param15));
//printf("_envAttack %f\n", _envAttack);

  float param16 = apvts.getRawParameterValue("ENV Dec")->load();
  _envDecay = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param16));
//printf("_envDecay %f\n", _envDecay);

  float param17 = apvts.getRawParameterValue("ENV Sus")->load();
  _envSustain = param17;
//printf("_envSustain %f\n", _envSustain);

  float param18 = apvts.getRawParameterValue("ENV Rel")->load();
  _envRelease = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.5f - 7.5f * param18));
  if (param18 < 0.01f) { _envRelease = 0.1f; }  // extra fast release
//printf("_envRelease %f\n", _envRelease);

  float param19 = apvts.getRawParameterValue("LFO Rate")->load();
  float lfoRate = std::exp(7.0f * param19 - 4.0f);
  _lfoInc = lfoRate * _inverseSampleRate * float(TWOPI * KMAX);  // TODO: ifs * TWOPI?
//printf("_lfoInc %f\n", _lfoInc);

  float param20 = apvts.getRawParameterValue("Vibrato")->load();
  _vibrato = 0.2f * (param20 - 0.5f) * (param20 - 0.5f);
  _pwmdep = _vibrato;
  if (param20 < 0.5f) { _vibrato = 0.0f; }
//printf("_vibrato %f\n", _vibrato);
//printf("_pwmdep %f\n", _pwmdep);

  // Noise level. This is a parabola from 0 to 1 (similar to skew = 2 in JUCE).
  float param21 = apvts.getRawParameterValue("Noise")->load();
  _noiseMix = param21 * param21;

  _volumeTrim = (3.2f - param0 - 1.5f * _noiseMix) * (1.5f - 0.5f * param7);

  // Lower the noise level so that the maximum is roughly -24 dB.
  _noiseMix *= 0.06f;
//printf("voltrim %f\n", voltrim);
//printf("_noiseMix %f\n", _noiseMix);

  // Master tuning. This does 2^(N/12) where N is the number of semitones.
  // The octave parameter is ±2 octaves, while tuning is ±100%.
// TODO: what is the -23.376f for?
// -48.375999 center, so that's 4 octaves lower? but the FFT analyzer does show the correct pitch, so figure out why
  float param22 = apvts.getRawParameterValue("Octave")->load();
  float param23 = apvts.getRawParameterValue("Tuning")->load();
  _tune = -23.376f - 2.0f * param23 - 12.0f * std::floor(param22 * 4.9f);
  _tune = _sampleRate * std::pow(1.059463094359f, _tune);
//printf("_tune %f\n", _tune);
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
            // This maps the position of the mod wheel to a parabolic curve
            // starting at 0 (position 0) up to 0.0806 (position 127).
            _modWheel = 0.000005f * float(data2 * data2);
            break;

          case 0x02:  // filter +
          case 0x4A:
          case 21:    // just for testing
            // Maps the position of the controller from 0 to 2.54.
            _filterCtl = 0.02f * float(data2);
            break;

          case 0x03:  // filter -
          case 22:    // just for testing
            // Maps the position of the controller from 0 to -3.81.
            _filterCtl = -0.03f * float(data2);
            break;

          case 0x07:  // volume
            // Map the position of the volume control to a parabolic curve
            // starting at 0.0 (position 0) up to 0.000806 (position 127).
            _volume = 0.00000005f * float(data2 * data2);
            break;

          case 0x10:  // resonance
          case 0x47:
          case 23:    // just for testing
            // This maps the position of the controller to a linear curve
            // from 1.001 (position 0) down to 0.1755 (position 127).
            _resonanceCtl = 0.0065f * float(154 - data2);
            break;

          case 0x40:  // sustain pedal
            // Make the variable 64 when the pedal is pressed and 0 when released.
            _sustain = data2 & 0x40;

            // Pedal released? Then end all sustained notes.
            if (_sustain == 0) {
              _notes[npos++] = deltaFrames;
              _notes[npos++] = SUSTAIN;
              _notes[npos++] = 0;
            }
            break;

          default:  // all notes off
            if (data1 > 0x7A) {
              for (int v = 0; v < NVOICES; ++v) {
                _voices[v].envl = 0.0f;
                _voices[v].env = 0.0f;

                // Setting the decay to 0.99 will fade out the voice very quickly.
                _voices[v].envd = 0.99f;
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
        // This maps the pressure value to a parabolic curve starting at 0
        // (position 0) up to 0.161 (position 127).
        _pressure = 0.00001f * float(data1 * data1);
        break;

      // Pitch bend
      case 0xE0:
        // This maps the pitch bend value from [-8192, 8191] to an exponential
        // curve from 0.89 to 1.12 and its reciprocal (from 1.12 down to 0.89).
        // When the pitch wheel is centered, both values are 1.0.
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

  // Only recalculate when a parameter has changed.
  bool expected = true;
  if (_parametersChanged.compare_exchange_strong(expected, false)) {
    update();
  }

  processEvents(midiMessages);

  int sampleFrames = buffer.getNumSamples();

  float *out1 = buffer.getWritePointer(0);
  float *out2 = buffer.getWritePointer(1);

  int event = 0;
  int frame = 0;  // how many samples are already rendered

  const float hpf = 0.997f;
  const float min = 1.0f;
  const float fq = _filterQ * _resonanceCtl;
  const float fx = 1.97f - 0.85f * fq;
  float fz = fzip;
  int k = K;

  float vib = std::sin(_lfo);
  float ff = _filterCutoff + _filterCtl + (_filterLFO + _pressure) * vib; //have to do again here as way that
  float pwm = 1.0f + vib * (_modWheel + _pwmdep);           //below triggers on k was too cheap!
  vib = 1.0f + vib * (_modWheel + _vibrato);

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

        // === Noise generator ===

        // Generate the next integer pseudorandom number.
        _noiseSeed = _noiseSeed * 196314165 + 907633515;

        // Convert this to a float. This gives a number between 2 and 4.
        // That's because 32-bit floating point numbers from 2.0 to 4.0 have
        // the hexadecimal form 0x40000000 - 0x407fffff.
        unsigned int r = (_noiseSeed & 0x7FFFFF) + 0x40000000;
        float noise = *(float *)&r;

        // Subtract 3 to get the float into the range [-1, 1], then multiply
        // by the noise level setting.
        noise = _noiseMix * (noise - 3.0f);

        // === TODO ===

        if (--k < 0) {
          _lfo += _lfoInc;
          if (_lfo > PI) { _lfo -= TWOPI; }
          vib = std::sin(_lfo);
          ff = _filterCutoff + _filterCtl + (_filterLFO + _pressure) * vib;
          pwm = 1.0f + vib * (_modWheel + _pwmdep);
          vib = 1.0f + vib * (_modWheel + _vibrato);
          k = KMAX;
        }

        for (int v = 0; v < NVOICES; ++v) {  // for each voice
          float e = V->env;
          if (e > SILENCE) {  // Sinc-Loop Oscillator
            float x = V->p + V->dp;
            if (x > min) {
              if (x > V->pmax) {
                x = V->pmax + V->pmax - x;
                V->dp = -V->dp;
              }
              V->p = x;
              x = V->sin0 * V->sinx - V->sin1;  // sine osc
              V->sin1 = V->sin0;
              V->sin0 = x;
              x = x / V->p;
            } else {
              V->p = x = - x;
              V->dp = V->period * vib * _pitchBend; //set period for next cycle
              V->pmax = std::floor(0.5f + V->dp) - 0.5f;
              V->dc = -0.5f * V->lev / V->pmax;
              V->pmax *= PI;
              V->dp = V->pmax / V->dp;
              V->sin0 = V->lev * std::sin(x);
              V->sin1 = V->lev * std::sin(x - V->dp);
              V->sinx = 2.0f * std::cos(V->dp);
              if (x*x > 0.1f) {
                x = V->sin0 / x;
              } else {
                x = V->lev;
              }
            }

            float y = V->p2 + V->dp2;  // osc2
            if (y > min) {
              if (y > V->pmax2) {
                y = V->pmax2 + V->pmax2 - y;
                V->dp2 = -V->dp2;
              }
              V->p2 = y;
              y = V->sin02 * V->sinx2 - V->sin12;
              V->sin12 = V->sin02;
              V->sin02 = y;
              y = y / V->p2;
            } else {
              V->p2 = y = - y;
              V->dp2 = V->period * V->detune * pwm * _pitchBend;
              V->pmax2 = std::floor(0.5f + V->dp2) - 0.5f;
              V->dc2 = -0.5f * V->lev2 / V->pmax2;
              V->pmax2 *= PI;
              V->dp2 = V->pmax2 / V->dp2;
              V->sin02 = V->lev2 * std::sin(y);
              V->sin12 = V->lev2 * std::sin(y - V->dp2);
              V->sinx2 = 2.0f * std::cos(V->dp2);
              if (y*y > 0.1f) {
                y = V->sin02 / y;
              } else {
                y = V->lev2;
              }
            }

            V->saw = V->saw * hpf + V->dc + x - V->dc2 - y;  // integrated sinc = saw
            x = V->saw + noise;
            V->env += V->envd * (V->envl - V->env);

            if (k == KMAX) {  // filter freq update at LFO rate
              if ((V->env + V->envl) > 3.0f) {  // envelopes
                V->envd = _envDecay;
                V->envl = _envSustain;
              }
              V->fenv += V->fenvd * (V->fenvl - V->fenv);
              if ((V->fenv + V->fenvl) > 3.0f) {
                V->fenvd = _filterDecay;
                V->fenvl = _filterSustain;
              }

              fz += 0.005f * (ff - fz);  // filter zipper noise filter
              y = V->fc * std::exp(fz + _filterEnv * V->fenv) * _inversePitchBend;  // filter cutoff
              if (y < 0.005f) { y = 0.005f; }
              V->ff = y;

              V->period += _glide * (V->target - V->period); // glide
              if (V->target < V->period) {
                V->period += _glide * (V->target - V->period);
              }
            }

            if (V->ff > fx) { V->ff = fx; }  // stability limit

            V->f0 += V->ff * V->f1;  // state-variable filter
            V->f1 -= V->ff * (V->f0 + fq * V->f1 - x - V->f2);
            V->f1 -= 0.2f * V->f1 * V->f1 * V->f1;  // soft limit

            V->f2 = x;

            o += V->env * V->f0;
          }

          // Go to the next active voice.
          V++;
        }

//if (std::isnan(o) || std::isinf(o)) {
//  printf("woots\n");
//}

        // Write the result into the output buffer.
        *out1++ = o;
        *out2++ = o;
      }

      // It's time to handle the event. This starts the new note, or stops the
      // note if velocity is 0.
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
        _voices[v].env = _voices[v].envl = 0.0f;
        _voices[v].f0 = _voices[v].f1 = _voices[v].f2 = 0.0f;
        _voices[v].ff = 0.0f;
        _numActiveVoices--;
      }
    }
  } else {
    // No voices playing and no events, so render empty block.
    while (--sampleFrames >= 0) {
      *out1++ = 0.0f;
      *out2++ = 0.0f;
    }
  }

  // Mark the events buffer as done.
  _notes[0] = EVENTS_DONE;

  fzip = fz;
  K = k;

  protectYourEars(out1, buffer.getNumSamples());
  protectYourEars(out2, buffer.getNumSamples());
}

void JX10AudioProcessor::noteOn(int note, int velocity)
{
  float l = 100.0f;  // louder than any envelope!
  int held = 0;

  if (velocity > 0) {  // note on
    if (_ignoreVelocity) { velocity = 80; }

    int v = 0;

    if (_mode & 4) {  // monophonic
      if (_voices[0].note > 0) {  // legato pitch change
        for (int tmp = NVOICES - 1; tmp > 0; tmp--) {  // queue any held notes
          _voices[tmp].note = _voices[tmp - 1].note;
        }
        float p = _tune * std::exp(-0.05776226505f * (float(note) + ANALOG * float(v)));
        while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }
        _voices[v].target = p;
        if ((_mode & 2) == 0) { _voices[v].period = p; }
        _voices[v].fc = std::exp(_filterVelocitySensitivity * float(velocity - 64)) / p;
        _voices[v].env += SILENCE + SILENCE;  // was missed out below if returned?
        _voices[v].note = note;
        return;
      }
    } else {  // polyphonic
      for (int tmp = 0; tmp < NVOICES; tmp++) {  // replace quietest voice not in attack
        if (_voices[tmp].note > 0) { held++; }
        if (_voices[tmp].env < l && _voices[tmp].envl < 2.0f) {
          l = _voices[tmp].env;
          v = tmp;
        }
      }
    }
    float p = _tune * std::exp(-0.05776226505f * (float(note) + ANALOG * float(v)));
    while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }
    _voices[v].target = p;
    _voices[v].detune = _detune;

    int tmp = 0;
    if (_mode & 2) {
      if ((_mode & 1) || held) { tmp = note - lastnote; }  // glide
    }
    _voices[v].period = p * std::pow(1.059463094359f, float(tmp) - _glidedisp);
    if (_voices[v].period < 3.0f) { _voices[v].period = 3.0f; }  // limit min period

    _voices[v].note = lastnote = note;

    _voices[v].fc = std::exp(_filterVelocitySensitivity * float(velocity - 64)) / p;  // filter tracking

    _voices[v].lev = _volumeTrim * _volume * (0.004f * float((velocity + 64) * (velocity + 64)) - 8.0f);
    _voices[v].lev2 = _voices[v].lev * _oscMix;

    float param20 = apvts.getRawParameterValue("Vibrato")->load();
    if (param20 < 0.5f) {  // force 180 deg phase difference for PWM
      if (_voices[v].dp > 0.0f) {
//TODO: he reuses p here which I don't like
        p = _voices[v].pmax + _voices[v].pmax - _voices[v].p;
        _voices[v].dp2 = -_voices[v].dp;
      } else {
        p = _voices[v].p;
        _voices[v].dp2 = _voices[v].dp;
      }
      _voices[v].p2 = _voices[v].pmax2 = p + PI * _voices[v].period;
      _voices[v].dc2 = 0.0f;
      _voices[v].sin02 = _voices[v].sin12 = _voices[v].sinx2 = 0.0f;
    }

    if (_mode & 4) { //monophonic retriggering
      _voices[v].env += SILENCE + SILENCE;
    } else {
      //if(programs[curProgram].param[15] < 0.28f)
      //{
      //  _voices[v].f0 = voice[v].f1 = voice[v].f2 = 0.0f; //reset filter
      //  voice[v].env = SILENCE + SILENCE;
      //  voice[v].fenv = 0.0f;
      //}
      //else
      _voices[v].env += SILENCE + SILENCE;  // anti-glitching trick
    }
    _voices[v].envl  = 2.0f;
    _voices[v].envd  = _envAttack;
    _voices[v].fenvl = 2.0f;
    _voices[v].fenvd = _filterAttack;
  }

  // Note off
  else {
    if ((_mode & 4) && (_voices[0].note == note)) {  // monophonic (and current note)
      int v;
      for (v = NVOICES - 1; v > 0; v--) {
        if (_voices[v].note > 0) { held = v; }  // any other notes queued?
      }
//TODO: does it make sense that we use voices[v] here? isn't that always 0?
      if (held > 0) {
        _voices[v].note = _voices[held].note;
        _voices[held].note = 0;

        float p = _tune * std::exp(-0.05776226505f * (float(_voices[v].note) + ANALOG * float(v)));
        while (p < 3.0f || (p * _detune) < 3.0f) { p += p; }
        _voices[v].target = p;
        if ((_mode & 2) == 0) { _voices[v].period = p; }
        _voices[v].fc = 1.0f / p;
      } else {
        _voices[v].envl  = 0.0f;
        _voices[v].envd  = _envRelease;
        _voices[v].fenvl = 0.0f;
        _voices[v].fenvd = _filterRelease;
        _voices[v].note  = 0;
      }
    } else {  // polyphonic
      for (int v = 0; v < NVOICES; v++) {
        if (_voices[v].note == note) {  // any voices playing that note?
          if (_sustain == 0) {
            _voices[v].envl  = 0.0f;
            _voices[v].envd  = _envRelease;
            _voices[v].fenvl = 0.0f;
            _voices[v].fenvd = _filterRelease;
            _voices[v].note  = 0;
          } else {
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
    _parametersChanged.store(true);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout JX10AudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // The original plug-in, being written for VST2, used parameters from 0 to 1.
  // It would be nicer to change the parameters to more natural ranges, which
  // JUCE allows us to do. For example, the octave setting could be -2 to +2,
  // rather than having to map [0, 1] to [-2, +2]. However, the factory presets
  // are specified as 0-1 too and I didn't feel like messing with those. This
  // is why we're keeping the parameters as they were originally.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "OSC Mix",
    "OSC Mix",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      char s[16] = { 0 };
      sprintf(s, "%4.0f:%2.0f", 100.0 - 50.0f * value, 50.0f * value);
      return juce::String(s);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "OSC Tune",
    "OSC Tune",
    juce::NormalisableRange<float>(),
    0.25f,
    "semi",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      float semi = std::floor(48.0f * value) - 24.0f;
      return juce::String(semi, 0);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "OSC Fine",
    "OSC Fine",
    juce::NormalisableRange<float>(),
    0.5f,
    "cent",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      float cent = 15.876f * value - 7.938f;
      cent = 0.1f * std::floor(cent * cent * cent);
      return juce::String(cent, 1);
    }));

  // If you're wondering why POLY and MONO appear twice, I think this was done
  // so `mode & 4` means any of the mono modes, and `mode & 2` means any of the
  // glide modes (legato or glide). This would have been nicer with an enum and
  // an AudioParameterChoice.
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mode",
    "Mode",
    juce::NormalisableRange<float>(),
    0.0f,
    "",
    juce::AudioProcessorParameter::genericParameter,
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
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Gld Rate",
    "Gld Rate",
    juce::NormalisableRange<float>(),
    0.35f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Gld Bend",
    "Gld Bend",
    juce::NormalisableRange<float>(),
    0.5f,
    "semi",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      float glidedisp = 6.604f * value - 3.302f;
      glidedisp *= glidedisp * glidedisp;
      return juce::String(glidedisp, 2);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Freq",
    "VCF Freq",
    juce::NormalisableRange<float>(),
    1.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(value * 100.0f, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Reso",
    "VCF Reso",
    juce::NormalisableRange<float>(),
    0.15f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Env",
    "VCF Env",
    juce::NormalisableRange<float>(),
    0.75f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(200.0f * value - 100.0f, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF LFO",
    "VCF LFO",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Vel",
    "VCF Vel",
    juce::NormalisableRange<float>(),
    0.5f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      if (value < 0.05f)
        return juce::String("OFF");
      else
        return juce::String(int(200.0f * value - 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Att",
    "VCF Att",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Dec",
    "VCF Dec",
    juce::NormalisableRange<float>(),
    0.3f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Sus",
    "VCF Sus",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "VCF Rel",
    "VCF Rel",
    juce::NormalisableRange<float>(),
    0.25f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "ENV Att",
    "ENV Att",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "ENV Dec",
    "ENV Dec",
    juce::NormalisableRange<float>(),
    0.5f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "ENV Sus",
    "ENV Sus",
    juce::NormalisableRange<float>(),
    1.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "ENV Rel",
    "ENV Rel",
    juce::NormalisableRange<float>(),
    0.30f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "LFO Rate",
    "LFO Rate",
    juce::NormalisableRange<float>(),
    0.81f,
    "Hz",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      float lfoHz = std::exp(7.0f * value - 4.0f);
      return juce::String(lfoHz, 3);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Vibrato",
    "Vibrato",
    juce::NormalisableRange<float>(),
    0.5f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      if (value < 0.5f)
        return "PWM " + juce::String(100.0f - 200.0f * value, 1);
      else
        return juce::String(200.0f * value - 100.0f, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Noise",
    "Noise",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Octave",
    "Octave",
    juce::NormalisableRange<float>(),
    0.5f,
    "",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 4.9f) - 2);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Tuning",
    "Tuning",
    juce::NormalisableRange<float>(),
    0.5f,
    "cent",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(200.0f * value - 100.0f, 1);
    }));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new JX10AudioProcessor();
}
