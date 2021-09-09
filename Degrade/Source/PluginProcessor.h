#pragma once

#include <JuceHeader.h>

class MDADegradeAudioProcessor : public juce::AudioProcessor,
                                 private juce::ValueTree::Listener
{
public:
  MDADegradeAudioProcessor();
  ~MDADegradeAudioProcessor() override;

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
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override {
    _parametersChanged.store(true);
  }

  std::atomic<bool> _parametersChanged { false };

  void update();
  float filterFreq(float hz);
  void resetState();

  // To reduce the sampling rate, we only read from the input buffer every
  // sampleInterval samples.
  int _sampleInterval, _sampleIndex;

  // This is 1.0 if sample-and-hold mode is active, 0.0 if not.
  float _mode;

  // Used to perform quantizing.
  float _g1, _g2;

  // Non-linearity values for negative and positive samples.
  float _linNeg, _linPos;

  // Level for headroom clipping.
  float _clip;

  // Output gain.
  float _g3;

  // Filter coefficients.
  float _fi, _fo;

  // Sum of the previous samples in sample-and-hold mode.
  float _accum;

  // The most recently computed sample value, before filtering.
  float _currentSample;

  // Delay units for the 8 filter stages.
  float _buf1, _buf2, _buf3, _buf4, _buf6, _buf7, _buf8, _buf9;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDADegradeAudioProcessor)
};
