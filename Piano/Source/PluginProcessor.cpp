#include "PluginProcessor.h"
#include "mdaPianoData.h"

MDAPianoProgram::MDAPianoProgram()
{
  param[0]  = 0.50f;  // Decay
  param[1]  = 0.50f;  // Release
  param[2]  = 0.50f;  // Hardness

  param[3]  = 0.50f;  // Vel>Hard
  param[4]  = 1.00f;  // Muffle
  param[5]  = 0.50f;  // Vel>Muff

  param[6]  = 0.33f;  // Vel Curve
  param[7]  = 0.50f;  // Stereo
  param[8]  = 0.33f;  // Max Poly

  param[9]  = 0.50f;  // Tune
  param[10] = 0.00f;  // Random
  param[11] = 0.50f;  // Stretch

  strcpy(name, "mda Piano");
}

MDAPianoProgram::MDAPianoProgram(const char *name,
                                 float p0, float p1, float p2, float p3,
                                 float p4, float p5, float p6, float p7,
                                 float p8, float p9, float p10, float p11)
{
  strcpy(this->name, name);
  param[0] = p0; param[1] = p1; param[2]  = p2;  param[3]  = p3;
  param[4] = p4; param[5] = p5; param[6]  = p6;  param[7]  = p7;
  param[8] = p8; param[9] = p9; param[10] = p10; param[11] = p11;
}

MDAPianoAudioProcessor::MDAPianoAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  // Just in case...
  _sampleRate = 44100.0f;
  _inverseSampleRate = 1.0f / _sampleRate;
  _delayMax = 0x7F;

  createPrograms();
  setCurrentProgram(0);

  _waves = pianoData;

  // Waveform data and keymapping is hard-wired in *this* version.
  _keygroups[ 0].root = 36; _keygroups[ 0].high = 37;  _keygroups[ 0].pos = 0;      _keygroups[ 0].end = 36275;  _keygroups[ 0].loop = 14774;
  _keygroups[ 1].root = 40; _keygroups[ 1].high = 41;  _keygroups[ 1].pos = 36278;  _keygroups[ 1].end = 83135;  _keygroups[ 1].loop = 16268;
  _keygroups[ 2].root = 43; _keygroups[ 2].high = 45;  _keygroups[ 2].pos = 83137;  _keygroups[ 2].end = 146756; _keygroups[ 2].loop = 33541;
  _keygroups[ 3].root = 48; _keygroups[ 3].high = 49;  _keygroups[ 3].pos = 146758; _keygroups[ 3].end = 204997; _keygroups[ 3].loop = 21156;
  _keygroups[ 4].root = 52; _keygroups[ 4].high = 53;  _keygroups[ 4].pos = 204999; _keygroups[ 4].end = 244908; _keygroups[ 4].loop = 17191;
  _keygroups[ 5].root = 55; _keygroups[ 5].high = 57;  _keygroups[ 5].pos = 244910; _keygroups[ 5].end = 290978; _keygroups[ 5].loop = 23286;
  _keygroups[ 6].root = 60; _keygroups[ 6].high = 61;  _keygroups[ 6].pos = 290980; _keygroups[ 6].end = 342948; _keygroups[ 6].loop = 18002;
  _keygroups[ 7].root = 64; _keygroups[ 7].high = 65;  _keygroups[ 7].pos = 342950; _keygroups[ 7].end = 391750; _keygroups[ 7].loop = 19746;
  _keygroups[ 8].root = 67; _keygroups[ 8].high = 69;  _keygroups[ 8].pos = 391752; _keygroups[ 8].end = 436915; _keygroups[ 8].loop = 22253;
  _keygroups[ 9].root = 72; _keygroups[ 9].high = 73;  _keygroups[ 9].pos = 436917; _keygroups[ 9].end = 468807; _keygroups[ 9].loop = 8852;
  _keygroups[10].root = 76; _keygroups[10].high = 77;  _keygroups[10].pos = 468809; _keygroups[10].end = 492772; _keygroups[10].loop = 9693;
  _keygroups[11].root = 79; _keygroups[11].high = 81;  _keygroups[11].pos = 492774; _keygroups[11].end = 532293; _keygroups[11].loop = 10596;
  _keygroups[12].root = 84; _keygroups[12].high = 85;  _keygroups[12].pos = 532295; _keygroups[12].end = 560192; _keygroups[12].loop = 6011;
  _keygroups[13].root = 88; _keygroups[13].high = 89;  _keygroups[13].pos = 560194; _keygroups[13].end = 574121; _keygroups[13].loop = 3414;
  _keygroups[14].root = 93; _keygroups[14].high = 999; _keygroups[14].pos = 574123; _keygroups[14].end = 586343; _keygroups[14].loop = 2399;

  apvts.state.addListener(this);
}

