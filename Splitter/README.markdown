# Splitter

2-way signal splitter. Frequency / level crossover for setting up dynamic processing.

| Parameter | Description |
| --------- | ----------- |
| Mode | see below |
| Frequency | Crossover frequency |
| Frequency SW | Select crossover output: **LOW** frequencies / **ALL** frequencies / **HIGH** frequencies |
| Level | Gate threshold |
| Level SW | Select gate output: **LOW** levels / **ALL** levels / **HIGH** levels |
| Envelope | Gate envelope speed |
| Output | Level trim |

This plug-in can split a signal based on frequency or level, for example for producing dynamic effects where only loud drum hits are sent to a reverb. Other functions include a simple "spectral gate" in **INVERSE** mode and a conventional gate and filter for separating drum sounds in **NORMAL** mode.

Modes:

- NORMAL: Output as selected with **Frequency** and **Level** controls
- INVERSE: Inverse of shown selection (e.g. everything except low frequencies at high level)
- NORM INV: Left / Right split of above
- INV NORM: Right / Left split of above
