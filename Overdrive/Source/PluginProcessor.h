#pragma once

#include <JuceHeader.h>

class MDAOverdriveAudioProcessor : public juce::AudioProcessor
{
public:
    MDAOverdriveAudioProcessor();
    ~MDAOverdriveAudioProcessor() override;

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

    // Amount of overdrive, a value between 0 and 1. This controls the mix
    // between the original signal and the overdriven one.
    float _drive;

    // Filter coefficient, a value between 1 and 0.025.
    float _filt;

    // Output gain, a value between 0.1 (for -20 dB) and 10 (for +20 dB).
    float _gain;

    // Delay units for the left and right channel filters.
    float _filtL, _filtR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDAOverdriveAudioProcessor)
};
