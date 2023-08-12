# Image

Stereo image adjuster and MS matrix

Allows the level and pan of mono and stereo components to be adjusted separately, or components to be separated for individual processing before recombining with a second *Image* plug-in.

| Parameter | Description |
| --------- | ----------- |
| Mode | see below |
| S Width | Stereo width (level of stereo component) |
| S Pan | Stereo image "skew" |
| M Level | Centre "depth" |
| M Pan | Centre pan |
| Output | Level trim (a 6dB correction may be required for some MS encoded material) |

Possible modes:

- **LR->LR**: stereo image adjustment
- **LR->MS**: encode to MS
- **MS->LR**: decode from MS
- **SM->LR**: decode from MS (input channels reversed)
