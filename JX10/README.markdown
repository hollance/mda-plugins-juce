# JX10

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