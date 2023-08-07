# MDA plug-ins in JUCE

This repo contains the [MDA freeware plug-ins](http://mda.smartelectronix.com) implemented in [JUCE](https://juce.com).

The [original plug-in source code](https://sourceforge.net/projects/mda-vst/) is for VST2, which is no longer supported. For my own elucidation and enjoyment, I decided to rewrite these plug-ins in JUCE.

The MDA plug-ins were originally written by Paul Kellett, about two decades ago. I also consulted the AU versions by [Sophia Poirier](http://destroyfx.org).

These source examples are provided mainly for **learning purposes**! As Paul said of his original source code release:

> This code is definitely not an example of how to write plug-ins! It's obvious that I didn't know much C++ when I started, and some of the optimizations might have worked on a 486 processor but are not relevant today.  The code is very raw with no niceties like parameter de-zipping, but maybe you'll find some useful stuff in there.

I've tried to fix some of these issues but did not add any new features such as the parameter dezippering. The code here is 20 years old, so it may no longer be the most optimal way to implement these algorithms. Consider this project to be a kind of plug-in archeology. ;-)

That said, it's still **a good place to get started** if you're learning about audio DSP, which is why I added lots of comments to describe what's going on. If you have a [basic understanding of JUCE](https://www.youtube.com/c/TheAudioProgrammer), you should be able to follow along.

Some notes:

- The code has been cleaned up a bit and documented, and occasionally bug fixed.
- These plug-ins have no UI and use the default generic JUCE UI.
- I'm not using the standard JUCE coding style because it's ugly. ;-)
- The code has only been tested with Xcode on a Mac using JUCE 7, but should work on Windows too.

This source code is licensed under the [MIT license](LICENSE.txt) but keep in mind that JUCE has its own licensing terms. JUCE conversion done by Matthijs Hollemans.

## Where to start

If you're new to audio programming and want to learn how these plug-ins work, start with one of the following easy plug-ins:

- RingMod
- Overdrive
- Degrade
- Delay

If you're interested in synths, check out:

- MDA Piano
- MDA EPiano
- JX10 analog synth
- DX10 FM synth

## Included plug-ins

You can find an archived copy of the [original documentation here](https://web.archive.org/web/20211028012511/http://mda.smartelectronix.com/vst/help/mdaplugs.htm).

> **Note:** The below documentation of the parameters was taken from the original website but is not always complete or correct.

### Ambience

Small space reverberator. Designed to simulate a distant mic in small rooms, without the processor overhead of a full reverb plug-in. Can be used to add "softness" to drums and to simulate someone talking "off mic".

| Parameter | Description |
| --------- | ----------- |
| Size | Room size (variable from "cardboard box" through "vocal booth" to "medium studio") |
| HF Damp | Gentle low-pass filter to emulate the high frequency absorption of softer wall surfaces |
| Mix | Wet / dry mix (affects perceived distance) |
| Output | Level trim |

### Degrade

This plug-in reduces the bit-depth and sample rate of the input audio (using the Quantize and Sample Rate parameters) and has some other features for attaining the sound of vintage digital hardware.  The headroom control is a peak clipper, and the Non-Linearity controls add some harmonic distortion to thicken the sound.  Post Filter is a low-pass filter to remove some of the grit introduced by the other controls.

| Parameter | Description |
| --------- | ----------- |
| Headroom | Peak clipping threshold |
| Quantize | Bit depth (typically 8 or below for "telephone" quality) |
| Sample Rate | Reduced sample rate |
| Sample Mode | Sample or Hold for different sound characters |
| Post Filter | Low-pass filter to muffle the distortion produced by the previous controls |
| Non-Linearity | Additional harmonic distortion "thickening" |
| Non-Linear Mode | Odd or Even for different sound characters |
| Output | Level trim |

### Delay

Simple stereo delay with feedback tone control.

| Parameter | Description |
| --------- | ----------- |
| Left Delay | Left channel delay time |
| Right Delay | Right channel delay as a percentage of left channel delay - variable to left, fixed ratios to right |
| Feedback | Feedback (sum of left and right outputs) |
| FB Tone | Feedback filtering - low-pass to left, high-pass to right |
| FX Mix | Wet / dry mix |
| Output | Level trim |

### DX10

Simple FM synthesizer. Sounds similar to the later Yamaha DX synths including the heavy bass but with a warmer, cleaner tone.

| Parameter | Description |
| --------- | ----------- |
| Attack | Envelope attack time |
| Decay | Envelope decay time |
| Release | Envelope release time |
| Ratio | Modulator frequency (as a multiple of the carrier frequency) |
| Fine Ratio | Fine control of modulator frequency for detuning and inharmonic effects |
| Mod Level 1 | Initial modulator level |
| Mod Decay| Time for modulator level to reach... |
| Mod Level 2 | Final modulator level |
| Mod Release | Time for modulator level to reach zero |
| Vel Sens | Veclocity control of modulator level (brightness) |
| Vibrato | Vibrato amount (note that heavy vibrato may also cause additional tone modulation effects) |
| Octave | Octave shift |

The plug-in is 8-voice polyphonic and is designed for high quality (low aliasing) and low processor usage - this means that some features that would increase processor usage have been left out!

### EPiano

Rhodes Piano. This plug-in is extremely similar to MDA Piano, but uses different samples. It also has some basic LFO modulation.

### JX10

Simple 2-oscillator analog synthesizer. 8 voice polyphonic.

> **TIP!** I cleaned up the code for this synth and [wrote a 350-page book about it](https://leanpub.com/synth-plugin). The book teaches the fundamentals of audio programming by showing you how to build this synth step-by-step.

The plug-in is designed for high quality (lower aliasing than most soft synths) and low processor usage - this means that some features that would increase CPU load have been left out!

Additional patch design by Stefan Andersson and Zeitfraktur, Sascha Kujawa.

Please note that the mda JX10 is not related to the JX series of synths available from JXPlugins.

| Parameter | Description |
| --------- | ----------- |
| OSC Mix | Level of second oscillator (both oscillators are sawtooth wave only - but see Vibrato below) |
| OSC Tune | Tuning of second oscillator in semitones |
| OSC Fine | Tuning of second oscillator in cents |
| Glide Mode | POLY = 8-voice polyphonic, P-LEGATO = 8-voice polyphonic with pitch glide if a key is held, P-GLIDE = 8-voice polyphonic with pitch glide, MONO = monophonic, M-LEGATO = monophonic with pitch glide if a key is held, M-GLIDE = monophonic with pitch glide |
| Glide Rate | Pitch glide rate |
| Glide Bend | Initial pitch-glide offset, for pitch-envelope effects |
| VCF Freq | Filter cutoff frequency |
| VCF Reso | Filter resonance |
| VCF Env | Cutoff modulation by VCF envelope |
| VCF LFO | Cutoff modulation by LFO |
| VCF Vel | Cutoff modulation by velocity (turn fully left to disable velocity control of cutoff and amplitude) |
| VCF A,D,S,R | Attack, Decay, Sustain, Release envelope for filter cutoff |
| ENV A,D,S,R | Attack, Decay, Sustain, Release envelope for amplitude |
| LFO Rate | LFO rate (sine wave only) |
| Vibrato | LFO modulation of pitch - turn to left for PWM effects |
| Noise | White noise mix |
| Octave | Master tuning in octaves |
| Tuning | Master tuning in cents |

When Vibrato is set to PWM, the two oscillators are phase-locked and will produce a square wave if set to the same pitch. Pitch modulation of one oscillator then causes Pulse Width Modulation (pitch modulation of both oscillators for vibrato is still available from the modulation wheel). Unlike other synths, in PWM mode the oscillators can still be detuned to give a wider range of PWM effects.

| MIDI Control |  |
| --------- | ----------- |
| Pitch Bend | +/- 2 semitones |
| CC1 (Mod Wheel) | Vibrato |
| Channel Pressure | Filter cutoff modulation by LFO |
| CC2, CC74 | Increase filter cutoff |
| CC3 | Decrease filter cutoff |
| CC7 | Volume |
| CC16, CC71 | Increase filter resonance |
| Program Change | 1 - 64 |

### Limiter

Opto-electronic style limiter.

| Parameter | Description |
| --------- | ----------- |
| Thresh | Threshold (this is not a brickwall limiter so this is only approximate) |
| Output | Level trim |
| Release |   |
| Attack |   |
| Knee | Select Hard or Soft (both can pump and distort when pushed hard - but that could be just what you want!) |

### Loudness

Equal loudness contours for bass EQ and mix correction.

| Parameter | Description |
| --------- | ----------- |
| Loudness | Source level relative to listening level (based on a 100 dB SPL maximum level) |
| Output | Level trim |
| Link | Automatically adjusts Output to maintain a consistent tonal balance at all levels |

The ear is less sensitive to low frequencies when listening at low volume. This plug-in is based on the Stevens-Davis equal loudness contours and allows the bass level to be adjusted to simulate or correct for this effect.

Example uses:

- If a mix was made with a very low or very high monitoring level, the amount of bass can sound wrong at a normal monitoring level. Use Loudness to adjust the bass content.
- Check how a mix would sound at a much louder level by decreasing Loudness. (although the non-linear behaviour of the ear at very high levels is not simulated by this plug-in).
- Fade out without the sound becoming "tinny" by activating Link and using Loudness to adjust the level without affecting the tonal balance.

### Overdrive

Soft distortion plug-in.

Possible uses include adding body to drum loops, fuzz guitar, and that 'standing outside a nightclub' sound. This plug does not simulate valve distortion, and any attempt to process organ sounds through it will be extremely unrewarding!

| Parameter | Description |
| --------- | ----------- |
| Drive | Amount of distortion |
| Muffle | Gentle low-pass filter |
| Output | Level trim |

### Piano

Acoustic piano instrument — this was quite a [popular free piano synth](https://www.kvraudio.com/product/piano-by-mda) back in the day.

Not designed to be the best sounding piano in the world, but boasts extremely low CPU and memory usage.

| Parameter | Description |
| --------- | ----------- |
| Decay |  Envelope decay time |
| Release | Envelope release time |
| Stereo Width | Key to pan position, with additional stereo ambience at high settings |
| Tuning | Controls for overall tuning, random tuning amount, and high frequency stretch tuning amount. Use Alt-click to reset sliders to their default positions |
| Vel Sens | Velocity curve: Mid point is normal "square law" response |
| Muffle | Gentle low pass filter. Use "V" slider to adjust velocity control |
| Hardness | Adjusts sample keyranges up or down to change the "size" and brightness of the piano. Use "V" slider to adjust velocity control |
| Polyphony | Adjustable from monophonic to 32 voices |

### RingMod

This was the first "mda" effect, made way back in 1998.  It is a simple ring modulator, multiplying the input signal by a sine wave, the frequency of which is set using the Frequency and Fine Tune controls.  As if ring modulation wasn't harsh enough already, the Feedback parameter can add even more edge to the sound!

Can be used as a high-frequency enhancer for drum sounds (when mixed with the original), adding dissonance to pitched material, and severe tremolo (at low frequency settings).

| Parameter | Description |
| --------- | ----------- |
| Freq | Set oscillator frequency in 100Hz steps |
| Fine | Oscillator frequency fine tune |
| Feedback | Amount of feedback for "harsh" sound |

### Shepard

Shepard tone generator

This plug-in generates a continuously rising or falling tone.  Or rather, that's what it sounds like but really new harmonics are always appearing at one end of the spectrum and disappearing at the other. (Using some EQ can improve the psychoacoustic effect, depending on your listening environment.)

These continuous tones are actually called "Risset tones", developed from the earlier "Shepard tones" which change in series of discrete steps. The Mode control allows the input signal to be mixed or ring modulated with the tone - this works well as one element of a complex chain of effects.

| Parameter | Description |
| --------- | ----------- |
| Mode| TONES = tones only, RING MOD = input ring modulated by tones, TONES+IN = tones mixed with input |
| Rate | Speed of rising (right) or falling (left) |
| Output | Level trim |

### Stereo

Stereo Simulator: Add artificial width to a mono signal. Haas delay and comb filtering.

This plug converts a mono signal to a 'wide' signal using two techniques: Comb filtering where the signal is panned left and right alternately with increasing frequency, and Haas panning where a time delayed signal sounds appears to be at the same level as a quieter non-delayed version.

Comb mode is mono compatible. Haas mode is not, but sounds fine on some signals such as backing vocals. This plug-in must be used in a stereo channel or bus!

| Parameter | Description |
| --------- | ----------- |
| Width | Stereo width (turn to right for comb filter mode, left for Haas panning mode) |
| Delay | Delay time - use higher values for more filter "combs" across the frequency spectrum |
| Balance | Balance correction for Haas mode |
| Mod | Amount of delay modulation (defaults to OFF to reduce processor usage) |
| Rate | Modulation rate (note that modulation completely disappears in mono) |

### SubSynth

Sub-Bass Synthesizer: Several low frequency enhancement methods. More bass than you could ever need!

Be aware that you may be adding low frequency content outside the range of your monitor speakers.  To avoid clipping, follow with a limiter plug-in (this can also give some giant hip-hop drum sounds!).

| Parameter | Description |
| --------- | ----------- |
| Type | see below |
| Level | Amount of synthesized low frequencysignal to be added |
| Tune | Maximum frequency - keep as low as possible to reduce distortion. In Key Osc mode sets the oscillator frequency |
| Dry Mix | Reduces the level of the original signal |
| Thresh | Increase to "gate" the low frequency effect and stop unwanted background rumbling |
| Release | Decay time in Key Osc mode |

Type can be:

- **Distort**: Takes the existing low frequencies, clips them to produce harmonics at a constant level, then filters out the higher harmonics. Has a similar effect to compressing the low frequencies.

- **Divide**: As above, but works at an octave below the input frequency, like an octave divider guitar pedal.

- **Invert**: Flips the phase of the low frequency signal once per cycle to add a smooth sub-octave. A simplified version of the classic Sub-Harmonic Synthesizer.

- **Key Osc.**: Adds a decaying "boom" - usually made with an oscillator before a noise gate keyed with the kick drum signal.

### TestTone

Signal generator with pink and white noise, impulses and sweeps.

Note: the AU version of this plug-in has a bunch of extra features and improvements that I didn't convert yet.

| Parameter | Description |
| --------- | ----------- |
| Mode | see below |
| Level | Peak output level |
| Channel | Generate signals on left or right channel only |
| F1 | Base frequency (not applicable to pink and white noise or impulses) |
| F2 | Fine frequency control, or end frequency for sweep modes |
| Sweep | Sweep duration for sweep modes (2 seconds silence is also added between sweeps). Sets repetition rate in inpulse mode |
| Thru | Allow the input signal to pass through the plug-in |
| 0 dB = | Calibrate output so indicated level is relative to, for example -0.01 dB FS or -18 dB FS |

Possible modes:

- MIDI #: sine waves at musical pitches (A3 = 69 = 440 Hz)
- IMPULSE: single-sample impulse
- WHITE: white noise
- PINK: pink noise
- SINE: ISO 1/3-octave frequencies
- LOG SWP: logarithmic frequency sweep
- LOG STEP: 1/3-octave steps
- LIN SWP: linear frequency sweep

## Plug-ins that have not been converted yet

- Bandisto - Multi-band distortion
- BeatBox - Drum replacer
- Combo - Amp & speaker simulator
- De-ess - High frequency dynamics processor
- Detune - Simple up/down pitch shifting thickener
- Dither - Range of dither types including noise shaping
- DubDelay - Delay with feedback saturation and time/pitch modulation
- Dynamics - Compressor / Limiter / Gate
- Envelope - Envelope follower / VCA
- Image - Stereo image adjustment and M-S matrix
- Leslie - Rotary speaker simulator
- Looplex - ?
- Multiband - Multi-band compressor with M-S processing modes
- Re-Psycho! - Drum loop pitch changer
- RezFilter - Resonant filter with LFO and envelope follower
- Round Panner - 3D panner
- SpecMeter
- Splitter - Frequency / level crossover for setting up dynamic processing
- Talkbox - High resolution vocoder
- Thru-Zero Flanger - Classic tape-flanging simulation
- Tracker - Pitch tracking oscillator, or pitch tracking EQ
- Vocoder - Switchable 8 or 16 band vocoder
- VocInput - Pitch tracking oscillator for generating vocoder carrier input

## How the code is structured

Read this section if you're new to JUCE or audio programming! Or if you're wondering why the code is structured the way it is...

Each plug-in is in its own folder. Go into the folder, open the **.jucer** file in Projucer and then launch your IDE from the Projucer project. Build the VST3 target and open it in JUCE's AudioPluginHost for testing.

Since there is no UI for these plug-ins, the only source files are **PluginProcessor.h** and **.cpp**.

### PluginProcessor

The main functions in PluginProcessor are:

- `createParameterLayout()`: this is where the parameters are defined
- `prepareToPlay()`: initializes the plug-in
- `resetState()`: sets things back to zero, clears out any delay lines, and so on. This method is called when the plug-in is initialized in `prepareToPlay` but also when the plug-in is `reset`.
- `update()`: takes the current parameter values and recomputes whatever depends on these parameters. This method is called from `processBlock` but only when parameters have actually changed.
- `processBlock()`: does the actual audio processing

The PluginProcessor has a number of member variables. Their names begin with an underscore, such as `_level`.

There are two types of member variables:

1. "cooked" parameters
2. rendering state

The cooked parameters are filled in by the `update()` method. This method is called by the audio thread at the start of `processBlock()`.  It reads the current parameter values from the APVTS and then puts the cooked version into the member variable. For example:

```c++
void MDARingModAudioProcessor::update()
{
    // Convert from decibels into a linear gain value.
    float level = apvts.getRawParameterValue("Level")->load();
    _level = juce::Decibels::decibelsToGain(level);
}
```

The second type of variable keeps track of rendering state. This is something like the current phase of an oscillator or the delay unit of a filter. These variables are given their initial value by `resetState()` and will be changed by `processBlock`.

Inside `processBlock()` we first read the member variables into local variables (for state) or constants (for parameters). Then the processing loop uses these local variables instead of the member variables. If the audio processing loop updates any of the state (which it usually does), the latest values get copied back into the corresponding member variables after the loop. I'm not sure how useful it is to copy the member variables into local variables, since both will be implemented as a load from a register using an offset, but this is how the original plug-ins did it.

### Parameter calculations

The MDA plug-ins were VST2, meaning that the parameters are always 0 - 1. Since JUCE allows us to have parameters in any range, they were changed to whatever felt more more natural (in dB, Hz, etc).

Let's say the plug-in defines an Output Level parameter. Like all VST2 parameters, this has a range of 0 - 1. But in the UI we want to display this as decibels. In the original code, the `getParameterDisplay()` function converted this 0 - 1 into a decibel value:

```c++
int2strng((VstInt32)(40.0 * fParam - 20.0), text);
```

Here, `fParam` is the 0 - 1 value. The formula used here will show a range from -20 dB (when `fParam = 0`) to +20 dB (when `fParam = 1`). Note that this `getParameterDisplay()` function is only used for drawing the UI, not for anything else.

In the audio processing code, the parameter is still 0 - 1. This value, which represents a range of decibels, is converted into a linear gain so that we can multiply it with the audio.

```c+++
g3 = (float)(pow(10.0, 2.0 * fParam - 1.0));
```

You may be wondering where this formula comes from. To go from decibels to a linear value, we do `pow(10, dB / 20)`. That is similar to what happens here, but not exactly the same. That's because our parameter isn't in decibels, it's 0 - 1. We first need to convert it to decibels:

```c++
  pow(10, dB / 20)
= pow(10, (40 * fParam - 20) / 20)
= pow(10, (40 * fParam / 20) - (20 / 20))
= pow(10, 2 * fParam - 1)
```

And that is indeed the formula the original plug-in used.

Fortunately, we don't need to do any of this. In the JUCE implementation, we can simply define the parameter to go from -20 to +20 dB. And then the `update()` function does:

```c++
float outputLevel = apvts.getRawParameterValue("Output")->load();  // -20 to +20 dB
g3 = juce::Decibels::decibelsToGain(outputLevel);
```

We can let JUCE handle the math, and we get to work with parameters that have a more natural range than 0 - 1.

However, sometimes it's a little more tricky. In the MDA Delay plug-in, the delay time in samples was calculated using the formula:

```c++
ldel = (VstInt32)(size * fParam0 * fParam0);
```

Here, `size` is the maximum length of the delay buffer in samples, and `fParam0` is a value between 0 and 1 as usual. Notice that `fParam0` gets squared, I'll explain why shortly.

The `getParameterDisplay()` for this parameter was defined as:

```c++
int2strng((VstInt32)(ldel * 1000.0f / getSampleRate()), text);
```

Rather than deriving this from `fParam0` directly, it takes the computed delay in samples `ldel`, and converts it to a time in milliseconds that is shown to the user.

In the JUCE version, I replaced this by a parameter that lets you directly choose the delay time in milliseconds, which seemed like a simpler approach. Instead of going from 0 - 1, the parameter goes to 0 to 500 ms. Makes sense, right?

However, recall that the audio processing logic squares the parameter value in the formula `size * fParam0 * fParam0`. Since that parameter goes from 0 - 1, this creates a nice little x^2 curve. This kind of thing is usually done to make it easier to pick smaller delays. For example, at a sample rate of 44100, the maximum delay length is 22050 samples. With the parameter set to 0.5, the delay is not 11025 (= half) but 5512 samples (= half squared). It makes the slider non-linear, which is what you want for things like times and frequencies.

But we are not working with a normalized value from 0 - 1 anymore, our parameter is in milliseconds already. Squaring that number doesn't make any sense.

Remember that squaring was only done to make the slider work more logarithmically, giving more room to smaller delays than to larger delays. In JUCE, we can achieve this by giving the slider a skew factor. Now our slider is non-linear too, just like in the original plug-in. But we don't have to do any of the math for it ourselves, JUCE takes care of this for us.

We can simply calculate the delay time linearly, without squaring the parameter:

```c++
const float samplesPerMsec = float(getSampleRate()) / 1000.0f;
float ldelParam = apvts.getRawParameterValue("L Delay")->load();  // 0 - 500 ms
ldel = int(ldelParam * samplesPerMsec);
```

### Denormals

Many plug-ins have code to flush denormals, for example like the following. This sets the variable to zero if the number becomes too small:

```c++
if (std::abs(f) > 1.0e-10f) {
    _filter = f;
} else {
    _filter = 0.0f;
}
```

I left this code in the plug-in even though it is not strictly necessary, as we use `juce::ScopedNoDenormals` to automatically flush denormals to zero.
