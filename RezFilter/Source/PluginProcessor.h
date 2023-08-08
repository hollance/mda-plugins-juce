#pragma once

#include <JuceHeader.h>

class MDARezFilterAudioProcessor : public juce::AudioProcessor
{
public:
    MDARezFilterAudioProcessor();
    ~MDARezFilterAudioProcessor() override;

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

    float cutoff;      // filter cutoff
    float q;           // filter Q
    float gain;        // output gain
    float filterMax;   // maximum cutoff

    float envDepth;    // envelope modulation amount
    float attack;      // envelope attack coefficient
    float release;     // envelope release coefficient
    float env;         // current envelope level

    float lfo;         // most recent LFO value
    float lfoPhase;    // current LFO phase
    float lfoInc;      // LFO rate
    float lfoDepth;    // LFO modulation amount
    bool sampleHold;   // 0 = sine, 1 = sample & hold

    float buf0, buf1;  // filter delay units

    float threshold;     // envelope trigger threshold
    float triggerEnv;    // secondary envelope used when triggered
    bool triggered;      // whether envelope exceeded threshold
    bool triggerAttack;  // triggerEnv currently in attack mode

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDARezFilterAudioProcessor)
};
