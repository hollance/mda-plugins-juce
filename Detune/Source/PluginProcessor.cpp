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



MDADetuneAudioProcessor::MDADetuneAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDADetuneAudioProcessor::~MDADetuneAudioProcessor()
{
}

const juce::String MDADetuneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDADetuneAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDADetuneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDADetuneAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDADetuneAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDADetuneAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDADetuneAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = float(newSampleRate);
    resetState();
}

void MDADetuneAudioProcessor::releaseResources()
{
}

void MDADetuneAudioProcessor::reset()
{
    resetState();
}

bool MDADetuneAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDADetuneAudioProcessor::resetState()
{
    std::memset(buf, 0, sizeof(buf));
    std::memset(win, 0, sizeof(win));
    pos0 = 0;
    pos1 = pos2 = 0.0f;
}

void MDADetuneAudioProcessor::update()
{
    float param0 = apvts.getRawParameterValue("Detune")->load();
    semi = 3.0f * param0 * param0 * param0;
    dpos2 = std::pow(1.0594631f, semi);  // 2^N/12?
    dpos1 = 1.0f / dpos2;   // TODO: is downward?

    // Output gain is -20 to +20 dB
    float param2 = apvts.getRawParameterValue("Output")->load();
    float gain = juce::Decibels::decibelsToGain(param2);

//TODO: describe these curves
    float param1 = apvts.getRawParameterValue("Mix")->load();
    dry = gain - gain * param1 * param1;
    wet = (gain + gain - gain * param1) * param1;

    float param3 = apvts.getRawParameterValue("Latency")->load();
    int tmp = 1 << (8 + int(4.9f * param3));

    if (tmp != buflen) {  //recalculate crossfade window
        buflen = tmp;
        if (buflen > BUFMAX) { buflen = BUFMAX; }
        bufres = 1000.0f * float(buflen) / sampleRate;

        //hanning half-overlap-and-add
        double p = 0.0, dp = 6.28318530718/buflen;
        for (int i = 0; i < buflen; i++) {
            win[i] = float(0.5 - 0.5 * std::cos(p));
            p += dp;
        }
    }
}

void MDADetuneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    int l = buflen - 1;
    int lh = buflen >> 1;
    float lf = float(buflen);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        float c = dry * a;
        float d = dry * b;

        --pos0 &= l;
        buf[pos0] = wet * (a + b);      //input

        pos1 -= dpos1;
        if (pos1 < 0.0f) { pos1 += lf; }          //output
        int p1i = int(pos1);
        float p1f = pos1 - float(p1i);
        a = buf[p1i];
        ++p1i &= l;
        a += p1f * (buf[p1i] - a);  //linear interpolation

        int p2i = (p1i + lh) & l;           //180-degree ouptut
        b = buf[p2i];
        ++p2i &= l;
        b += p1f * (buf[p2i] - b);  //linear interpolation

        p2i = (p1i - pos0) & l;           //crossfade
        float x = win[p2i];
        c += b + x * (a - b);

        pos2 -= dpos2;  //repeat for downwards shift - can't see a more efficient way?
        if (pos2 < 0.0f) { pos2 += lf; }           //output
        p1i = int(pos2);
        p1f = pos2 - float(p1i);
        a = buf[p1i];
        ++p1i &= l;
        a += p1f * (buf[p1i] - a);  //linear interpolation

        p2i = (p1i + lh) & l;           //180-degree ouptut
        b = buf[p2i];
        ++p2i &= l;
        b += p1f * (buf[p2i] - b);  //linear interpolation

        p2i = (p1i - pos0) & l;           //crossfade
        x = win[p2i];
        d += b + x * (a - b);

        out1[i] = c;
        out2[i] = d;
    }

    protectYourEars(out1, buffer.getNumSamples());
    protectYourEars(out2, buffer.getNumSamples());
}

juce::AudioProcessorEditor *MDADetuneAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDADetuneAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDADetuneAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDADetuneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Detune", 1),
        "Detune",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.2f,
        juce::AudioParameterFloatAttributes()
            .withLabel("cents")
            .withStringFromValueFunction([](float value, int)
            {
                float semi = 3.0f * value * value * value;
                return juce::String(100.0f * semi, 1);
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Mix", 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.9f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float value, int)
            {
                return juce::String(int(99.0f * value));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Latency", 1),
        "Latency",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([this](float value, int)
            {
                int buflen = 1 << (8 + int(4.9f * value));
                if (buflen > BUFMAX) { buflen = BUFMAX; }
                float bufres = 1000.0f * float(buflen) / sampleRate;
                return juce::String(bufres, 1);
            })));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDADetuneAudioProcessor();
}
