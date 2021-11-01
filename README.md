# Ray casting experiment

Experiment with ray casted 2.5D graphics using direct pixel painting.

Since direct frame buffer access is not possible anymore, I've used CSFML.

CSFML (the C binding of SFML) is not well documented, so reading the source
code is the best alternative: https://26.customprotocol.com/csfml/files.htm

## Installation

### Linux

Install package `libcsfml-dev`.

Compile by running `make`.

## Playing around

Use key `Q` to close the window and `WASD` to move and rotate the camera.

![Screenshot](https://user-images.githubusercontent.com/5512054/139693715-6d420fab-3479-4e8e-871b-1a2b04de9152.png)
