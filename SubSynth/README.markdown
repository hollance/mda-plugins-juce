# SubSynth

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
