#include "PluginProcessor.h"

MDAStereoAudioProcessor::MDAStereoAudioProcessor()
  : AudioProcessor(BusesProperties()
                   .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                   .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  apvts.state.addListener(this);
}

MDAStereoAudioProcessor::~MDAStereoAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDAStereoAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDAStereoAudioProcessor::getNumPrograms()
{
  return 1;
}

int MDAStereoAudioProcessor::getCurrentProgram()
{
  return 0;
}

void MDAStereoAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDAStereoAudioProcessor::getProgramName(int index)
{
  return {};
}

void MDAStereoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAStereoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  // Store this in a variable so we can use it to format the parameters.
  _sampleRate = sampleRate;

  // Allocate the delay buffer. Because the maximum values of the Delay and Mod
  // parameters are 2100 samples, the maximum delay length is 4200 samples. The
  // delay buffer should be at least that size.
  // As explained in createParameterLayout(), expressing Delay and Mod in terms
  // of samples isn't the best choice, and this should really be independent of
  // the current sample rate.
  _delayMax = 4800;
  _delayBuffer.resize(_delayMax);

  resetState();
  _parametersChanged.store(true);
}

void MDAStereoAudioProcessor::releaseResources()
{
}

void MDAStereoAudioProcessor::reset()
{
  resetState();
}

bool MDAStereoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAStereoAudioProcessor::resetState()
{
  _phase = 0.0f;
  _writePos = 0;

  // Clear out the delay buffer.
  memset(_delayBuffer.data(), 0, _delayMax * sizeof(float));
}

