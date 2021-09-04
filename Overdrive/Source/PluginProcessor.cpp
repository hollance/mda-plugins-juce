#include "PluginProcessor.h"

MDAOverdriveAudioProcessor::MDAOverdriveAudioProcessor()
  : AudioProcessor(BusesProperties()
                   .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                   .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  apvts.state.addListener(this);
}

MDAOverdriveAudioProcessor::~MDAOverdriveAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDAOverdriveAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDAOverdriveAudioProcessor::getNumPrograms()
{
  return 1;
}

int MDAOverdriveAudioProcessor::getCurrentProgram()
{
  return 0;
}

void MDAOverdriveAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDAOverdriveAudioProcessor::getProgramName(int index)
{
  return {};
}

void MDAOverdriveAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAOverdriveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  resetState();
  _parametersChanged.store(true);
}

void MDAOverdriveAudioProcessor::releaseResources()
{
}

void MDAOverdriveAudioProcessor::reset()
{
  resetState();
}

bool MDAOverdriveAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAOverdriveAudioProcessor::resetState()
{
  // Set the filter delay units back to zero.
  _filtL = _filtR = 0.0f;
}

void MDAOverdriveAudioProcessor::update()
{
  // The amount of drive is a percentage; divide by 100 to make it 0 - 1.
  _drive = apvts.getRawParameterValue("Drive")->load() / 100.0f;

  // The muffle parameter controls a low-pass filter. Muffle is a percentage;
  // convert it to a value between 1.0 and 0.025 that drops off exponentially.
  // 1.0 means the filter is disabled; the smaller the value, the lower the
  // cutoff frequency will be. At 100%, the cutoff is around 200 Hz somewhere.
  // Since it's a first-order filter, it has a gentle roll-off of 6 dB/octave.
  float muffle = apvts.getRawParameterValue("Muffle")->load();
  _filt = std::pow(10.0f, -1.6f * muffle / 100.0f);

  // The output level is between -20 dB and +20 dB. Convert to linear gain.
  float output = apvts.getRawParameterValue("Output")->load();
  _gain = juce::Decibels::decibelsToGain(output);
}

void MDAOverdriveAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  const float *in1 = buffer.getReadPointer(0);
  const float *in2 = buffer.getReadPointer(1);
  float *out1 = buffer.getWritePointer(0);
  float *out2 = buffer.getWritePointer(1);

  const float drive = _drive;
  const float gain = _gain;
  const float f = _filt;

  float fa = _filtL, fb = _filtR;

  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    float a = in1[i];
    float b = in2[i];

    // Overdrive: this applies a sqrt to slightly distort and boost the sound.
    float aa = (a > 0.0f) ? std::sqrt(a) : -std::sqrt(-a);
    float bb = (b > 0.0f) ? std::sqrt(b) : -std::sqrt(-b);

    // Filter: this is a simple exponentially weighted moving average filter.
    // The difference equation is: y(n) = f * x(n) + (1 - f) * y(n - 1).
    //
    // When f = 1, the filter is disabled since that makes (1 - f) equal to 0.
    // As you move the "muffle" slider to the right, f becomes exponentially
    // smaller and more smoothing is applied, making the filter cutoff lower.
    //
    // The input to the filter x(n) = drive * (aa - a) + a. This is simply the
    // linear interpolation between the original sample value and the overdriven
    // version, aa or sqrt(a). Rewriting the math shows that this is the same
    // as x(n) = drive * aa + (1 - drive) * a. The larger drive is, the more we
    // add the overdriven signal into the mix.
    //
    // fa is y(n - 1) for the left channel and fb is for the right channel.
    fa = fa + f * (drive * (aa - a) + a - fa);
    fb = fb + f * (drive * (bb - b) + b - fb);

    // Apply output gain and write to output buffer.
    out1[i] = fa * gain;
    out2[i] = fb * gain;
  }

  // Catch denormals
  if (std::abs(fa) > 1.0e-10f) _filtL = fa; else _filtL = 0.0f;
  if (std::abs(fb) > 1.0e-10f) _filtR = fb; else _filtR = 0.0f;
}

juce::AudioProcessorEditor *MDAOverdriveAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDAOverdriveAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAOverdriveAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAOverdriveAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Drive",
    "Drive",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Muffle",
    "Muffle",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Output",
    "Output",
    juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
    0.0f,
    "dB"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDAOverdriveAudioProcessor();
}
