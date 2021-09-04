#include "PluginProcessor.h"

MDARingModAudioProcessor::MDARingModAudioProcessor()
  : AudioProcessor(BusesProperties()
                   .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                   .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  apvts.state.addListener(this);
}

MDARingModAudioProcessor::~MDARingModAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDARingModAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDARingModAudioProcessor::getNumPrograms()
{
  return 1;
}

int MDARingModAudioProcessor::getCurrentProgram()
{
  return 0;
}

void MDARingModAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDARingModAudioProcessor::getProgramName(int index)
{
  return {};
}

void MDARingModAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDARingModAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  resetState();
  _parametersChanged.store(true);
}

void MDARingModAudioProcessor::releaseResources()
{
}

void MDARingModAudioProcessor::reset()
{
  resetState();
}

bool MDARingModAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDARingModAudioProcessor::resetState()
{
  _phase = 0.0f;
  _prevL = 0.0f;
  _prevR = 0.0f;
}

void MDARingModAudioProcessor::update()
{
  float freq = apvts.getRawParameterValue("Frequency")->load();
  float fine = apvts.getRawParameterValue("Fine-tune")->load();

  const auto twoPi = juce::MathConstants<float>::twoPi;

  // The phase increment is 2*pi*freq/sampleRate. The frequency is between 0 Hz
  // and 16000 Hz, in steps of 100 Hz. The fine-tune amount is from 0 - 100 Hz.
  _phaseInc = twoPi * (fine + freq) / (float)getSampleRate();

  // Feedback is a percentage from 0 to 95%.
  _feedbackAmount = apvts.getRawParameterValue("Feedback")->load() / 100.0f;

  // Convert from decibels to a linear gain value.
  float level = apvts.getRawParameterValue("Level")->load();
  _level = juce::Decibels::decibelsToGain(level);
}

void MDARingModAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  const float level = _level;
  const float phaseInc = _phaseInc;
  const float feedback = _feedbackAmount;

  float phase = _phase;
  float prevL = _prevL;
  float prevR = _prevR;

  const auto twoPi = juce::MathConstants<float>::twoPi;

  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    // The value of the sine is the instantaneous gain.
    const float g = std::sin(phase);

    // Increment the phase and wrap at 2 pi.
    phase = std::fmod(phase + phaseInc, twoPi);

    // Add the previous output value to the new input sample, multiplied by the
    // feedback factor. Then multiply by the sine wave for ring modulation.
    prevL = (feedback * prevL + in1[i]) * g;
    prevR = (feedback * prevR + in2[i]) * g;

    // Before putting the value into the output buffer, multiply it by the
    // output level in order to attenuate it, if necessary.
    out1[i] = prevL * level;
    out2[i] = prevR * level;
  }

  _phase = phase;
  _prevL = prevL;
  _prevR = prevR;
}

juce::AudioProcessorEditor *MDARingModAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDARingModAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDARingModAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDARingModAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Frequency",
    "Frequency",
    juce::NormalisableRange<float>(0.0f, 16000.0f, 100.0f, 0.5f),
    1000.0f,
    "Hz"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Fine-tune",
    "Fine-tune",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
    0.0f,
    "Hz"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Feedback",
    "Feedback",
    juce::NormalisableRange<float>(0.0f, 95.0f, 0.01f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Level",
    "Level",
    juce::NormalisableRange<float>(-30.0f, 0.0f, 0.01f),
    -6.0f,
    "dB"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDARingModAudioProcessor();
}
