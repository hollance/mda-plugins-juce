# MDA plug-ins in JUCE

This repo contains the [MDA freeware plug-ins](http://mda.smartelectronix.com) implemented in [JUCE](https://juce.com). 

The [original plug-in source code](https://sourceforge.net/projects/mda-vst/) is for VST2, which is no longer supported. For my own elucidation and enjoyment, I decided to rewrite them in JUCE.

Some notes:

- The code has been cleaned up a bit and documented, and occasionally bug fixed.
- These plug-ins have no UI and use the default generic JUCE UI.
- The MDA plug-ins were VST2, meaning that the parameters are always 0 - 1. Since JUCE allows us to have parameters in any range, they were changed to whatever felt more more natural (in dB, Hz, etc).
- I'm not using the JUCE coding style because it's ugly.

The plug-ins were originally written by Paul Kellett, about two decades ago. I also consulted the AU versions by [Sophia Poirier](http://destroyfx.org).

These source examples are provided mainly for **learning purposes**! As Paul said of his original source code release:

> This code is definitely not an example of how to write plug-ins! It's obvious that I didn't know much C++ when I started, and some of the optimizations might have worked on a 486 processor but are not relevant today.  The code is very raw with no niceties like parameter de-zipping, but maybe you'll find some useful stuff in there.  

I've tried to fix some of these issues but did not add any new features such as the parameter dezipping. The code here is 20 years old, so it may no longer be the most optimal way to implement these algorithms.

This source code is licensed under the [MIT license](LICENSE.txt) but keep in mind that JUCE has its own licensing terms. JUCE conversion done by Matthijs Hollemans.

## Included plug-ins

You can find the [original documentation here](http://mda.smartelectronix.com/vst/help/mdaplugs.htm).

### mda Degrade

This plug-in reduces the bit-depth and sample rate of the input audio (using the Quantize and Sample Rate parameters) and has some other features for attaining the sound of vintage digital hardware.  The headroom control is a peak clipper, and the Non-Linearity controls add some harmonic distortion to thicken the sound.  Post Filter is a low-pass filter to remove some of the grit introduced by the other controls.

### mda RingMod

This was the first "mda" effect, made way back in 1998.  It is a simple ring modulator, multiplying the input signal by a sine wave, the frequency of which is set using the Frequency and Fine Tune controls.  As if ring modulation wasn't harsh enough already, the Feedback parameter can add even more edge to the sound!

## Plug-ins that have not been converted yet

- Bandisto - Multi-band distortion
- BeatBox - Drum replacer 
- Combo - Amp & speaker simulator
- De-ess - High frequency dynamics processor
- Delay - Simple stereo delay with feedback tone control
- Detune - Simple up/down pitch shifting thickener
- Dither - Range of dither types including noise shaping 
- DubDelay - Delay with feedback saturation and time/pitch modulation
- Dynamics - Compressor / Limiter / Gate
- Envelope - Envelope follower / VCA
- Image - Stereo image adjustment and M-S matrix
- Leslie - Rotary speaker simulator
- Limiter - Opto-electronic style limiter
- Loudness - Equal loudness contours for bass EQ and mix correction 
- Multiband - Multi-band compressor with M-S processing modes 
- Overdrive - Soft distortion
- Re-Psycho! - Drum loop pitch changer
- RezFilter - Resonant filter with LFO and envelope follower
- Round Panner - 3D panner
- Shepard - Continuously rising/falling tone generator
- Splitter - Frequency / level crossover for setting up dynamic processing
- Stereo Simulator - Haas delay and comb filtering
- Sub-Bass Synthesizer - Several low frequency enhancement methods
- Talkbox - High resolution vocoder
- TestTone - Signal generator with pink and white noise, impulses and sweeps
- Thru-Zero Flanger - Classic tape-flanging simulation
- Tracker - Pitch tracking oscillator, or pitch tracking EQ
- Vocoder - Switchable 8 or 16 band vocoder
- VocInput - Pitch tracking oscillator for generating vocoder carrier input
