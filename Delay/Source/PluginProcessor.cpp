#include "PluginProcessor.h"

MDADelayAudioProcessor::MDADelayAudioProcessor()
  : AudioProcessor(BusesProperties()
                   .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                   .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  apvts.state.addListener(this);
}

MDADelayAudioProcessor::~MDADelayAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDADelayAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDADelayAudioProcessor::getNumPrograms()
{
  return 1;
}

int MDADelayAudioProcessor::getCurrentProgram()
{
  return 0;
}

void MDADelayAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDADelayAudioProcessor::getProgramName(int index)
{
  return {};
}

void MDADelayAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDADelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  // The maximum delay time is half a second. Feel free to make this smaller
  // or larger. The original plug-in used a fixed number of samples, but that
  // would make the maximum delay time different if the sample rate changed.
  _delayMax = int(std::ceil(sampleRate * 0.5));
  _delayBuffer.resize(_delayMax);

  resetState();
  _parametersChanged.store(true);
}

void MDADelayAudioProcessor::releaseResources()
{
}

void MDADelayAudioProcessor::reset()
{
  resetState();
}

bool MDADelayAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDADelayAudioProcessor::resetState()
{
  memset(_delayBuffer.data(), 0, _delayMax * sizeof(float));
  _pos = 0;
  _filt0 = 0.0f;
}

void MDADelayAudioProcessor::update()
{
  // The delay length should be between 0 and _delayMax samples. The parameter
  // goes from 0 - 1. To calculate the actual delay length, the parameter value
  // is squared. This makes it easier to pick smaller delay sizes. For example,
  // at a sample rate of 44100, the maximum delay length is 22050 samples. With
  // the parameter set to 0.5, the delay is not 11025 (= half) but 5512 samples
  // (= half squared). This is similar to setting a skew factor on the slider.
  float ldelParam = apvts.getRawParameterValue("L Delay")->load();
  _ldel = int(_delayMax * ldelParam * ldelParam);

  // Actually make the minimum delay time 4 samples, not 0. A delay time of 0
  // would be equal to the maximum delay because of wrap-around, so that's not
  // very useful. Although I'm not sure why the minimum delay is 4 samples and
  // not 1. Notice that really short delays introduce a filtering effect.
  if (_ldel < 4) _ldel = 4;

  // The right channel delay is a percentage of the left channel delay length.
  // Moving the slider to the left gives you a variable ratio between 0% and
  // 200%. Moving the slider to the right chooses from a set of fixed ratios.
  // At the center position, the ratio is 200%. To give right the same delay
  // as left, put the slider roughly on 0.75.
  float rdelParam = apvts.getRawParameterValue("R Delay")->load();
  float tmp;
  switch (int(rdelParam * 17.9f)) {          // fixed left/right ratios
    case 17: tmp = 0.5000f; break;
    case 16: tmp = 0.6667f; break;
    case 15: tmp = 0.7500f; break;
    case 14: tmp = 0.8333f; break;
    case 13: tmp = 1.0000f; break;
    case 12: tmp = 1.2000f; break;
    case 11: tmp = 1.3333f; break;
    case 10: tmp = 1.5000f; break;
    case  9: tmp = 2.0000f; break;
    default: tmp = 4.0f * rdelParam; break;  // variable ratio (param < 0.5)
  }
  _rdel = int(_delayMax * ldelParam * ldelParam * tmp);

  // Make sure the delay time does not become too large or too small.
  if (_rdel > _delayMax) _rdel = _delayMax;
  if (_rdel < 4) _rdel = 4;

  // The "tone" control goes from -100 to +100, where moving the control to the
  // left means low-pass filtering and to the right means high-pass filtering.
  // When the tone control is centered, there is no filtering.
  float toneParam = apvts.getRawParameterValue("Fb Tone")->load();
  _filt = toneParam / 200.0f + 0.5f;

  // Set the crossover frequency & high/low mix. Here, lmix determines how much
  // the low-pass filtered sample is combined with the unfiltered sample, whose
  // proportion is given by hmix.
  if (_filt > 0.5f) {               // high-pass:
    _filt = 0.5f * _filt - 0.25f;   // filt now goes 0 to 0.25
    _lmix = -2.0f * _filt;          // lmix goes from 0 to -0.5
    _hmix = 1.0f;
  } else {                          // low-pass:
    _hmix = 2.0f * _filt;           // hmix goes from 0 to 1
    _lmix = 1.0f - _hmix;           // lmix goes from 1 to 0
  }

  // At this point, _filt is a value between 0 and 0.5 (low-pass) or 0.25
  // (high-pass). Turn this value into a cutoff frequency. On the left of
  // the slider, the frequency goes from 158 Hz - 28 kHz. On the right, it
  // goes from 158 Hz - 2113 Hz.
  float hz = std::pow(10.0f, 2.2f + 4.5f * _filt);

  // The filter itself is a one-pole filter: y(n) = (1 - f)*x(n) + f*y(n - 1).
  // Calculate the coefficient f using the formula exp(-2pi * hz / sampleRate).
  _filt = std::exp(-6.2831853f * hz / (float)getSampleRate());

  // Feedback: value between 0 and 0.49. If this is 0, the delay repeats only
  // once. For higher values, the delay will keeping echoing.
  float feedbackParam = apvts.getRawParameterValue("Feedback")->load() / 100.0f;
  _feedback = 0.495f * feedbackParam;

  // Output gain is in decibels, so convert to a linear value.
  float gain = apvts.getRawParameterValue("Output")->load();
  gain = juce::Decibels::decibelsToGain(gain);

  // For the wet/dry mix, don't do a linear interpolation but use a curve:
  // dry = (1 - x^2) and wet = (1 - (1 - x)^2). Each curve is approx -3 dB at
  // mix = 0.5, but added together they increase the gain by +3 dB at 50% mix
  // (i.e. it's not an equal power curve).
  float mix = apvts.getRawParameterValue("FX Mix")->load() / 100.0f;
  _wet = gain * (1.0f - (1.0f - mix) * (1.0f - mix)) * 0.5f;
  _dry = gain * (1.0f - mix * mix);

  // Note: the original plug-in had an additional factor 2.0 in the formula for
  // dry, which seems wrong. That would boost the signal by 6 dB if the mix is
  // all dry (i.e. mix = 0), so I removed it. However, during processing we add
  // the left and right signals to create a mono signal, which boosts the level
  // by 6 dB if the mix is all wet (i.e. when mix = 1). To compensate for this,
  // I'm lowering the wet level by -6 dB; that's what the * 0.5 is for. This is
  // probably why dry originally had that extra factor 2. Now the volume of all
  // dry vs. all wet is the same again but at 0 dB instead of +6 dB.
}

void MDADelayAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear any output channels that don't contain input data.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  // Only recalculate when a parameter has changed.
  bool expected = true;
  if (_parametersChanged.compare_exchange_strong(expected, false)) {
    update();
  }

  const float *in1 = buffer.getReadPointer(0);
  const float *in2 = buffer.getReadPointer(1);
  float *out1 = buffer.getWritePointer(0);
  float *out2 = buffer.getWritePointer(1);

  const float wet = _wet;
  const float dry = _dry;
  const float fb = _feedback;
  const float f = _filt;
  const float lmix = _lmix;
  const float hmix = _hmix;
  const int s = _delayMax;

  float f0 = _filt0;

  // This keeps track of where we will write new values in the delay buffer.
  int p = _pos;

  // Get the read positions for the left and right channels. These need to
  // wrap around when they reach the end of the buffer, which is why we use
  // the modulo operator.
  int l = (p + _ldel) % s;
  int r = (p + _rdel) % s;

  // Note: This logic cannot tell apart a delay of 0 from a delay of _delayMax
  // samples, since due to the modulo operation these point to the same index
  // in the delay buffer. So a delay of 0 samples is not possible. The original
  // plug-in didn't do this, but you could allow the user to set the delay time
  // to 0 and then bypass all of the delay logic in the loop.

  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    float a = in1[i];
    float b = in2[i];

    // Read from the delay buffer.
    float dl = _delayBuffer[l];
    float dr = _delayBuffer[r];

    // Combine the left and right input samples into a mono signal.
    // Also add the delayed values but attenuated by the feedback factor.
    // The larger the feedback, the longer the sound will keep echoing.
    float tmp = wet * (a + b) + fb * (dl + dr);

    // Apply the low-pass filter. As seen in the other MDA plug-ins, this is
    // a simple exponentially weighted moving average filter (EWMA).
    f0 = f * (f0 - tmp) + tmp;

    // Do the crossover mix and write the new value back into the delay line.
    //
    // How this mix works: lmix determines how important the low-pass filtered
    // value is, while hmix says how important the unfiltered sample value is.
    //
    // When the tone control is on the left, hmix is (1 - lmix). The further to
    // the left, the larger lmix and the smaller hmix, meaning that the low-pass
    // filtered sample value is more now important. The closer the control is to
    // the center, the larger hmix and the less important the low-pass filtered
    // value becomes.
    //
    // When the tone control is on the right, hmix is 1 but lmix is negative,
    // which means the low-pass filtered value is subtracted from the original,
    // turning this into a high-pass filter. The more to the right the control
    // is, the more this happens.
    //
    // The high-pass mode is more subtle than the low-pass mode since the
    // maximum cutoff frequency is lower (it's mostly useful for getting rid
    // of a muddy low end).
    //
    // Note that the filtering only applies to the delayed signal, not to the
    // original (dry) signal.
    _delayBuffer[p] = lmix * f0 + hmix * tmp;

    // Update the locations where we will write into and read from the delay
    // buffer. Since l = p + ldel and r = p + rdel, it means older / delayed
    // values are to the right in the buffer (higher indices) and newer values
    // are to the left (lower indices). Therefore, we must decrement and wrap
    // around when we have reached the end (well, beginning) of the buffer.
    p--; if (p < 0) p = s - 1;
    l--; if (l < 0) l = s - 1;
    r--; if (r < 0) r = s - 1;

    // The output for this sample is the original sample mixed with the values
    // we read from the delay buffer. Note that the output is still stereo but
    // the delayed signal that gets mixed into the stream is always mono.
    out1[i] = dry * a + dl;
    out2[i] = dry * b + dr;
  }

  _pos = p;

  // Trap denormals
  if (std::abs(f0) < 1.0e-10f) _filt0 = 0.0f; else _filt0 = f0;
}

