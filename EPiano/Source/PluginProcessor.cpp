#include "PluginProcessor.h"
#include "mdaEPianoData.h"

MDAEPianoProgram::MDAEPianoProgram(const char *name,
                                   float p0, float p1, float p2, float p3,
                                   float p4, float p5, float p6, float p7,
                                   float p8, float p9, float p10, float p11)
{
  strcpy(this->name, name);
  param[0] = p0; param[1] = p1; param[2]  = p2;  param[3]  = p3;
  param[4] = p4; param[5] = p5; param[6]  = p6;  param[7]  = p7;
  param[8] = p8; param[9] = p9; param[10] = p10; param[11] = p11;
}

MDAEPianoAudioProcessor::MDAEPianoAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  // Just in case...
  _sampleRate = 44100.0f;
  _inverseSampleRate = 1.0f / _sampleRate;

  createPrograms();
  setCurrentProgram(0);

  _waves = epianoData;

  // Fill it with zeros, just to make sure it's empty.
  memset(_keygroups, 0, sizeof(_keygroups));

  // Waveform data and keymapping
  _keygroups[ 0].root = 36; _keygroups[ 0].high = 39;  // C1
  _keygroups[ 3].root = 43; _keygroups[ 3].high = 45;  // G1
  _keygroups[ 6].root = 48; _keygroups[ 6].high = 51;  // C2
  _keygroups[ 9].root = 55; _keygroups[ 9].high = 57;  // G2
  _keygroups[12].root = 60; _keygroups[12].high = 63;  // C3
  _keygroups[15].root = 67; _keygroups[15].high = 69;  // G3
  _keygroups[18].root = 72; _keygroups[18].high = 75;  // C4
  _keygroups[21].root = 79; _keygroups[21].high = 81;  // G4
  _keygroups[24].root = 84; _keygroups[24].high = 87;  // C5
  _keygroups[27].root = 91; _keygroups[27].high = 93;  // G5
  _keygroups[30].root = 96; _keygroups[30].high = 999; // C6

  _keygroups[ 0].pos = 0;      _keygroups[ 0].end = 8476;   _keygroups[ 0].loop = 4400;
  _keygroups[ 1].pos = 8477;   _keygroups[ 1].end = 16248;  _keygroups[ 1].loop = 4903;
  _keygroups[ 2].pos = 16249;  _keygroups[ 2].end = 34565;  _keygroups[ 2].loop = 6398;
  _keygroups[ 3].pos = 34566;  _keygroups[ 3].end = 41384;  _keygroups[ 3].loop = 3938;
  _keygroups[ 4].pos = 41385;  _keygroups[ 4].end = 45760;  _keygroups[ 4].loop = 1633;  // was 1636
  _keygroups[ 5].pos = 45761;  _keygroups[ 5].end = 65211;  _keygroups[ 5].loop = 5245;
  _keygroups[ 6].pos = 65212;  _keygroups[ 6].end = 72897;  _keygroups[ 6].loop = 2937;
  _keygroups[ 7].pos = 72898;  _keygroups[ 7].end = 78626;  _keygroups[ 7].loop = 2203;  // was 2204
  _keygroups[ 8].pos = 78627;  _keygroups[ 8].end = 100387; _keygroups[ 8].loop = 6368;
  _keygroups[ 9].pos = 100388; _keygroups[ 9].end = 116297; _keygroups[ 9].loop = 10452;
  _keygroups[10].pos = 116298; _keygroups[10].end = 127661; _keygroups[10].loop = 5217;  // was 5220
  _keygroups[11].pos = 127662; _keygroups[11].end = 144113; _keygroups[11].loop = 3099;
  _keygroups[12].pos = 144114; _keygroups[12].end = 152863; _keygroups[12].loop = 4284;
  _keygroups[13].pos = 152864; _keygroups[13].end = 173107; _keygroups[13].loop = 3916;
  _keygroups[14].pos = 173108; _keygroups[14].end = 192734; _keygroups[14].loop = 2937;
  _keygroups[15].pos = 192735; _keygroups[15].end = 204598; _keygroups[15].loop = 4732;
  _keygroups[16].pos = 204599; _keygroups[16].end = 218995; _keygroups[16].loop = 4733;
  _keygroups[17].pos = 218996; _keygroups[17].end = 233801; _keygroups[17].loop = 2285;
  _keygroups[18].pos = 233802; _keygroups[18].end = 248011; _keygroups[18].loop = 4098;
  _keygroups[19].pos = 248012; _keygroups[19].end = 265287; _keygroups[19].loop = 4099;
  _keygroups[20].pos = 265288; _keygroups[20].end = 282255; _keygroups[20].loop = 3609;
  _keygroups[21].pos = 282256; _keygroups[21].end = 293776; _keygroups[21].loop = 2446;
  _keygroups[22].pos = 293777; _keygroups[22].end = 312566; _keygroups[22].loop = 6278;
  _keygroups[23].pos = 312567; _keygroups[23].end = 330200; _keygroups[23].loop = 2283;
  _keygroups[24].pos = 330201; _keygroups[24].end = 348889; _keygroups[24].loop = 2689;
  _keygroups[25].pos = 348890; _keygroups[25].end = 365675; _keygroups[25].loop = 4370;
  _keygroups[26].pos = 365676; _keygroups[26].end = 383661; _keygroups[26].loop = 5225;
  _keygroups[27].pos = 383662; _keygroups[27].end = 393372; _keygroups[27].loop = 2811;
  _keygroups[28].pos = 383662; _keygroups[28].end = 393372; _keygroups[28].loop = 2811;  // ghost
  _keygroups[29].pos = 393373; _keygroups[29].end = 406045; _keygroups[29].loop = 4522;
  _keygroups[30].pos = 406046; _keygroups[30].end = 414486; _keygroups[30].loop = 2306;
  _keygroups[31].pos = 406046; _keygroups[31].end = 414486; _keygroups[31].loop = 2306;  // ghost
  _keygroups[32].pos = 414487; _keygroups[32].end = 422408; _keygroups[32].loop = 2169;

  // For more seamless looping, it can be a good idea to make the end and start
  // of the loop cross-fade into each other. This code changes the waveforms in
  // the lookup table.
  for (int k = 0; k < 28; ++k) {
    int p0 = _keygroups[k].end;
    int p1 = _keygroups[k].end - _keygroups[k].loop;

    float xf = 1.0f;
    float dxf = -0.02f;
    while (xf > 0.0f) {
      _waves[p0] = short((1.0f - xf) * float(_waves[p0]) + xf * float(_waves[p1]));
      p0--;
      p1--;
      xf += dxf;
    }
  }

  apvts.state.addListener(this);
}

