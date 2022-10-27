#include "PluginProcessor.h"

MDASubSynthAudioProcessor::MDASubSynthAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDASubSynthAudioProcessor::~MDASubSynthAudioProcessor()
{
}

const juce::String MDASubSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDASubSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDASubSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDASubSynthAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDASubSynthAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDASubSynthAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDASubSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store this in a variable so we can use it to format the parameters.
    _sampleRate = sampleRate;

    resetState();
}

void MDASubSynthAudioProcessor::releaseResources()
{
}

void MDASubSynthAudioProcessor::reset()
{
    resetState();
}

bool MDASubSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDASubSynthAudioProcessor::resetState()
{
    _oscPhase = 0.0f;
    _env = 0.0f;
    _filt1 = _filt2 = _filt3 = _filt4 = 0.0f;
}

void MDASubSynthAudioProcessor::update()
{
    // Reset these to their starting values.
    _sign = 1.0f;
    _phase = 1.0f;

    // What is the current mode? The first three modes -- Distort, Divide, and
    // Invert -- manipulate the input signal to enhance the bass frequencies.
    // The fourth mode, Key Osc, outputs a decaying sine wave.
    _type = apvts.getRawParameterValue("Type")->load();

    // The filter is a basic one-pole filter. The difference equation is:
    //   y(n + 1) = a * x(n) + (1 - a) * y(n)
    // The coefficient a goes from 0.001 to 0.1. This roughly corresponds to
    // the -3 dB frequency being at 10 - 320 Hz, but I'm not 100% convinced
    // these are correct numbers.
    // In "Key Osc" mode, the cut-off is set to a fixed frequency.
    float fParam3 = apvts.getRawParameterValue("Tune")->load();
    _filti = (_type == 3) ? 0.018f : std::pow(10.0f, -3.0f + (2.0f * fParam3));
    _filto = 1.0f - _filti;

    // In "Key Osc" mode, the Tune parameter sets the oscillator frequency.
    _phaseInc = 0.456159f * std::pow(10.0f, -2.5f + (1.5f * fParam3));

    // The wet/dry levels are percentages (not decibel levels).
    _wet = apvts.getRawParameterValue("Level")->load() * 0.01f;
    _dry = apvts.getRawParameterValue("Dry Mix")->load() * 0.01f;

    // The threshold parameter is in decibels, so convert this to linear gain.
    float fParam5 = apvts.getRawParameterValue("Thresh")->load();
    _threshold = juce::Decibels::decibelsToGain(fParam5);

    // In "Key Osc" mode, the plug-in creates a sine wave. This sine will decay
    // when the level of the input sound is below the threshold parameter.
    // The envelope is computed by env *= decay, making it an exponential decay.
    // Here, we convert the parameter into a value between 0.99 and 0.99999.
    // The smaller this value, the quicker the sound decays.
    float fParam6 = apvts.getRawParameterValue("Release")->load();
    _decay = 1.0f - std::pow(10.0f, -2.0f - (3.0f * fParam6));
}

void MDASubSynthAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    update();

    const float *in1 = buffer.getReadPointer(0);
    const float *in2 = buffer.getReadPointer(1);
    float *out1 = buffer.getWritePointer(0);
    float *out2 = buffer.getWritePointer(1);

    const int type = _type;
    const float phaseInc = _phaseInc;
    const float decay = _decay;
    const float threshold = _threshold;
    const float wet = _wet;
    const float dry = _dry;
    const float fi = _filti;
    const float fo = _filto;

    float sign = _sign;
    float phase = _phase;
    float osc = _oscPhase;
    float env = _env;
    float f1 = _filt1;
    float f2 = _filt2;
    float f3 = _filt3;
    float f4 = _filt4;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        // Combine the two channels into one (stereo to mono) and low-pass filter
        // it twice. Filtering twice gives us a roll-off of 12 dB/octave.
        f1 = (fo * f1) + (fi * (a + b));
        f2 = (fo * f2) + (fi * f1);

        float sub = 0.0f;

        // Only in Distort, Divide, and Invert modes:
        if (type != 3) {
            // Turn the low-pass filtered input into a very harsh "binary" signal
            // that is either -1, 0, or +1, with nothing in between. The threshold
            // level, together with the amplitude of the input audio, determines how
            // extreme this effect is. Where the sound is louder than the threshold,
            // you get a series of "pulses" going between -1 and +1; where the sound
            // is softer, you get silence. This is like applying a limiter and also
            // sampling with a resolution of 2 bits. To reduce the intensity of this
            // plug-in's effect, move the threshold closer to 0 dB, so that there
            // are fewer of these "pulses".
            if (f2 > threshold) {
                sub = 1.0f;
            } else if (f2 < -threshold) {
                sub = -1.0f;
            } else {
                sub = 0.0f;
            }

            // In Distort mode, the above clipping to produce harmonics is all
            // that we do, followed by another series of low-pass filters below.
            // However, the Divide and Invert modes do additional processing.

            // Octave divider
            if (sub * sign < 0.0f) {
                sign = -sign;
                if (sign < 0.0f) phase = -phase;
            }

            // In Divide mode, multiply the distorted signal by the value of phase.
            // This causes the distorted signal to become an octave lower for extra
            // nastiness.
            if (type == 1) {
                sub = phase * sub;
            }

            /*
              How the above works:

              The value of sign is initially 1. The distorted signal "sub" is
              always -1, 0, or +1. When sign = 1, nothing happens for sub = 0
              or +1, because sub * sign is >= 0 in those cases. But when sub
              becomes -1, sub * sign is now < 0 and we enter the if-statement.

              There we flip the value of sign, making it -1. Phase is initially 1.
              Since sign becomes negative, phase becomes -1.

              The output signal is then the distorted signal, which was -1, times
              the value of phase, also -1. So the output is +1.

              While sub remains at -1 (or becomes 0), nothing changes: sub * sign
              is now 0 * -1 or -1 * -1, which both are >= 0.

              At some point, sub becomes +1 again. Now sub * sign is < 0 and we
              enter the if-statement: sign becomes +1, phase remains -1.

              The output signal is sub (+1) times phase (-1), which is -1.

              What happened so far is that the output signal is the inverse of
              the input signal. But that's not all, keep reading...

              While sub remains at +1 (or becomes 0), nothing changes.

              Eventually, sub becomes -1. Now sub * sign is < 0. We flip sign
              to -1, which causes phase to be flipped to +1. The output is now
              -1 * +1 = -1.

              Note that this two cycles of -1 in a row. Even though the signal
              went from - to + to -, the output stayed at -1.

              When sub becomes +1 again, we flip sign to +1, phase stays at +1.
              The output is +1.

              When sub becomes -1 again, sign flips to -1 and phase becomes -1.
              The output is -1 * -1 = +1.

              So now we've done two cycles where the output is +1. And so on...

              What happens is that the output stays high or low for twice as
              long as the input does. This makes its period twice as large and
              the frequency two times (i.e. one octave) lower.
             */

            // In Invert mode, we also apply the phase value to make the sound an
            // octave lower, but this time we use the filtered, undistorted input.
            // The factor 2 is to add a +6 dB boost.
            if (type == 2) {
                sub = phase * f2 * 2.0f;
            }
        }

        // Key Osc mode
        if (type == 3) {
            // As long as the low-pass filtered input sound is over the threshold,
            // output a sustained sine wave. But when the input level drops below
            // the threshold, decay the sine wave. This creates a "boom" sound when
            // new notes are played or drums are hit. (There is no attack on the
            // envelope, so this may have DC offset glitches, but these get smoothed
            // out by the low-pass filter that follows.)
            if (f2 > threshold) {
                env = 1.0f;
            } else {
                env = env * decay;  // exponential decay
            }

            // The sound itself is just a basic sine wave oscillator.
            sub = env * std::sin(osc);
            osc = std::fmod(osc + phaseInc, 6.283185f);
        }

        // Low-pass filter the sub-bass sound two more times using the same filter.
        // This is needed to get rid of discontinuities introduced above.
        f3 = (fo * f3) + (fi * sub);
        f4 = (fo * f4) + (fi * f3);

        // Mix the sub-bass signal with the original into the buffer.
        out1[i] = (a * dry) + (f4 * wet);
        out2[i] = (b * dry) + (f4 * wet);
    }

    // Fix numerical underflow.
    if (std::abs(f1) < 1.0e-10f) _filt1 = 0.0f; else _filt1 = f1;
    if (std::abs(f2) < 1.0e-10f) _filt2 = 0.0f; else _filt2 = f2;
    if (std::abs(f3) < 1.0e-10f) _filt3 = 0.0f; else _filt3 = f3;
    if (std::abs(f4) < 1.0e-10f) _filt4 = 0.0f; else _filt4 = f4;

    _sign = sign;
    _phase = phase;
    _oscPhase = osc;
    _env = env;
}

juce::AudioProcessorEditor *MDASubSynthAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDASubSynthAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDASubSynthAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDASubSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Type", 1),
        "Type",
        juce::StringArray({ "Distort", "Divide", "Invert", "Key Osc." }),
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Level", 1),
        "Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        30.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Tune", 1),
        "Tune",
        juce::NormalisableRange<float>(),
        0.6f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction(
                [this](float value, int) {
                    // Convert the 0 - 1 parameter into a frequency between 10
                    // and 320 Hz. (It's simpler to just set the NormalisableRange
                    // to [10, 320] and give the parameter a skew factor.)
                    float x = 0.0726f * _sampleRate * std::pow(10.0f, -2.5f + (1.5f * value));
                    return juce::String(x, 1);
                }
            )));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Dry Mix", 1),
        "Dry Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Thresh", 1),
        "Thresh",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f),
        -24.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Release", 1),
        "Release",
        juce::NormalisableRange<float>(),
        0.65f,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction(
                [this](float value, int) {
                    // Convert the 0 - 1 parameter into a decay value between
                    // 0.99 and 1.0.
                    float x = 1.0f - std::pow(10.0f, -2.0f - (3.0f * value));

                    // For display in the UI, we want to convert this decay value
                    // to a time in milliseconds. The original plug-in used the
                    // formula: -301.03f / (sampleRate * log10(x))
                    // This appears to be an estimate of how long it takes for the
                    // signal to decay from 1.0 to roughly 0.001, i.e. by a factor
                    // of 1000 or by 60 dB. However, the constant in that formula
                    // seems to be off by about 10x.
                    x = -3010.301f / (_sampleRate * std::log10(x));

                    return juce::String(x, 1);
                }
            )));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDASubSynthAudioProcessor();
}
