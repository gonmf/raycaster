# Ray casting experiment

Experiment with ray casted 2.5D graphics using direct pixel painting.

Since direct frame buffer access is not possible anymore, I've used CSFML.

CSFML (the C binding of SFML) is not well documented, so reading the source
code is the best alternative: https://26.customprotocol.com/csfml/files.htm

## Installation

### Linux

Install package `libcsfml-dev`.

Compile by running `make`.

## Controls

Use key `Escape` to close the window and `WASD` to move and rotate the camera.
Keys `QE` can be used to strafe left and right.

![Screenshot](https://user-images.githubusercontent.com/5512054/139965678-059558f5-756f-4eab-a4c7-941fe170dbe2.png)