MDAEPianoAudioProcessor::~MDAEPianoAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDAEPianoAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDAEPianoAudioProcessor::getNumPrograms()
{
  return NPROGS;
}

int MDAEPianoAudioProcessor::getCurrentProgram()
{
  return _currentProgram;
}

void MDAEPianoAudioProcessor::setCurrentProgram(int index)
{
  _currentProgram = index;

  const char *paramNames[] = {
    "Envelope Decay",
    "Envelope Release",
    "Hardness",
    "Treble Boost",
    "Modulation",
    "LFO Rate",
    "Velocity Sensitivity",
    "Stereo Width",
    "Polyphony",
    "Fine Tuning",
    "Random Tuning",
    "Overdrive",
  };

  for (int i = 0; i < NPARAMS; ++i) {
    apvts.getParameter(paramNames[i])->setValueNotifyingHost(_programs[index].param[i]);
  }
}

const juce::String MDAEPianoAudioProcessor::getProgramName(int index)
{
  return { _programs[index].name };
}

void MDAEPianoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
  // not implemented
}

void MDAEPianoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  _sampleRate = sampleRate;
  _inverseSampleRate = 1.0f / _sampleRate;

  resetState();
  _parametersChanged.store(true);
}

void MDAEPianoAudioProcessor::releaseResources()
{
}

void MDAEPianoAudioProcessor::reset()
{
  resetState();
}

