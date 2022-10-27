#include "PluginProcessor.h"

static constexpr float twopi = 6.2831853f;

MDATestToneAudioProcessor::MDATestToneAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    apvts.state.addListener(this);
    apvts.addParameterListener("Mode", this);
    apvts.addParameterListener("0dB =", this);
}

MDATestToneAudioProcessor::~MDATestToneAudioProcessor()
{
    apvts.removeParameterListener("Mode", this);
    apvts.removeParameterListener("0dB =", this);
    apvts.state.removeListener(this);
}

const juce::String MDATestToneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDATestToneAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDATestToneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDATestToneAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDATestToneAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDATestToneAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDATestToneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
    _parametersChanged.store(true);
}

void MDATestToneAudioProcessor::releaseResources()
{
}

void MDATestToneAudioProcessor::reset()
{
    resetState();
}

bool MDATestToneAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDATestToneAudioProcessor::resetState()
{
    // Reset the filter delays and the oscillator phase.
    _z0 = _z1 = _z2 = _z3 = _z4 = _z5 = _phase = 0.0f;
}

void MDATestToneAudioProcessor::update()
{
    _mode = int(apvts.getRawParameterValue("Mode")->load());

    // Level is -60 db to 0 dB.
    float fParam1 = apvts.getRawParameterValue("Level")->load();
    _left = juce::Decibels::decibelsToGain(fParam1);

    // For white noise and pink noise, the random generator outputs numbers
    // between ±16384, so scale these back to the range 0 - 1 (more or less).
    if (_mode == 2) _left *= 0.0000610f;  // white noise
    if (_mode == 3) _left *= 0.0000243f;  // pink noise

    // Using left channel, right channel, or both?
    float fParam2 = apvts.getRawParameterValue("Channel")->load();
    if (fParam2 < 0.3f) _right = 0.0f; else _right = _left;
    if (fParam2 > 0.6f) _left = 0.0f;

    // The sweep time parameter is in milliseconds. Convert to number of samples.
    float fParam6 = apvts.getRawParameterValue("Sweep")->load();
    _durationInSeconds = fParam6 / 1000.0f;
    _durationInSamples = int(_durationInSeconds * getSampleRate());
    _sweepRemaining = _durationInSamples;

    // Calibration: This code looks complicated but it just creates a slider that
    // goes from -20 dB to 0 dB; there's also a section between -1 dB and 0 dB
    // that goes in smaller steps.
    float fParam7 = apvts.getRawParameterValue("0dB =")->load();
    if (fParam7 > 0.8f) {  // -1 dB to 0 dB
        float cal;
        if (fParam7 > 0.96f) cal = 0.0f;
        else if (fParam7 > 0.92f) cal = -0.01000001f;
        else if (fParam7 > 0.88f) cal = -0.02000001f;
        else if (fParam7 > 0.84f) cal = -0.1f;
        else cal = -0.2f;

        // Convert decibels to a linear gain. Use this to trim the output level.
        cal = std::pow(10.0f, 0.05f * cal);
        _left *= cal;
        _right *= cal;
    } else {
        // Here, the slider is between -21 dB to -1 dB. We only use this to change
        // how the output level is displayed. There's nothing to calculate here.
    }

    // The meaning of F1 and F2 depends on the mode the user has chosen.
    float fParam3 = apvts.getRawParameterValue("F1")->load();
    float fParam4 = apvts.getRawParameterValue("F2")->load();

    // F2 is sometimes used to "fine tune" the value of F1. In that case, the F2
    // slider goes from -0.5 to +0.5. However, this is not entirely linear: there
    // is an area around the center (between 40% and 60%) that's always 0. Maybe
    // that was done to make it easier to set this parameter to zero.
    float df = 0.0f;
    if (fParam4 > 0.6) df = 1.25f*fParam4 - 0.75f;  // 0 to 0.5
    if (fParam4 < 0.4) df = 1.25f*fParam4 - 0.50f;  // -0.5 to 0

    // To get radians, multiply the frequency in Hz by 2 pi / sampleRate.
    _fscale = twopi / getSampleRate();

    switch (_mode) {
        // MIDI note:
        // F1 chooses the MIDI note number
        // F2 is used to tune the note up or down by ±50 cents
        case 0: {
            float f = std::floor(128.0f * fParam3);

            // The usual formula for converting a MIDI note number into a frequency
            // is 440 * 2^((note - 69) / 12). Then to find the phase increment, we
            // multiply by 2 pi and divide by the sample rate. That works out to be
            // exactly the same as the formula used here:
            _phaseInc = 51.37006f * std::pow(1.0594631f, f + df) / getSampleRate();
            break;
        }

        // Sine wave:
        // F1 chooses the frequency on a logaritmic scale
        // F2 fine-tunes that frequency by 2 semitones up or down
        case 5: {
            // Convert F1 from 0 - 1 into the range 13 - 43 (integers only), and then
            // turn it in to a frequency between 20 and 20000 Hz. Why these strange
            // numbers? This is from the ISO 266 standard for preferred frequencies
            // for acoustical measurements. The formula is 1000 * 10^(n / 10) where n
            // is an integer. For n = -17, the freq is 20 Hz; for n = 13, the freq is
            // 20 kHz. Here, that formula was rewritten so that param = 0 gives 20 Hz
            // and param = 1 gives 20 kHz. The new formula is 1000 * 10^((f - 30)/10)
            // where f is now between 13 and 43. The factor 1000 cancels out against
            // the -30/10 in the exponent, which leaves 10^(f / 10).
            float f = 13.0f + std::floor(30.0f * fParam3);
            f = std::pow(10.0f, 0.1f * (f + df));
            _phaseInc = _fscale * f;
            break;
        }

        // Log sweep & step:
        // F1 is the first frequency
        // F2 is the second frequency
        case 6:
        case 7:
            // Convert F1 and F2 from 0 - 1 into the range 13 - 43 (integers only),
            // but don't turn these numbers into real frequencies yet. In the audio
            // callback, we'll turn this into a frequency using pow(10, 0.1 * value).
            // That makes these variables log-frequencies, i.e. they are the base-10
            // logarithms of the actual frequencies, where 13 corresponds to 20 Hz
            // and 43 to 20 kHz. We do this to keep these values in log-space so that
            // interpolating from start to finish is done in logarithmic steps.
            _sweepStart = 13.0f + std::floor(30.0f * fParam3);
            _sweepEnd = 13.0f + std::floor(30.0f * fParam4);

            // Only sweep up. Swap the frequencies if start is lower than end.
            // (It shouldn't be very hard to change the plug-in so it allows sweeps
            // from high to low frequencies as well.)
            if (_sweepStart > _sweepEnd) {
                float tmp = _sweepEnd;
                _sweepEnd = _sweepStart;
                _sweepStart = tmp;
            }

            // In step mode, make sure to include the end frequency as the final step.
            if (_mode == 7) _sweepEnd += 1.0f;

            // How much do we need to increment the log-frequency on every sample?
            _sweepInc = (_sweepEnd - _sweepStart) / (_durationInSeconds * getSampleRate());
            _sweepFreq = _sweepStart;

            // Put two seconds of silence between sweeps.
            _sweepRemaining = _durationInSamples = 2 * int(getSampleRate());
            break;

        // Linear sweep:
        // F1 is the first frequency
        // F2 is the second frequency
        case 8:
            // Turn F1 and F2 directly into a frequency between 0 Hz and 20000 Hz.
            _sweepStart = 200.0f * std::floor(100.0f * fParam3);
            _sweepEnd = 200.0f * std::floor(100.0f * fParam4);

            // Only sweep up. Swap the frequencies if start is lower than end.
            if (_sweepStart > _sweepEnd) {
                float tmp = _sweepEnd;
                _sweepEnd = _sweepStart;
                _sweepStart = tmp;
            }

            // Convert the start / end frequencies to an angle in radians.
            _sweepStart = _fscale * _sweepStart;
            _sweepEnd = _fscale * _sweepEnd;
            _sweepFreq = _sweepStart;

            // We increment the angle by this step size on every sample. This will
            // linearly move the frequency of the tone from the start angle to the
            // end angle over the total duration of the sweep.
            _sweepInc = (_sweepEnd - _sweepStart) / (_durationInSeconds * getSampleRate());

            // Put two seconds of silence between sweeps.
            _sweepRemaining = _durationInSamples = 2 * int(getSampleRate());
            break;
    }

    // Audio thru determines the loudness of input audio in the mix. This is a
    // decibel value between -40 dB and 0 dB, so convert it to linear gain.
    float fParam5 = apvts.getRawParameterValue("Thru")->load();
    if (fParam5 == 0.0f) {
        _thru = 0.0f;  // no audio thru
    } else {
        _thru = juce::Decibels::decibelsToGain(fParam5);
    }
}

void MDATestToneAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // Only recalculate when a parameter has changed.
    // The original plug-in used two counters for this, updateRx and updateTx.
    // Whenever a parameter changed, updateTx was incremented. At the start of
    // the audio processing code, it would call update() if updateRx != updateTx.
    // Inside update(), updateRx would be set equal to updateTx again. (The way
    // we do it is a bit more thread-safe.)
    bool expected = true;
    if (_parametersChanged.compare_exchange_strong(expected, false)) {
        update();
    }

    const float *in1 = buffer.getReadPointer(0);
    const float *in2 = buffer.getReadPointer(1);
    float *out1 = buffer.getWritePointer(0);
    float *out2 = buffer.getWritePointer(1);

    const int mode = _mode;
    const float thru = _thru;
    const float left = _left;
    const float right = _right;
    const float fscale = _fscale;
    const float sweepStart = _sweepStart;
    const float sweepEnd = _sweepEnd;
    const float step = _sweepInc;
    const int sweepDuration = _durationInSamples;

    int samplesRemaining = _sweepRemaining;
    float freq = _sweepFreq;
    float phase = _phase;
    float phaseInc = _phaseInc;
    float z0 = _z0, z1 = _z1, z2 = _z2, z3 = _z3, z4 = _z4, z5 = _z5;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        float x = 0.0f;

        switch (mode) {
            // Impulse
            case 1:
                // This simply counts down. When the timer is zero, output a short
                // pulse and reset the timer. Otherwise, always output silence.
                if (samplesRemaining > 0) {
                    samplesRemaining--;
                    x = 0.0f;
                } else {
                    samplesRemaining = sweepDuration;
                    x = 1.0f;
                }
                break;

            // White noise and pink noise
            case 2:
            case 3:
                // The original plug-in assumed RAND_MAX is 32767, but that's
                // wrong now. It would be better to use juce::Random for this.
                x = float(std::rand()) / RAND_MAX * 32767 - 16384;

                // Filter the white noise to get pink noise.
                if (mode == 3) {
                    z0 = 0.997f * z0 + 0.029591f * x;
                    z1 = 0.985f * z1 + 0.032534f * x;
                    z2 = 0.950f * z2 + 0.048056f * x;
                    z3 = 0.850f * z3 + 0.090579f * x;
                    z4 = 0.620f * z4 + 0.108990f * x;
                    z5 = 0.250f * z5 + 0.255784f * x;
                    x = z0 + z1 + z2 + z3 + z4 + z5;
                }
                break;

            // Mute
            case 4:
                x = 0.0f;
                break;

            // Tones, for MIDI note / sine wave mode
            case 0:
            case 5:
                // Just a simple sine wave oscillator. Increment the phase using
                // the phase increment and wrap around when it reaches 2 pi.
                phase = std::fmod(phase + phaseInc, twopi);
                x = std::sin(phase);
                break;

            // Log sweep & step
            case 6:
            case 7:
                // Before the sweep starts, count down and output zeros.
                // This puts two seconds of silence between sweeps.
                if (samplesRemaining > 0) {
                    samplesRemaining--;
                    phase = 0.0f;
                    x = 0.0f;
                } else {
                    // Output sine value for the current frequency.
                    x = std::sin(phase);

                    // Increment the frequency in log-space.
                    freq += step;

                    // Turn the log-frequency into an actual frequency in radians.
                    if (mode == 7) {
                        // In step mode, round off the frequency value. This steps
                        // upwards by roughly 4 semitones (a major third interval).
                        phaseInc = fscale * std::pow(10.0f, 0.1f * float(int(freq)));
                    } else {
                        phaseInc = fscale * std::pow(10.0f, 0.1f * freq);
                    }

                    // Use the frequency in radians to increment the oscillator phase.
                    phase = std::fmod(phase + phaseInc, twopi);

                    // If we've reached the end frequency, restart the sweep.
                    if (freq > sweepEnd) {
                        samplesRemaining = sweepDuration;
                        freq = sweepStart;
                    }
                }
                break;

            // Linear sweep
            case 8:
                // Before the sweep starts, count down and output zeros.
                // This puts two seconds of silence between sweeps.
                if (samplesRemaining > 0) {
                    samplesRemaining--;
                    phase = 0.0f;
                    x = 0.0f;
                } else {
                    // Output sine value for the current frequency.
                    x = std::sin(phase);

                    // Increment the phase linearly and wrap around at 2 pi.
                    freq += step;
                    phase = std::fmod(phase + freq, twopi);

                    // If we've reached the end frequency, restart the sweep.
                    if (freq > sweepEnd) {
                        samplesRemaining = sweepDuration;
                        freq = sweepStart;
                    }
                }
                break;
        }

        // Mix the incoming audio signal with the sound we produced.
        out1[i] = thru*a + left*x;
        out2[i] = thru*b + right*x;
    }

    _phase = phase;
    _sweepFreq = freq;
    _sweepRemaining = samplesRemaining;
    _z0 = z0; _z1 = z1; _z2 = z2; _z3 = z3; _z4 = z4; _z5 = z5;
}