MDAPianoAudioProcessor::~MDAPianoAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDAPianoAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDAPianoAudioProcessor::getNumPrograms()
{
  return NPROGS;
}

int MDAPianoAudioProcessor::getCurrentProgram()
{
  return _currentProgram;
}

void MDAPianoAudioProcessor::setCurrentProgram(int index)
{
  _currentProgram = index;

  const char *paramNames[] = {
    "Envelope Decay",
    "Envelope Release",
    "Hardness Offset",
    "Velocity to Hardness",
    "Muffling Filter",
    "Velocity to Muffling",
    "Velocity Sensitivity",
    "Stereo Width",
    "Polyphony",
    "Fine Tuning",
    "Random Detuning",
    "Stretch Tuning",
  };

  for (int i = 0; i < NPARAMS; ++i) {
    apvts.getParameter(paramNames[i])->setValueNotifyingHost(_programs[index].param[i]);
  }
}

const juce::String MDAPianoAudioProcessor::getProgramName(int index)
{
  return { _programs[index].name };
}

void MDAPianoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
  // not implemented
}

void MDAPianoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  _sampleRate = sampleRate;
  _inverseSampleRate = 1.0f / _sampleRate;

  // The comb filter that is used for stereo separation uses a delay line.
  // The delay should not be too long. The original plug-in hardcodes the
  // delay length to either 127 samples or 255 samples, depending on the sample
  // rate. This means the delay is not always the same duration in seconds but
  // it's probably good enough... (about 3 ms at 44100 Hz).
  if (_sampleRate > 64000.0f) _delayMax = 0xFF; else _delayMax = 0x7F;

  resetState();
  _parametersChanged.store(true);
}

void MDAPianoAudioProcessor::releaseResources()
{
}

void MDAPianoAudioProcessor::reset()
{
  resetState();
}

bool MDAPianoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDAPianoAudioProcessor::createPrograms()
{
  // Note that the presets store the parameters as values between 0 and 1.
  // This is because the original plugin was for VST2, which always uses
  // parameters in that range. Our parameters have different ranges, for
  // example from -50 to +50 cents, but the underlying normalized values
  // should still match those from the original version and therefore the
  // presets still work!

  _programs.emplace_back("mda Piano",        0.500f, 0.500f, 0.500f, 0.5f, 0.803f, 0.251f, 0.376f, 0.500f, 0.330f, 0.500f, 0.246f, 0.500f);
  _programs.emplace_back("Plain Piano",      0.500f, 0.500f, 0.500f, 0.5f, 0.751f, 0.000f, 0.452f, 0.000f, 0.000f, 0.500f, 0.000f, 0.500f);
  _programs.emplace_back("Compressed Piano", 0.902f, 0.399f, 0.623f, 0.5f, 1.000f, 0.331f, 0.299f, 0.499f, 0.330f, 0.500f, 0.000f, 0.500f);
  _programs.emplace_back("Dance Piano",      0.399f, 0.251f, 1.000f, 0.5f, 0.672f, 0.124f, 0.127f, 0.249f, 0.330f, 0.500f, 0.283f, 0.667f);
  _programs.emplace_back("Concert Piano",    0.648f, 0.500f, 0.500f, 0.5f, 0.298f, 0.602f, 0.550f, 0.850f, 0.356f, 0.500f, 0.339f, 0.660f);
  _programs.emplace_back("Dark Piano",       0.500f, 0.602f, 0.000f, 0.5f, 0.304f, 0.200f, 0.336f, 0.651f, 0.330f, 0.500f, 0.317f, 0.500f);
  _programs.emplace_back("School Piano",     0.450f, 0.598f, 0.626f, 0.5f, 0.603f, 0.500f, 0.174f, 0.331f, 0.330f, 0.500f, 0.421f, 0.801f);
  _programs.emplace_back("Broken Piano",     0.050f, 0.957f, 0.500f, 0.5f, 0.299f, 1.000f, 0.000f, 0.500f, 0.330f, 0.450f, 0.718f, 0.000f);
}

