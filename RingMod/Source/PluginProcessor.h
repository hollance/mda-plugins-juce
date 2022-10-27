#pragma once

#include <JuceHeader.h>

class MDARingModAudioProcessor : public juce::AudioProcessor
{
public:
    MDARingModAudioProcessor();
    ~MDARingModAudioProcessor() override;

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

    void update();
    void resetState();

    // Output level. This was not in the original plug-in, but with a lot of
    // feedback it's useful to dial back the total volume to prevent clipping.
    float _level;

    // Phase increment for the sine wave.
    float _phaseInc;

    // Amount of feedback to add (value between 0 and 1).
    float _feedbackAmount;

    // Current phase for the sine wave.
    float _phase;

    // Previous output values for the left and right channels; used for feedback.
    float _prevL, _prevR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDARingModAudioProcessor)
};
