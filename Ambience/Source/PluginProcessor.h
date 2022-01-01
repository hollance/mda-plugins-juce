#pragma once

#include <JuceHeader.h>

class MDAAmbienceAudioProcessor : public juce::AudioProcessor,
                                  private juce::ValueTree::Listener
{
public:
  MDAAmbienceAudioProcessor();
  ~MDAAmbienceAudioProcessor() override;

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

  void flushBuffers();

  // Delay lines. The maximum length of these is hardcoded to 1024 samples.
  float *_buf1, *_buf2, *_buf3, *_buf4;

  // Read position in the delay buffers.
  int _pos;

  // This sets the length of the delays.
  float _size;

  // Feedback coefficient for the allpass filters.
  float _feedback;

  // Low-pass filter coefficient for HF damping.
  float _damp;

  // Low-pass filter state value.
  float _filter;

  // Wet/dry mix.
  float _wet, _dry;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDAAmbienceAudioProcessor)
};