void MDAPianoAudioProcessor::resetState()
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
  _muff = 160.0f;
  _sustain = 0;

  // Empty the delay for the comb filter.
  _delayPos = 0;
  memset(_combDelay, 0, sizeof(float) * 256);
}

void MDAPianoAudioProcessor::update()
{
  // Envelope Decay: The UI shows this as 0% - 100%. We turn it into two linear
  // curves, one from 0.0 to 0.5, and one from 0.5 to 1.0. Both have a slightly
  // different slope. The line goes from 0.25 (at 0%), to 1.0 (at 50%), to 2.0
  // (at 100%). The smaller the percentage, the quicker the note decays.
  float param0 = apvts.getRawParameterValue("Envelope Decay")->load() / 100.0f;
  _envDecay = 2.0f * param0;
  if (_envDecay < 1.0f) _envDecay += 0.25f - 0.5f * param0;

  // Envelope Release. The UI shows this as a percentage, so divide by 100.
  float param1 = apvts.getRawParameterValue("Envelope Release")->load();
  _envRelease = param1 / 100.0f;

  // Hardness Offset: The UI shows -50% to +50%. Here we turn this into an
  // integer between -6 and +6. This is the number of semitones that the MIDI
  // note number gets shifted by, which changes the waveform we select for it.
  float param2 = apvts.getRawParameterValue("Hardness Offset")->load();
  param2 = (param2 + 50.0f) / 100.0f;  // first to 0 - 1
  _hardnessOffset = int(12.0f * param2 - 6.0f);

  // Velocity to Hardness: The UI shows 0% to 100%. Convert to 0.0 - 0.12.
  // Used in addition to the Hardness Offset for changing the keygroup.
  float param3 = apvts.getRawParameterValue("Velocity to Hardness")->load();
  _hardnessVelocity = 0.12f * param3 / 100.0f;

  // Muffling Filter: The UI shows this as a percentage, so divide by 100.
  float param4 = apvts.getRawParameterValue("Muffling Filter")->load();
  _mufflingFilter = param4 / 100.0f;

  // Velocity to Muffling: The UI shows 0% to 100%. Convert to 0.0 - 5.0 but
  // in a curved fashion, since we raise the parameter to the 2nd power.
  float param5 = apvts.getRawParameterValue("Velocity to Muffling")->load() / 100.0f;
  _mufflingVelocity = param5 * param5 * 5.0f;

  // Velocity Sensitivity: The UI shows 0% to 100%. Convert this into a value
  // between 0.25 and 3.0. There are actually two curves: 0% to 25% is a steep
  // line from 0.25 to 1.5; 25% to 100% is a slightly less steep line that goes
  // from 1.5 to 3.0.
  float param6 = apvts.getRawParameterValue("Velocity Sensitivity")->load() / 100.0f;
  _velocitySensitivity = 1.0f + param6 + param6;
  if (param6 < 0.25f) _velocitySensitivity -= 0.75f - 3.0f * param6;

  // Stereo Width: The UI shows 0% to 200%.
  float param7 = apvts.getRawParameterValue("Stereo Width")->load() / 200.0f;
  _comb = param7 * param7;  // 0 - 1 but skewed

  // This is used to keep the overall volume constant as the Stereo Width value
  // changes. It reduces the volume to compensate for _width becoming larger.
  _trim = 1.50f - 0.79f * _comb;

  // This is proportional to Stereo Width, but smaller. It goes from 0 to 0.03.
  // For some reason this only increases until Stereo Width is 150% and then it
  // is flat between 150% and 200%.
  _width = 0.04f * param7;
  if (_width > 0.03f) _width = 0.03f;

  // Polyphony is an integer number between 8 and 32.
  _polyphony = int(apvts.getRawParameterValue("Polyphony")->load());

  // Fine Tuning: The UI shows -50 to +50 cents. Convert this into -0.5 to
  // +0.5, which turns it from cents into semitones.
  _fine = apvts.getRawParameterValue("Fine Tuning")->load() / 100.0f;

  // Random Detuning: The UI shows 0 to 50 cents. Convert this into 0 - 0.077.
  // This will be turned into (a fraction of) semitones later.
  float param10 = apvts.getRawParameterValue("Random Detuning")->load();
  _random = 0.077f * param10 * param10;

  // The UI shows -50 to +50 cents. Convert this into -0.000217 to +0.000217.
  // This will be turned into (a fraction of) semitones later.
  float param11 = apvts.getRawParameterValue("Stretch Tuning")->load();
  param11 = (param11 + 50.0f) / 100.0f;  // first to 0 - 1
  _stretch = 0.000434f * (param11 - 0.5f);
}

