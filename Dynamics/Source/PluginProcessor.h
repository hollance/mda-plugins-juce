#pragma once

#include <JuceHeader.h>

class MDADynamicsAudioProcessor : public juce::AudioProcessor
{
public:
    MDADynamicsAudioProcessor();
    ~MDADynamicsAudioProcessor() override;

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

    float threshold;         // threshold for compressor
    float ratio;             // ratio for compressor
    float trim;              // makeup gain
    float attack;            // envelope attack coefficient
    float release;           // envelope release coefficient
    float limiterThreshold;  // limiter threshold
    float gateThreshold;     // gate threshold
    float gateAttack;        // gate attack coefficient
    float gateRelease;       // gate release coefficient
    float dry;               // dry/wet mix amount
    bool compressOnly;       // if false, also apply limiter & gate

    float env;               // current envelope level
    float limiterEnv;        // envelope used by the limiter
    float gateEnv;           // envelope used by the gate

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDADynamicsAudioProcessor)
};
