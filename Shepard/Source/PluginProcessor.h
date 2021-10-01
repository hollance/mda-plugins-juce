#pragma once

#include <JuceHeader.h>

class MDAShepardAudioProcessor : public juce::AudioProcessor,
                                 private juce::ValueTree::Listener
{
public:
  MDAShepardAudioProcessor();
  ~MDAShepardAudioProcessor() override;

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

  // The currently selected mode.
  int _mode;

  // Output gain level.
  float _level;

  // The speed of rising or falling.
  float _delta;

  // Two wavetables containing sine waves.
  static const int bufferSize = 512;
  float _buf1[bufferSize];
  float _buf2[bufferSize];

  // Read position in the wavetables.
  float _pos;

  // Increment of read position. This is what determines the current pitch.
  float _rate;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDAShepardAudioProcessor)
};
