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

### MacOS

First install `homebrew`, and then package `csfml` and the C compiler of your choice.

Compile by running `make`.

## Controls

Use `WASD` to move and your mouse to look around. Pause the game with `P` and quit with `Escape`.

## Screenshots

![Screenshot2](https://user-images.githubusercontent.com/5512054/140052580-0562dcc8-d3f2-494c-9cfd-3b9faf2b554c.png)
![Screenshot1](https://user-images.githubusercontent.com/5512054/139965678-059558f5-756f-4eab-a4c7-941fe170dbe2.png)

---

Copyright (c) 2021 Gonçalo Mendes Ferreira

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
