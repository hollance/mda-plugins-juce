#pragma once

#include <JuceHeader.h>

class MDATestToneAudioProcessor : public juce::AudioProcessor,
                                  private juce::ValueTree::Listener,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    MDATestToneAudioProcessor();
    ~MDATestToneAudioProcessor() override;

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

    void parameterChanged(const juce::String &identifier, float value) override
    {
        // The following code triggers the UI to redraw F1 and F2 when the mode
        // changes, and the output level when the calibration setting changes.
        // Note that this doesn't work in AudioPluginHost's "Show all parameters"
        // but it does work with "Show plugin GUI". Not sure if this is the best
        // way to handle it in JUCE, but it seems to work. :-)

        if (identifier == "Mode") {
            auto f1Param = apvts.getParameter("F1");
            f1Param->beginChangeGesture();
            f1Param->setValueNotifyingHost(f1Param->getValue());
            f1Param->endChangeGesture();

            auto f2Param = apvts.getParameter("F2");
            f2Param->beginChangeGesture();
            f2Param->setValueNotifyingHost(f2Param->getValue());
            f2Param->endChangeGesture();
        }
        else if (identifier == "0dB =") {
            auto param = apvts.getParameter("Level");
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->getValue());
            param->endChangeGesture();
        }
    }

    void update();
    void resetState();

    juce::String midi2string(float n);
    juce::String iso2string(float b);

    // The meaning of the F1 and F2 parameters changes based on the current mode.
    // What is displayed for the output level depends on the calibration setting.
    // These functions return the parameter value that is displayed to the user.
    juce::String stringFromValueF1(float value);
    juce::String stringFromValueF2(float value);
    juce::String stringFromValueOutputLevel(float value);
    juce::String stringFromValueCalibration(float value);

    // The currently selected mode.
    int _mode;

    // Gain for the left and right channels.
    float _left, _right;

    // Incoming audio is mixed into the generated sound at this gain level.
    float _thru;

    // Sweep time in seconds (1 - 32 sec).
    float _durationInSeconds;

    // Sweep time in number of samples.
    int _durationInSamples;

    // Remaining sweep time in number of samples.
    int _sweepRemaining;

    // Start and end frequencies for sweep.
    float _sweepStart, _sweepEnd;

    // Current sweep frequency.
    float _sweepFreq;

    // Step value for the frequency sweeps.
    float _sweepInc;

    // 2 pi / sample rate. Used for converting frequencies to radians.
    float _fscale;

    // Current phase and phase increment for sine wave oscillator.
    float _phase, _phaseInc;

    // Filter delay units for the pink noise filter.
    float _z0, _z1, _z2, _z3, _z4, _z5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDATestToneAudioProcessor)
};