bool MDAEPianoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAEPianoAudioProcessor::createPrograms()
{
  // Note that the presets store the parameters as values between 0 and 1.
  // This is because the original plugin was for VST2, which always uses
  // parameters in that range. Our parameters have different ranges, for
  // example from -50 to +50 cents, but the underlying normalized values
  // should still match those from the original version and therefore the
  // presets still work!

  _programs.emplace_back("Default",   0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f);
  _programs.emplace_back("Bright",    0.500f, 0.500f, 1.000f, 0.800f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.500f);
  _programs.emplace_back("Mellow",    0.500f, 0.500f, 0.000f, 0.000f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
  _programs.emplace_back("Autopan",   0.500f, 0.500f, 0.500f, 0.500f, 0.250f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
  _programs.emplace_back("Tremolo",   0.500f, 0.500f, 0.500f, 0.500f, 0.750f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
  _programs.emplace_back("(default)", 0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f);
  _programs.emplace_back("(default)", 0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f);
  _programs.emplace_back("(default)", 0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f);
}

void MDAEPianoAudioProcessor::resetState()
{
  // Turn off all playing voices.
  for (int v = 0; v < NVOICES; ++v) {
    _voices[v].env = 0.0f;
    _voices[v].decay = 0.99f;   // very quick fade out
  }
  _numActiveVoices = 0;

  // Clear out any pending MIDI events.
  _notes[0] = EVENTS_DONE;

  // These variables are changed by MIDI CC, reset to defaults.
  _volume = 0.2f;
  _sustain = 0;

  // Zero out the filter delay units.
  _filtL = _filtR = 0.0f;

  // Reset the LFOs.
  _lfo0 = 0.0f;
  _lfo1 = 1.0f;
}

void MDAEPianoAudioProcessor::update()
{
  // Envelope Decay: The UI shows this as 0% - 100%. Divide it by 100 to turn
  // it into a value between 0 and 1. The smaller the percentage, the quicker
  // the note decays. The actual decay value is calculated in noteOn().
  _envDecay = apvts.getRawParameterValue("Envelope Decay")->load() / 100.0f;

  // Envelope Release. The UI shows this as a percentage, so divide by 100.
  float param1 = apvts.getRawParameterValue("Envelope Release")->load();
  _envRelease = param1 / 100.0f;

  // Hardness Offset: The UI shows -50% to +50%. Here we turn this into an
  // integer between -6 and +6. This is the number of semitones that the MIDI
  // note number gets shifted by, which changes the waveform we select for it.
  float param2 = apvts.getRawParameterValue("Hardness")->load();
  param2 = (param2 + 50.0f) / 100.0f;  // first to 0 - 1
  _hardnessOffset = int(12.0f * param2 - 6.0f);

  // Treble Boost: The UI shows -50% to +50%. When negative, this acts as a
  // low-shelf filter that suppresses the high frequencies. When positive,
  // this is a high-shelf filter that boosts the high frequencies.
  float param3 = apvts.getRawParameterValue("Treble Boost")->load();
  param3 = (param3 + 50.0f) / 100.0f;           // first to 0 - 1
  _trebleGain = 4.0f * param3 * param3 - 1.0f;  // -1 ... +3

  // The treble boost consists of a basic low-pass filter. Here, we calculate
  // the coefficient for that filter. It either has a cutoff of about 794 Hz
  // (slider to the left) or 2241 Hz (slider to the right). Note that the code
  // says 14000 or 5000 but that includes an extra factor 2pi.
  if (param3 > 0.5f) _filtCoef = 14000.0f; else _filtCoef = 5000.0f;
  _filtCoef = 1.0f - std::exp(-_inverseSampleRate * _filtCoef);

  // Modulation: The UI shows 100% panning modulation on the left, 0% in the
  // center, and 100% tremolo on the right. For tremolo, both the left and
  // right channel use the same LFO depth (this is a number between 0 and 1).
  // For panning modulation, the LFO depth for the left channel is between 0
  // and -1; the right channel is the opposite and goes between 0 and +1.
  _modulation = apvts.getRawParameterValue("Modulation")->load();
  _rmod = _lmod = _modulation + _modulation - 1.0f;
  if (_modulation < 0.5f) _rmod = -_rmod;  // for panning only

  // LFO Rate: The UI shows this as a frequency in Hz, between 0.07 and 37 Hz.
  // The parameter itself is between 0 and 1, and the exp(...) turns that into
  // a frequency. The _lfoRate is the corresponding phase increment, given by
  // the usual formula: 2pi * freq / sampleRate.
  float param5 = apvts.getRawParameterValue("LFO Rate")->load();
  _lfoRate = 6.283f * _inverseSampleRate * std::exp(6.22f * param5 - 2.61f);

  // Velocity Sensitivity: The UI shows 0% to 100%. Convert this into a value
  // between 0.25 and 3.0. There are actually two curves: 0% to 25% is a steep
  // line from 0.25 to 1.5; 25% to 100% is a slightly less steep line that goes
  // from 1.5 to 3.0.
  float param6 = apvts.getRawParameterValue("Velocity Sensitivity")->load() / 100.0f;
  _velocitySensitivity = 1.0f + param6 + param6;
  if (param6 < 0.25f) _velocitySensitivity -= 0.75f - 3.0f * param6;

  // Stereo Width: The UI shows 0% to 200%. Turn this into 0.0 - 0.03.
  float param7 = apvts.getRawParameterValue("Stereo Width")->load() / 200.0f;
  _width = 0.03f * param7;

  // Polyphony is an integer number between 1 and 32.
  _polyphony = int(apvts.getRawParameterValue("Polyphony")->load());

  // Fine Tuning: The UI shows -50 to +50 cents. Convert this into -0.5 to
  // +0.5, which turns it from cents into semitones.
  _fine = apvts.getRawParameterValue("Fine Tuning")->load() / 100.0f;

  // Random Detuning: The UI shows 0 to 50 cents. Convert this into 0 - 0.077.
  // This will be turned into (a fraction of) semitones later.
  float param10 = apvts.getRawParameterValue("Random Tuning")->load();
  _random = 0.077f * param10 * param10;

  // Overdrive: The UI shows 0% to 100%. Convert this into 0 - 1.8.
  float param11 = apvts.getRawParameterValue("Overdrive")->load() / 100.0f;
  _overdrive = 1.8f * param11;
}

void MDAEPianoAudioProcessor::processEvents(juce::MidiBuffer &midiMessages)
{
  // There are different ways a synth can handle MIDI events. This plug-in does
  // it by copying the events it cares about -- note on and note off -- into an
  // array. In the render loop, we step through this array to process the events
  // one-by-one. Controllers such as the sustain pedal are processed right away,
  // at the start of the block, so these are not sample accurate.

  int npos = 0;
  for (const auto metadata : midiMessages) {
    if (metadata.numBytes != 3) continue;

    const auto data0 = metadata.data[0];
    const auto data1 = metadata.data[1];
    const auto data2 = metadata.data[2];
    const int deltaFrames = metadata.samplePosition;

    switch (data0 & 0xf0) {  // status byte (all channels)
      // Note off
      case 0x80:
        _notes[npos++] = deltaFrames;   // time offset
        _notes[npos++] = data1 & 0x7F;  // note
        _notes[npos++] = 0;             // vel
        break;

      // Note on
      case 0x90:
        _notes[npos++] = deltaFrames;   // time offset
        _notes[npos++] = data1 & 0x7F;  // note
        _notes[npos++] = data2 & 0x7F;  // vel
        break;

      // Controller
      case 0xB0:
        switch (data1) {
          case 0x01: { // mod wheel
            // Convert the mod wheel position to a value between 0 and 1.
            float modwhl = 0.0078f * float(data2);

            // Override autopan/tremolo depth if the mod wheel is used.
            // This overwrites the "Modulation" parameter. (It would be better
            // if the mod wheel value was stored in its own member variable,
            // and then we add that amount to the Modulation parameter amount.)
            if (modwhl > 0.05f) {
              _rmod = _lmod = modwhl;
              if (_modulation < 0.5f) _rmod = -_rmod;  // for panning mode
            }
            break;
          }

          case 0x07:  // volume
            // Map the position of the volume control to a parabolic curve
            // starting at 0.0 (position 0) up to 0.323 (position 127).
            _volume = 0.00002f * float(data2 * data2);
            break;

          case 0x40:  // sustain pedal
          case 0x42:  // sustenuto pedal
            // Make the variable 64 when the pedal is pressed and 0 when released.
            _sustain = data2 & 0x40;

            // Pedal released? Then end all sustained notes.
            if (_sustain == 0) {
              _notes[npos++] = deltaFrames;
              _notes[npos++] = SUSTAIN;
              _notes[npos++] = 0;
            }
            break;

          default:  // all notes off
            if (data1 > 0x7A) {
              // Setting the decay to 0.99 will fade out the voice very quickly.
              for (int v = 0; v < NVOICES; ++v) _voices[v].decay = 0.99f;
              _sustain = 0;
            }
            break;
        }
        break;

      // Program change
      case 0xC0:
        if (data1 < NPROGS) setCurrentProgram(data1);
        break;

      default: break;
    }

    // Discard events if buffer full!
    if (npos > EVENTBUFFER) npos -= 3;
  }
  _notes[npos] = EVENTS_DONE;
  midiMessages.clear();
}

void MDAEPianoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  processEvents(midiMessages);

  const int sampleFrames = buffer.getNumSamples();
  const float overdrive = _overdrive;

  float *out0 = buffer.getWritePointer(0);
  float *out1 = buffer.getWritePointer(1);

  int event = 0;
  int frame = 0;  // how many samples are already rendered

  while (frame < sampleFrames) {
    // Get the timestamp of the next note on/off event. This is usually in the
    // future, i.e. a number of samples after the current sample.
    int frames = _notes[event++];

    // This catches the EVENTS_DONE special event. There are no events left.
    if (frames > sampleFrames) frames = sampleFrames;

    // The timestamp for the event is relative to the start of the block.
    // Make it relative to the previous event; this tells us how many samples
    // to process until the new event actually happens.
    frames -= frame;

    // When we're done with this event, this is how many samples we will have
    // processed in total.
    frame += frames;

    // Until it's time to process the upcoming event, render the active voices.
    while (--frames >= 0) {
      Voice *V = _voices;

      // Accumulators for the left and right channel. We will add the outputs
      // of all the active voices to these.
      float l = 0.0f, r = 0.0f;

      for (int v = 0; v < _numActiveVoices; ++v) {
        // Increment the read position in the waveform. The read position is
        // split into `pos`, which is the integer part, and `frac`, which is
        // the fractional part. To read the next sample value, we move the read
        // position ahead by the step size `delta`, a fixed-point number, where
        // the lowest 16 bits are the fractional part.
        V->frac += V->delta;

        // If the fractional part of the read position is now more than 1.0,
        // or more than 65535, increment the integer part of the read position.
        V->pos += V->frac >> 16;

        // Remove the integer amount from `frac` (if any), since that just got
        // added to `pos`. This is the same as doing `frac modulo 65536`.
        V->frac &= 0xFFFF;

        // If the read position has reached the end of the sample, wrap it
        // around to where the loop begins. The attack portion of the sample is
        // played just once, and from then on we just keep looping this region.
        if (V->pos > V->end) V->pos -= V->loop;

        // Integer-based linear interpolation. Together, `pos` and `frac` will
        // point to a value in between two samples (unless frac is 0).
        // Suppose pos = 3 and frac = 0.6 (or really 65536 * 0.6 = 39321). Then
        // the interpolated sample should be 40% of the sample at index 3 and
        // 60% of the sample at index 4. That's exactly what the formula below
        // calculates: it takes the sample value at index 3, plus 0.6 times the
        // sample at index 4, minus 0.6 times the sample at index 3. The >> 16
        // is used to divide the result by 65536 because of how frac is stored.
        int i = _waves[V->pos] + ((V->frac * (_waves[V->pos + 1] - _waves[V->pos])) >> 16);

        // Apply the envelope and scale. The original sample data is 16-bit but
        // we're working with floats here so divide by 32768 as well.
        float x = V->env * float(i) / 32768.0f;

        // Update the envelope. Multiplying by a decay value that is less than
        // 1.0 gives this an exponentially decaying curve.
        V->env = V->env * V->decay;

        // Simple distortion effect. For samples that are positive, subtract
        // the square of that sample times the overdrive factor, which can be
        // larger than 1. This "flattens" the top of the waveform. The louder
        // you play, the more extreme the distortion is.
        if (x > 0.0f) {
          x -= overdrive * x * x;
          if (x < -V->env) x = -V->env;   // but not too extreme!
        }

        // Apply panning. The amount of panning was computed in noteOn().
        l += V->outl * x;
        r += V->outr * x;

        // Ear protection: just in case the sound explodes, turn it off. Silly
        // bugs (such as filter cutoff > Nyquist) can blow out your eardrums...
        if ((l < -2.0f) || (l > 2.0f)) {
          l = 0.0f;
        }
        if ((r < -2.0f) || (r > 2.0f)) {
          r = 0.0f;
        }

        // Go to the next active voice.
        V++;
      }

      // Treble boost. This happens in 2 steps: First there is a basic low-pass
      // filter with the difference equation y(n) = f*x(n) + (1 - f)*y(n - 1).
      // The left and right channels have their own instance of this filter.
      _filtL += _filtCoef * (l - _filtL);
      _filtR += _filtCoef * (r - _filtR);

      // Next, we subtract the low-pass filtered signal from the original signal,
      // which leaves only the high / treble frequencies. Then, depending on
      // whether the Treble Boost setting is + or -, we add or subtract these
      // high frequencies using the "treble gain" factor. This creates a shelf:
      // by subtracting the high freqs, we remove the high end (obviously).
      // But when we add the high frequencies, the high end gets boosted.
      l += _trebleGain * (l - _filtL);
      r += _trebleGain * (r - _filtR);

      // This formula creates a sine wave in _lfo0 and a cosine wave in _lfo1,
      // with amplitudes between -1 and +1. You might be wondering how, since
      // we're not calling sin or cos anywhere? Plot it in a Python notebook
      // and see for yourself. Note that this approximation only works on low
      // frequencies, so it's only suitable for LFOs.
      _lfo0 += _lfoRate * _lfo1;
      _lfo1 -= _lfoRate * _lfo0;

      // Apply the modulation to the left and right channels. Note that the
      // modulation is applied to the mix of all voices -- in more advanced
      // synths, each individual voice can have its own LFO.
      // If this is tremolo, then we change the amplitude of both channels by
      // the same amount. If it is panning modulation (autopanning), when the
      // left channel amplitude goes up, the right channel amplitude goes down,
      // and vice versa.
      l += l * _lmod * _lfo1;
      r += r * _rmod * _lfo1;

      // Write the result into the output buffer.
      *out0++ = l;
      *out1++ = r;
    }

    // It's time to handle the event. This starts the new note, or stops the
    // note if velocity is 0.
    if (frame < sampleFrames) {

      // Reset the LFO phase for tremolo when the voices have stopped playing.
      // The original plug-in had a comment here that said "reset LFO phase -
      // good idea?", so maybe it isn't. ;-)
      if (_numActiveVoices == 0 && _modulation > 0.5f) {
        _lfo0 = -0.7071f;
        _lfo1 = 0.7071f;
      }

      int note = _notes[event++];
      int vel  = _notes[event++];
      noteOn(note, vel);
    }
  }

  // Handle floating point underflow.
  if (std::fabs(_filtL) < 1.0e-10f) _filtL = 0.0f;
  if (std::fabs(_filtR) < 1.0e-10f) _filtR = 0.0f;

  // Turn off voices whose envelope has dropped below the minimum level.
  for (int v = 0; v < _numActiveVoices; ++v) {
    if (_voices[v].env < SILENCE) {
      _voices[v] = _voices[--_numActiveVoices];
    }
  }

  // Mark the events buffer as done.
  _notes[0] = EVENTS_DONE;
}

void MDAEPianoAudioProcessor::noteOn(int note, int velocity)
{
  if (velocity > 0) {
    // === Find voice ===

    // If max polyphony is not reached yet, use a free voice.
    int vl = 0;
    if (_numActiveVoices < _polyphony) {
      vl = _numActiveVoices;
      _numActiveVoices++;
    } else {
      // Otherwise, find the quietest voice to steal.
      float l = 99.0f;
      for (int v = 0; v < _polyphony; ++v) {
        if (_voices[v].env < l) {
          l = _voices[v].env;
          vl = v;
        }
      }
    }

    // === Calculate pitch ===

    // Create "random" detuning based on the note number. The same note will
    // always give the same amount of tuning, so it's not really random, but
    // it *is* different for different notes. This is an interesting but weird
    // looking function. Like many of the other formulas used by this plug-in,
    // plot them into a graphing calculator to make sense of how they work.
    // It returns numbers between -0.5 and +0.5 semitones (= ±50 cents).
    int k = (note - 60) * (note - 60);
    float r = _random * (float(k % 13) - 6.5f);

    // Add the fine-tuning amount. Note that `l` is in semitones. Both the fine
    // and random tuning are in fractions of a semitone.
    float l = _fine + r;

    // Next, we need to determine which waveform to use for this note. That is
    // described by the keygroups. The Hardness Offset parameter can be used to
    // tweak this, so that notes sound duller or brighter. (MDA Piano also has
    // a velocity-to-hardness setting so that loud notes sound brighter, but
    // EPiano does not.)
    int s = _hardnessOffset;

    // What keygroup is this note in? We effectively subtract the "hardness" s
    // from the note number, so if `s` is positive (more hardness), we pretend
    // we're playing a lower note. Pitched up to the actual note we're playing
    // makes this sound "harder" / brighter than the waveform from this note's
    // actual keygroup.
    int kg = 0;
    while (note > (_keygroups[kg].high + s)) kg += 3;

    // Since the waveform from a keygroup is used for more than one note, we
    // have to calculate how many semitones this note is apart from the group's
    // root note, so it will be played at the correct pitch. Unless the note is
    // the root, this adds or subtracts one or more semitones.
    l += float(note - _keygroups[kg].root);

    // At this point, `l` is the number of semitones, plus the extra tunings in
    // cents, that we need to tune away from the keygroup's root note. We need
    // to turn this into a step size for reading through the waveform data.
    // If you're playing the root note for the keygroup with no detuning, at a
    // sample rate of 44100 Hz, then we need to step through the waveform 0.5
    // samples at a time, since the waveform was recorded at 32000 Hz.
    // If you're not playing the root note, and/or the note is detuned, the
    // step size will be smaller or larger.
    // To pitch shift, you multiply the reference frequency by 2^(semitones/12)
    // where "semitones" is the amount of pitch shifting. Here, the reference
    // "frequency" is 32000/sampleRate for the root note. To get the step size,
    // multiply by exp(0.05776226 * semitones) which is the same as 2^(semis/12).
    l = 32000.0f * _inverseSampleRate * std::exp(0.05776226505f * l);

    // Instead of storing the step size as a float, this plug-in stores it as a
    // fixed point number, so it's multiplied by 65536 (shifted by 16 bits).
    _voices[vl].delta = int(65536.0f * l);

    // For each sampled waveform, EPiano actually has three different samples
    // for three different velocity levels. That's why in the loop for finding
    // the keygroup above, we incremented kg by 3. Currently, the waveform for
    // the low velocity is selected, so pick one of the other two if needed.
    if (velocity > 48) kg++;  // mid velocity sample
    if (velocity > 80) kg++;  // high velocity sample

    // Copy the other keygroup info into this voice.
    _voices[vl].frac = 0;
    _voices[vl].pos = _keygroups[kg].pos;
    _voices[vl].end = _keygroups[kg].end - 1;
    _voices[vl].loop = _keygroups[kg].loop;
    _voices[vl].note = note;

    // === Calculate panning based on the note number ===

    if (note <  12) note = 12;   // min = C-1
    if (note > 108) note = 108;  // max = C8

    // The idea is that middle C (note 60, C4) is equally loud in both the left
    // and right channels. Notes to the left of middle C are louder in the left
    // channel, and notes to the right are louder in the right channel.
    // The user can set Stereo Width from 0% to 200%. At 0%, both channels are
    // always equally loud. The higher the Stereo Width setting is, the more
    // the notes are panned left and right.
    // Here, we calculate how far the note is from middle C and multiply that
    // by _width, which is proportional to the Stereo Width parameter. For note
    // C4, this gives 0. If the note is on the left of middle C, this gives a
    // negative number. The further away the note is from C4, the larger `w`.
    const float w = _width * float(note - 60);

    // At middle C, the output volume for both channels is simply `_volume`.
    // This note is perfectly centered. For notes to the right of C4, the
    // volume for the right channel is made larger while the volume for the
    // left channel is made smaller by the same amount. And vice versa.
    // Together, outr and outl always sum to 2*_volume.
    _voices[vl].outr = _volume * (1.0f + w);
    _voices[vl].outl = _volume * (1.0f - w);

    // === Envelope ===

    // Calculate the starting envelope value based on the velocity.
    // When the velocity sensitivity is low (closer to 0%), in MDA EPiano this
    // doesn't mean that it takes less velocity to play loud notes -- it only
    // makes the dynamic range smaller, so that there isn't as much difference
    // between low and high velocities -- but it doesn't raise the floor.
    // When the velocity sensitivity is high (closer to 100%) you can play much
    // louder (or softer). Note that the envelope can start out higher than 1.0.
    _voices[vl].env = (3.0f + 2.0f * _velocitySensitivity) * std::pow(0.0078f * velocity, _velocitySensitivity);

    // Make higher notes (anything over middle C) gradually quieter by giving
    // them a lower starting envelope value.
    if (note > 60) _voices[vl].env *= std::exp(0.01f * float(60 - note));

    // Calculate the exponential decay factor. The lower the note, the longer
    // the decay is. Note that a typical decay factor is 0.999967. Even a tiny
    // change, to say 0.999929, makes a big difference in the decay time!
    if (note < 44) note = 44;  // limit max decay length
    _voices[vl].decay = std::exp(-_inverseSampleRate * std::exp(-1.0f + 0.03f*float(note) - 2.0f*_envDecay));
  }

  // Note off
  else {
    for (int v = 0; v < NVOICES; ++v) {
      // Any voices playing this note?
      if (_voices[v].note == note) {
        // If the sustain pedal is not pressed...
        if (_sustain == 0) {
          // ...then calculate a new decay factor that will fade out the voice.
          // The length of the release depends on the parameter and on the note
          // number, so that lower notes have a longer release.
          _voices[v].decay = std::exp(-_inverseSampleRate * std::exp(6.0f + 0.01f*float(note) - 5.0f*_envRelease));
        } else {
          // Sustain pedal is pressed, so put the note in sustain mode.
          _voices[v].note = SUSTAIN;
        }
      }
    }
  }
}

juce::AudioProcessorEditor *MDAEPianoAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDAEPianoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAEPianoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    _parametersChanged.store(true);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAEPianoAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Envelope Decay",
    "Envelope Decay",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Envelope Release",
    "Envelope Release",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Hardness",
    "Hardness",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Treble Boost",
    "Treble Boost",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 1.0f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Modulation",
    "Modulation",
    juce::NormalisableRange<float>(),
    0.5f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      if (value > 0.5f)
        return "Trem " + juce::String(200.0f * value - 100.0f, 1);
      else
        return "Pan " + juce::String(100.0f - 200.0f * value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "LFO Rate",
    "LFO Rate",
    juce::NormalisableRange<float>(),
    0.0f,
    "Hz",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(std::exp(6.22f * value - 2.61f), 2);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity Sensitivity",
    "Velocity Sensitivity",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    33.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Stereo Width",
    "Stereo Width",
    juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
    100.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Polyphony",
    "Polyphony",
    juce::NormalisableRange<float>(1.0f, 32.0f, 1.0f),
    16.0f,
    "voices"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Fine Tuning",
    "Fine Tuning",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "cents"));

  // The random detuning parameter is not linear but uses a bit of a curve.
  // Could do this with skew, but it's easier to keep this parameter in the
  // 0 - 1 range since we will only treat it indirectly as cents anyway.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Random Tuning",
    "Random Tuning",
    juce::NormalisableRange<float>(),
    0.0f,
    "cents",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(50.0f * value * value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Overdrive",
    "Overdrive",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
    0.0f,
    "%"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDAEPianoAudioProcessor();
}
