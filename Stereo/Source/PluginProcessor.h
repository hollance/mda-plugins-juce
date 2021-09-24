#pragma once

#include <JuceHeader.h>

class MDAStereoAudioProcessor : public juce::AudioProcessor,
                                private juce::ValueTree::Listener
{
public:
  MDAStereoAudioProcessor();
  ~MDAStereoAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void reset() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override;

  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout() };

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override
  {
    _parametersChanged.store(true);
  }

  std::atomic<bool> _parametersChanged { false };

  void update();
  void resetState();

  // Used to calculate the release time in milliseconds in the UI.
  float _sampleRate;

  // This buffer stores the delayed samples (mono).
  std::vector<float> _delayBuffer;

  // Maximum length of the delay buffer in samples.
  int _delayMax;

  // Where we will write the next new sample value in the delay buffer.
  int _writePos;

  // Delay time in samples.
  float _delayTime;

  // Amount of delay modulation (0.0 = off).
  float _mod;

  // Current phase and phase increment for the sine wave that is used to
  // modulate the read position in the delay buffer.
  float _phase, _phaseInc;

  // Filter coefficients for l=left, r=right, i=input, d=delayed sample.
  float _fli, _fld, _fri, _frd;

  // Output level.
  float _gain;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDAStereoAudioProcessor)
};
