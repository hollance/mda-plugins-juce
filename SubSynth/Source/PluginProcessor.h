#pragma once

#include <JuceHeader.h>

class MDASubSynthAudioProcessor : public juce::AudioProcessor
{
public:
    MDASubSynthAudioProcessor();
    ~MDASubSynthAudioProcessor() override;

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

    // Used to calculate the release time in milliseconds in the UI.
    float _sampleRate;

    // The kind of sub-bass sound to add.
    int _type;

    // Amount of synthesized low-frequency signal to be added.
    float _wet;

    // Reduces the level of the original signal.
    float _dry;

    // Threshold level. The lower this is, the more intense the effect.
    float _threshold;

    // Used to find the octave below the input frequency.
    float _sign, _phase;

    // Oscillator phase and phase increment, for "Key Osc" mode.
    float _oscPhase, _phaseInc;

    // Current envelope level for the oscillator.
    float _env;

    // Decay amount for "Key Osc" mode.
    float _decay;

    // Low-pass filter coefficients.
    float _filti, _filto;

    // Filter delays. We use the same filter four times.
    float _filt1, _filt2, _filt3, _filt4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDASubSynthAudioProcessor)
};