void MDAPianoAudioProcessor::processEvents(juce::MidiBuffer &midiMessages)
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
          case 0x01:  // mod wheel
          case 0x43:  // soft pedal
            // This maps the position of the mod wheel to a parabolic curve
            // starting at 161.29 (position 0) down to 0.0 (position 127).
            _muff = 0.01f * float((127 - data2) * (127 - data2));
            break;

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
              _muff = 160.0f;
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

void MDAPianoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

        // Apply the muffle filter. This is a gentle first-order low-pass filter
        // with the difference equation:
        //    y(n) = f * x(n) + f * x(n - 1) + (1 - f)*y(n - 1)
        // I guess technically this is a shelving filter since it has one pole
        // and one zero, but the way the coefficients have been chosen it acts
        // as a low-pass filter. If I did the math right, this filter has a
        // zero at z = -1, meaning Nyquist. It has a pole at z = 1 - f.
        // If f is 1, which happens when the filter is fully turned off, the
        // pole disappears. As f approaches 0, the pole shifts closer towards
        // z = 1, which makes the filter cutoff lower. The zero doesn't move.
        // Another way to look at this: as f becomes smaller, the current and
        // previous samples x(n) and x(n - 1) count less while the feedback
        // from the previous output counts more. This is what removes the high
        // frequencies.
        // Note: because x(n) and x(n-1) are both multiplied by f, this filter
        // has a 6 dB overall gain. You can remove this by multiplying them by
        // f / 2 instead.
        V->f0 += V->ff * (x + V->f1 - V->f0);
        V->f1 = x;

        // Apply panning. The amount of panning was computed in noteOn().
        l += V->outl * V->f0;
        r += V->outr * V->f0;

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

      // When you sum a signal with a delayed version, you get a comb filter.
      // This filter boosts frequencies that are a multiple of the delay length
      // and suppresses other frequencies. To get a wider stereo field, we can
      // add this filtered signal to one channel and subtract it from the other.
      // The length of the delay is fixed (127 or 255 samples).
      _combDelay[_delayPos] = l + r;            // add to delay line, as mono
      ++_delayPos &= _delayMax;                 // increment position & wrap around
      float x = _comb * _combDelay[_delayPos];  // read from delay line

      // Write the result into the output buffer.
      *out0++ = l + x;
      *out1++ = r - x;
    }

    // It's time to handle the event. This starts the new note, or stops the
    // note if velocity is 0.
    if (frame < sampleFrames) {
      int note = _notes[event++];
      int vel  = _notes[event++];
      noteOn(note, vel);
    }
  }

  // Turn off voices whose envelope has dropped below the minimum level.
  for (int v = 0; v < _numActiveVoices; ++v) {
    if (_voices[v].env < SILENCE) {
      _voices[v] = _voices[--_numActiveVoices];
    }
  }

  // Mark the events buffer as done.
  _notes[0] = EVENTS_DONE;
}