juce::AudioProcessorEditor *MDATestToneAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDATestToneAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDATestToneAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        _parametersChanged.store(true);
    }
}

juce::String MDATestToneAudioProcessor::midi2string(float n)
{
    // It's a lot easier to build up the string with juce::String but this
    // is how the original plug-in did it.

    char t[8];
    int p = 0;

    // Convert the MIDI note number to text. This has up to 3 digits.
    int nn = int(n);
    if (nn > 99) t[p++] = 48 + (int(0.01 * n) % 10);
    if (nn > 9)  t[p++] = 48 + (int(0.10 * n) % 10);
    t[p++] = 48 + (int(n) % 10);
    t[p++] = ' ';

    // Find the octave and the note inside the octave.
    int o = int(nn / 12.0f);
    int s = nn - (12 * o);

    // Octaves go from -2 to +8.
    o -= 2;

    // Convert to a note name.
    switch (s) {
        case  0: t[p++] = 'C';               break;
        case  1: t[p++] = 'C'; t[p++] = '#'; break;
        case  2: t[p++] = 'D';               break;
        case  3: t[p++] = 'D'; t[p++] = '#'; break;
        case  4: t[p++] = 'E';               break;
        case  5: t[p++] = 'F';               break;
        case  6: t[p++] = 'F'; t[p++] = '#'; break;
        case  7: t[p++] = 'G';               break;
        case  8: t[p++] = 'G'; t[p++] = '#'; break;
        case  9: t[p++] = 'A';               break;
        case 10: t[p++] = 'A'; t[p++] = '#'; break;
        default: t[p++] = 'B';
    }

    // Convert the octave number to text.
    if (o < 0) { t[p++] = '-'; o = -o; }
    t[p++] = 48 + (o % 10);

    // Null-terminate the string.
    t[p] = 0;

    return t;
}