juce::AudioProcessorEditor *MDADelayAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDADelayAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDADelayAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDADelayAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // The original plug-in displays the left channel delay time in milliseconds.
  // To calculate that, we need access to the project's sample rate, which we
  // don't have in this static function. Here we simply use the naked parameter
  // value from 0.0 to 1.0, where 0 means no delay and 1 means maximum delay.
  // Showing the delay in milliseconds is nicer for the user, but that requires
  // making a custom UI.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "L Delay",
    "L Delay",
    juce::NormalisableRange<float>(0.0f, 1.0f),
    0.5f));

  // The original plug-in displays the right channel delay time as a percentage
  // of the left channel delay time. Moving the slider to the left gives you a
  // variable ratio, to the right gives a choice between several fixed ratios.
  // Here, we just use a parameter from 0 - 1 where 0.5 means 200% and dragging
  // the slider to the left or the right makes this percentage smaller (but in
  // different ways).

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "R Delay",
    "R Delay",
    juce::NormalisableRange<float>(0.0f, 1.0f),
    0.27f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Feedback",
    "Feedback",
    juce::NormalisableRange<float>(0.0f, 99.0f, 0.01f),
    70.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Fb Tone",
    "Fb Tone",
    juce::NormalisableRange<float>(-100.0f, 100.0f, 0.01f),
    0.0f,
    "Lo <> Hi"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "FX Mix",
    "FX Mix",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
    33.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Output",
    "Output",
    juce::NormalisableRange<float>(-30.0f, 6.0f, 0.01f),
    0.0f,
    "dB"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDADelayAudioProcessor();
}