void MDAPianoAudioProcessor::noteOn(int note, int velocity)
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

    // For notes higher than middle C, add stretch tuning. This formula gently
    // curves upward so that higher notes are pitched slightly up from normal,
    // or downward so that higher notes are pitched down. The further the note
    // is away from C4, the more it's detuned up or down. Since _stretch is a
    // very small number, the highest notes will at most be pitched up/down by
    // ±50 cents (although very high notes outside of the normal piano range
    // will actually be detuned more).
    if (note > 60) l += _stretch * float(k);

    // Next, we need to determine which waveform to use for this note. That is
    // described by the keygroups. The Hardness Offset parameter can be used to
    // tweak this, so that notes sound duller or brighter.
    int s = _hardnessOffset;

    // High velocity can increment this value even more, so that loud notes
    // sound brighter (which is what happens on a real piano).
    if (velocity > 40) s += int(_hardnessVelocity * float(velocity - 40));

    // What keygroup is this note in? We effectively subtract the "hardness" s
    // from the note number, so if `s` is positive (more hardness), we pretend
    // we're playing a lower note. Pitched up to the actual note we're playing
    // makes this sound "harder" / brighter than the waveform from this note's
    // actual keygroup.
    int kg = 0;
    while (note > (_keygroups[kg].high + s)) kg++;

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
    // samples at a time, since the waveform was recorded at 22050 Hz.
    // If you're not playing the root note, and/or the note is detuned, the
    // step size will be smaller or larger.
    // To pitch shift, you multiply the reference frequency by 2^(semitones/12)
    // where "semitones" is the amount of pitch shifting. Here, the reference
    // "frequency" is 22050 / sampleRate for the root note, i.e. 0.5 at 44100
    // Hz. To get the step size, multiply by exp(0.05776226 * semitones) which
    // is the same as 2^(semitones/12).
    l = 22050.0f * _inverseSampleRate * std::exp(0.05776226505f * l);

    // Instead of storing the step size as a float, this plug-in stores it as a
    // fixed point number, so it's multiplied by 65536 (shifted by 16 bits).
    _voices[vl].delta = int(65536.0f * l);

    // Copy the other keygroup info into this voice.
    _voices[vl].frac = 0;
    _voices[vl].pos = _keygroups[kg].pos;
    _voices[vl].end = _keygroups[kg].end;
    _voices[vl].loop = _keygroups[kg].loop;
    _voices[vl].note = note;

    // === Muffle filter ===

    // Calculate the coefficient for the low-pass filter. This is a combination
    // of the Muffling Filter parameter (0 means 100% and 1 means 0%), times
    // the mod wheel position, plus an additional amount based on the velocity
    // times its sensitivity (note: for low velocities this amount is actually
    // negative, which lowers the filter cutoff).
    // The smaller `f` is, the more we filter (the lower the cutoff frequency).
    // This happens when the Muffling Filter parameter is lower (i.e. closer to
    // 100%), the more the mod wheel is used (this makes _muff smaller), and/or
    // the lower the velocity is.
    float f = 50.0f + _mufflingFilter * _mufflingFilter * _muff + _mufflingVelocity * float(velocity - 64);

    // Set lower and upper bounds on the filter cutoff. The lower bound depends
    // on the note number, so that higher notes have a higher minimum cutoff.
    if (f < 55.0f + 0.25f * float(note)) f = 55.0f + 0.25f * float(note);
    if (f > 210.0f) f = 210.0f;  // square root of 44100

    // Set the filter coefficient and clear out the delays.
    _voices[vl].ff = f * f * _inverseSampleRate;
    _voices[vl].f0 = 0.0f;
    _voices[vl].f1 = 0.0f;

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

    // Because `w` also gets larger when the Stereo Width parameter is larger,
    // we need to compensate so that a wider stereo field doesn't sound louder.
    // That's what the _trim value is for. This is a parabolic curve based on
    // the Stereo Width setting. The wider the stereo width, the smaller _trim
    // is. This gets multiplied by the overall volume setting to give `p`.
    const float p = _volume * _trim;

    // At middle C, the output volume for both channels is simply `p`. This note
    // is perfectly centered. For notes to the right of C4, the volume for the
    // right channel is made larger while the volume for the left channel is
    // made smaller by the same amount. And vice versa. Together, outr and outl
    // always sum to 2*p.
    _voices[vl].outr = p * (1.0f + w);
    _voices[vl].outl = p * (1.0f - w);

    // Note: I'm not sure why their particular curves were chosen for _width
    // and _trim (see aslso the calculations in update()). If the Stereo Width
    // setting is 200% and you play very high or very low notes, the values of
    // outr and outl can actually become negative, which means the phase gets
    // inverted for that channel. That seems odd...

    // === Envelope ===

    // Calculate the starting envelope value based on the velocity.
    // When the velocity sensitivity is low (closer to 0%), in MDA Piano this
    // doesn't mean that it takes less velocity to play loud notes -- it only
    // makes the dynamic range smaller, so that there isn't as much difference
    // between low and high velocities, but doesn't it raise the floor.
    // When the velocity sensitivity is high (closer to 100%) you can play much
    // louder (or softer). Note that the envelope can start out higher than 1.0.
    _voices[vl].env = (0.5f + _velocitySensitivity) * std::pow(0.0078f * velocity, _velocitySensitivity);

    // Calculate the exponential decay factor. The lower the note, the longer
    // the decay is. Note that a typical decay factor is 0.999967. Even a tiny
    // change, to say 0.999929, makes a big difference in the decay time!
    if (note < 44) note = 44;  // limit max decay length
    _voices[vl].decay = std::exp(-_inverseSampleRate * std::exp(-0.6f + 0.033f*float(note) - _envDecay));
  }

  // Note off
  else {
    for (int v = 0; v < NVOICES; ++v) {
      // Any voices playing this note?
      if (_voices[v].note == note) {
        // If the sustain pedal is not pressed...
        if (_sustain == 0) {
          // ...then calculate a new decay factor that will fade out the voice.
          // Note that we don't release on the highest notes. The length of the
          // release depends on the parameter and on the note number.
          if (note < 94 || note == SUSTAIN) {
            _voices[v].decay = std::exp(-_inverseSampleRate * std::exp(2.0f + 0.017f*float(note) - 2.0f*_envRelease));
          }
        } else {
          // Sustain pedal is pressed, so put the note in sustain mode.
          _voices[v].note = SUSTAIN;
        }
      }
    }
  }
}

juce::AudioProcessorEditor *MDAPianoAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDAPianoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAPianoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    _parametersChanged.store(true);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDAPianoAudioProcessor::createParameterLayout()
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
    "Hardness Offset",
    "Hardness Offset",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity to Hardness",
    "Velocity to Hardness",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  // The "Muffling Filter" goes from 100 to 0 but JUCE doesn't let us specify
  // the range starting at 100 and ending at 0, so we reverse it for display.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Muffling Filter",
    "Muffling Filter",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(100.0f - value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity to Muffling",
    "Velocity to Muffling",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

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
    juce::NormalisableRange<float>(8.0f, 32.0f, 1.0f),
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
    "Random Detuning",
    "Random Detuning",
    juce::NormalisableRange<float>(),
    0.0f,
    "cents",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(50.0f * value * value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Stretch Tuning",
    "Stretch Tuning",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "cents"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDAPianoAudioProcessor();
}