void MDAStereoAudioProcessor::update()
{
  // The Rate parameter is displayed as going from 100 to 0.1 sec. That makes
  // it a period. Here, we convert the 0 - 1 value into 0.01 - 10 Hz, which is
  // a frequency, or 1 divided by the period that's being displayed.
  // The formula to go from a frequency to radians is 2*pi*freq / sampleRate,
  // but here the factor 2 is missing. That's probably because the waveform for
  // modulation isn't a regular sine wave but the absolute value of the sine,
  // which has twice the frequency. So we compensate for that factor 2 here.
  float fParam5 = apvts.getRawParameterValue("Rate")->load();
  _phaseInc = 3.141f * std::pow(10.0f, -2.0f + 3.0f * fParam5) / _sampleRate;

  // Delay time is between 20 and 2100 samples.
  float fParam2 = apvts.getRawParameterValue("Delay")->load();
  _delayTime = 20.0f + 2080.0f * std::pow(fParam2, 2.0f);

  // Modulation amount goes from 0 to 2100. In the audio callback, this is
  // added to the number of samples from the delay time. So the total delay
  // buffer size must be 2100 + 2100 = 4200. It's actually slightly larger.
  float fParam4 = apvts.getRawParameterValue("Mod")->load();
  _mod = 2100.0f * std::pow(fParam4, 2.0f);

  // The Width parameter is split into two halves that both go from 0 to 100.
  // The left half is for Haas panning; the right half is for comb filtering.
  float fParam1 = apvts.getRawParameterValue("Width")->load();
  if (fParam1 < 0.5f) {
    /*
      Haas panning: put the same sound on both sides but apply a short delay to
      only one of the sides (the right channel). This makes the sound appear to
      come from the left, so make the left channel quieter to compensate.

      If the Width parameter = 0 ("100 Haas" in the UI), the right channel is a
      purely delayed version of the input. As width goes from 0.0 to 0.5, the
      right channel acts more like a comb filter (see below).

      In Haas mode, only the signal in the right channel is modulated, since
      that is the only channel that uses the delay buffer.
    */
    _fli = 0.25f + 1.5f * fParam1;  // 0.25 - 1.0
    _fld = 0.0f;
    _fri = 2.0f * fParam1;          // 0.0 - 1.0
    _frd = 1.0f - _fri;             // 1.0 - 0.0
  } else {
    /*
      Comb filter mode:

      Summing the input with a delayed version of itself creates a so-called
      comb filter.

      This plugin uses a feed-forward comb filter with the difference equation:
        y(n) = fli * x(n) - fld * x(n - delay)  for the left channel, and
        y(n) = fri * x(n) - frd * x(n - delay)  for the right channel.

      Note that this formula is actually subtracting, not summing. However,
      that is only true for the left channel; for the right channel `frd` will
      be negative. This is what causes a 90-degree phase shift between the left
      and right channels.

      How this works: the amount of delay determines which frequencies get
      boosted and which get attenuated. Any frequencies that are a multiple of
      `sampleRate / delay` Hz will be doubled, while other frequencies get more
      suppressed the further away they are from one of these multiples.

      Because the left channel subtracts the delayed input rather than adds,
      its filtering frequencies are shifted by half the comb width. So when a
      frequency is boosted on the right channel, it is attenuated on the left
      channel, and vice versa. This is what pulls the sound apart into separate
      channels.

      The smaller delay is, the fewer combs there are in the spectrum and the
      coarser / softer this separation is. With a longer delay, there are more
      combs and the separation effect is more extreme. You can also start to
      hear the echo now.

      Modulating the delay length will change which frequencies get panned left
      vs. right, and it makes the sound appear to move between left and right.

      The Width parameter determines the mix between the current input sample
      and the delayed sample. If width = 0.5 then fli and fri are 1.0, and fld
      and frd are 0.0, so no actual filtering takes place. For larger values of
      width, fli and fri go down, while fld / frd go up. It's necessary to mix
      the two signals in this way otherwise their sum would have an amplitude
      that is twice as large as the input sound.
    */
    _fli = 1.5f - fParam1;  // 1.0 - 0.5
    _fld = 1.0f - _fli;     // 0.0 - 0.5
    _fri = _fli;
    _frd = -(_fld);
  }

  // If the Balance parameter is to the right, then attentuate the left filter
  // coefficients, and vice versa. This is a simple linear mix. It's especially
  // useful in Haas mode because that makes one side louder than the other.
  float fParam3 = apvts.getRawParameterValue("Balance")->load();
  if (fParam3 > 0.5f) {
    _fli *= (1.0f - fParam3) * 2.0f;
    _fld *= (1.0f - fParam3) * 2.0f;
  } else {
    _fri *= 2.0f * fParam3;
    _frd *= 2.0f * fParam3;
  }

  // I'm not entirely sure where this comes from, but it makes the mixing curve
  // between the current sample and the delayed sample less linear.
  _fri *= 0.5f + std::abs(fParam1 - 0.5f);
  _frd *= 0.5f + std::abs(fParam1 - 0.5f);
  _fli *= 0.5f + std::abs(fParam1 - 0.5f);
  _fld *= 0.5f + std::abs(fParam1 - 0.5f);

  // The output level is in decibels, so convert to a linear gain.
  float level = apvts.getRawParameterValue("Output")->load();
  _gain = juce::Decibels::decibelsToGain(level);
}

void MDAStereoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  const int delayMax = _delayMax;
  const float delayTime = _delayTime;
  const float mod = _mod;
  const float phaseInc = _phaseInc;
  const float fli = _fli;
  const float fld = _fld;
  const float fri = _fri;
  const float frd = _frd;

  float phase = _phase;
  int writePos = _writePos;

  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    // Sum the stereo channels into a mono signal. The goal of this plug-in
    // is to take a mono sound and widen it, but in its current form it only
    // accepts input from a stereo bus. Since the input is mono(-ish), it's
    // fair to assume both channels are mostly the same anyway.
    float a = in1[i] + in2[i];

    // Write the current input into the delay buffer. Note: because the delay
    // buffer is slightly larger than the maximum delay time this works fine,
    // but it would be better to first read from the buffer, then write into
    // it. Just in case the readPos wraps around to be the same as writePos.
    _delayBuffer[writePos] = a;

    // Figure out where to read from the delay buffer. We will read at least
    // delayTime samples after the write position. If modulation is enabled,
    // add an additional amount to the delay time (unipolar modulation) that
    // varies in the shape of a sine wave (but only the positive half).
    float readOffset = delayTime;
    if (mod > 0.0f) {
      readOffset += std::abs(mod * std::sin(phase));
      phase += phaseInc;
    }

    // Read a delayed sample from the delay buffer.
    int readPos = (writePos + int(readOffset)) % delayMax;
    float b = _delayBuffer[readPos];

    // Apply the filter. Each channel has its own filter coefficients, which is
    // what creates the stereo effect.
    float c = (a * fli) - (b * fld);   // left channel
    float d = (a * fri) - (b * frd);   // right channel

    // Update the write position for the next sample. Wrap around when we
    // reach the edge of the buffer.
    writePos -= 1;
    if (writePos < 0) writePos += delayMax;

    // The original plug-in didn't have this but I found that it's possible to
    // make the sound become too loud, so I added an output level parameter to
    // compensate for this.
    out1[i] = c * _gain;
    out2[i] = d * _gain;
  }

  _writePos = writePos;

  // Wrap-around the phase so it stays within 0 - 2pi. It's possible the phase
  // already goes over 2pi in the loop, but that's OK, the sin() function can
  // handle this. We just don't want it to grow too big over time.
  _phase = std::fmod(phase, 6.2831853f);
}

juce::AudioProcessorEditor *MDAStereoAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDAStereoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAStereoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAStereoAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Width",
    "Width",
    juce::NormalisableRange<float>(),
    0.78f,
    "",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      auto s = juce::String(int(200.0f * std::abs(value - 0.5f)));
      return s + (value < 0.5 ? " Haas" : " Comb");
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Balance",
    "Balance",
    juce::NormalisableRange<float>(),
    0.5f,
    "",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(200.0f * std::abs(value - 0.5f)));
    }));

  // The Delay and Mod parameters are displayed as milliseconds but the plug-in
  // actually treats them as number of samples (between 20 and 2100 samples for
  // Delay and 0 - 2100 samples for Mod). That's not ideal because it means the
  // max delay time becomes shorter when the project's sample rate is higher.
  // I think the assumption was that the sample rate would be at most 44100 or
  // 48000 Hz. It would better to let the user choose the delay and modulation
  // depth in milliseconds instead.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Delay",
    "Delay",
    juce::NormalisableRange<float>(),
    0.43f,
    "ms",
    juce::AudioProcessorParameter::genericParameter,
    [this](float value, int) {
      float x = 20.0f + 2080.0f * std::pow(value, 2.0f);
      return juce::String(1000.0f * x / _sampleRate);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod",
    "Mod",
    juce::NormalisableRange<float>(),
    0.0f,
    "ms",
    juce::AudioProcessorParameter::genericParameter,
    [this](float value, int) {
      float x = 2100.0f * std::pow(value, 2.0f);
      if (x > 0.0f) {
        return juce::String(1000.0f * x / _sampleRate);
      } else {
        return juce::String("OFF");
      }
    }));

  // The Rate parameter is displayed as going from 100 to 0.1 sec in a
  // logaritmic fashion. This is the period of the sine wave that's used for
  // modulation, so 1 / frequency. Rather than doing pow(10, 2 - 3 * value),
  // with value going from 0 to 1, we could use a NormalisableRange from 100
  // to 0.1 with a skew factor. Or just display in Hz instead.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Rate",
    "Rate",
    juce::NormalisableRange<float>(),
    0.5f,
    "sec",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(std::pow(10.0f, 2.0f - 3.0f * value));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Output",
    "Output",
    juce::NormalisableRange<float>(-48.0, 0.0f, 0.1f),
    -6.0f,
    "dB"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDAStereoAudioProcessor();
}
