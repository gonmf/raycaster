# Ray casting experiment

I've been meaning to write my own 3D graphics engine, so here it is. It has texture mapping, collision detection, vertical camera movement, etc.
Some notable omissions for now are floor/ceiling texture mapping and floor height/altitude.
As a demo I've rendered something akin to the first Wolfenstein game.

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

Use `WASD` to move and your mouse to look around. Pause the game with `P` and quit with `Escape`. Use `E` to open doors.

## Screenshots

![1](https://user-images.githubusercontent.com/5512054/140398798-80b76118-27dc-4fe0-b92a-db0558d9a11e.png)
![2](https://user-images.githubusercontent.com/5512054/140398800-bcb72269-6cb0-4f5e-a6e8-c2a75149466f.png)
![3](https://user-images.githubusercontent.com/5512054/140398804-f3bec682-3478-4ab2-a00f-ad4245de6e94.png)


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
