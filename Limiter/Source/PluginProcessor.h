#pragma once

#include <JuceHeader.h>

class MDALimiterAudioProcessor : public juce::AudioProcessor,
                                 private juce::ValueTree::Listener
{
public:
  MDALimiterAudioProcessor();
  ~MDALimiterAudioProcessor() override;

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

  // The maximum amplitude you want the sound to have. Louder sounds will be
  // reduced to approximately this level. With a very slow attack it may take
  // a long time before the sound drops below the threshold level, if ever.
  // In soft knee mode, the threshold only indirectly represents the maximum
  // amplitude, but it still serves the same purpose.
  float _threshold;

  // The attack time determines how quickly the limiter responds when the sound
  // level approaches (in soft knee mode) or exceeds the threshold (hard knee).
  float _attack;

  // The release time determines how quickly the gain level resets itself after
  // the limiter no longer needs to be active.
  float _release;

  // Use this for boosting the output level when setting a low threshold.
  float _trim;

  // Choose between hard knee and soft knee modes.
  bool _softKnee;

  // The current gain level. When the audio signal exceeds or approaches the
  // threshold, the gain level is lowered to reduce the amplitude. The limiter
  // essentially calculates a gain signal over time that is used to prevent the
  // audio from becoming too loud. This is the gain signal's most recent value.
  float _gain;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDALimiterAudioProcessor)
};
