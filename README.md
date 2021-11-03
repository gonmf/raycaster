# Ray casting experiment

I've been meaning to write my own 3D graphics engine, so here it is.
It currently uses raycasting for a 2.5D effect. As a demo I've rendered
something akin to the first Wolfenstein game.

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