juce::String MDATestToneAudioProcessor::iso2string(float b)
{
    switch (int(b)) {
        case 13: return "20 Hz";
        case 14: return "25 Hz";
        case 15: return "31 Hz";
        case 16: return "40 Hz";
        case 17: return "50 Hz";
        case 18: return "63 Hz";
        case 19: return "80 Hz";
        case 20: return "100 Hz";
        case 21: return "125 Hz";
        case 22: return "160 Hz";
        case 23: return "200 Hz";
        case 24: return "250 Hz";
        case 25: return "310 Hz";
        case 26: return "400 Hz";
        case 27: return "500 Hz";
        case 28: return "630 Hz";
        case 29: return "800 Hz";
        case 30: return "1 kHz";
        case 31: return "1.25 kHz";
        case 32: return "1.6 kHz";
        case 33: return "2.0 kHz";
        case 34: return "2.5 kHz";
        case 35: return "3.1 kHz";
        case 36: return "4 kHz";
        case 37: return "5 kHz";
        case 38: return "6.3 kHz";
        case 39: return "8 kHz";
        case 40: return "10 kHz";
        case 41: return "12.5 kHz";
        case 42: return "16 kHz";
        case 43: return "20 kHz";
        default: return "--";
    }
}

juce::String MDATestToneAudioProcessor::stringFromValueF1(float value)
{
    const int mode = int(apvts.getRawParameterValue("Mode")->load());

    switch (mode) {
        // MIDI note
        case 0:
            return midi2string(std::floor(128.0f * value));  // semitones

        // sine wave, iso band freq
        case 5:
            return iso2string(13.0f + std::floor(30.0f * value));

        // log sweep & step start freq
        case 6:
        case 7:
            return iso2string(13.0f + std::floor(30.0f * value));

        // linear sweep start freq
        case 8:
            return juce::String(int(200.0f * std::floor(100.0f * value)));

        // no frequency display
        default:
            return "--";
    }
}

juce::String MDATestToneAudioProcessor::stringFromValueF2(float value)
{
    const int mode = int(apvts.getRawParameterValue("Mode")->load());

    float df = 0.0f;
    if (value > 0.6) df = 1.25f*value - 0.75f;  // 0 to 0.5
    if (value < 0.4) df = 1.25f*value - 0.50f;  // -0.5 to 0

    switch (mode) {
        // MIDI note
        case 0:
            return juce::String(int(100.0f * df));  // cents

        // sine wave, Hz
        case 5: {
            float f1Value = apvts.getRawParameterValue("F1")->load();
            float f = 13.0f + std::floor(30.0f * f1Value);
            f = std::pow(10.0f, 0.1f * (f + df));
            return juce::String(f, 2);
        }

        // log sweep & step end freq
        case 6:
        case 7:
            return iso2string(13.0f + std::floor(30.0f * value));

        // linear sweep end freq
        case 8:
            return juce::String(int(200.0f * std::floor(100.0f * value)));

        // no frequency display
        default:
            return "--";
    }
}

juce::String MDATestToneAudioProcessor::stringFromValueOutputLevel(float value)
{
    // If value of the "0dB =" slider is -1 dB or less, it isn't actually used
    // for any calculations -- it just changes the value displayed on the output
    // level slider.

    float fParam7 = apvts.getRawParameterValue("0dB =")->load();

    float calx;
    if (fParam7 > 0.8f) {
        // Don't calibrate the output level.
        calx = 0.0f;
    } else {
        // Convert the parameter to decibels: -21 dB to -1 dB.
        calx = int(25.0f * fParam7 - 21.1f);
    }

    return juce::String(value - calx, 2);
}

juce::String MDATestToneAudioProcessor::stringFromValueCalibration(float value)
{
    // Note that there's some duplicate code in these stringFromValue functions
    // that also exists in update(). Could refactor this and put the calculations
    // in a single place, but for a small plug-in like this, I'm not bothering.

    float cal = 0.0f;

    // From -1 dB to 0 dB the slider uses smaller steps.
    if (value > 0.8f) {
        if (value > 0.96f) cal = 0.0f;
        else if (value > 0.92f) cal = -0.01000001f;
        else if (value > 0.88f) cal = -0.02000001f;
        else if (value > 0.84f) cal = -0.1f;
        else cal = -0.2f;
    } else {
        // The rest of the slider goes from -21 dB to -1 dB in steps of 1 dB.
        cal = int(25.0f * value - 21.1f);
    }

    return juce::String(cal, 2);
}

juce::AudioProcessorValueTreeState::ParameterLayout MDATestToneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // The UI for this plug-in takes a little getting used to. Not all parameters
    // are relevant to all modes, and some parameters change meaning based on mode.

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::StringArray({ "MIDI #", "IMPULSE", "WHITE", "PINK", "---",
                            "SINE", "LOG SWP.", "LOG STEP", "LIN SWP." }),
        4));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Level", 1),
        "Level",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f),
        -16.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction(
                [this](float value, int) { return stringFromValueOutputLevel(value); }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Channel", 1),
        "Channel",
        juce::NormalisableRange<float>(),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("L <> R")
            .withStringFromValueFunction(
                [](float value, int) {
                    return value > 0.3f ? (value > 0.7f ? "RIGHT" : "CENTRE" ) : "LEFT";
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("F1", 1),
        "F1",
        juce::NormalisableRange<float>(),
        0.57f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [this](float value, int) { return stringFromValueF1(value); }
        )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("F2", 1),
        "F2",
        juce::NormalisableRange<float>(),
        0.50f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [this](float value, int) { return stringFromValueF2(value); }
        )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Sweep", 1),
        "Sweep",
        juce::NormalisableRange<float>(1000.0f, 32000.0f, 500.0f),
        10000.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Thru", 1),
        "Thru",
        juce::NormalisableRange<float>(-40.0f, 0.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction(
                [](float value, int) {
                    return value == 0.0f ? "OFF" : juce::String(value, 2);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("0dB =", 1),
        "0dB =",
        juce::NormalisableRange<float>(),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dBFS")
            .withStringFromValueFunction(
                [this](float value, int) { return stringFromValueCalibration(value); }
            )));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDATestToneAudioProcessor();
}
