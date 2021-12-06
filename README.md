# Ray casting experiment

I've been meaning to write my own 3D graphics engine from scratch, so here it is. It has texture mapping, collision detection, vertical camera movement, animation, etc.
Some notable omissions for now are floor/ceiling texture mapping and altitude.

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

Input | Action
--- | ---
Keys `WASD` | Move character
_mouse movement_ | Move camera
_mouse left click_ | Shoot weapon
Keys `1234` | Select weapon
Key `E` | Open doors
Key `M` | Toggle map

## Screenshots

![1](https://user-images.githubusercontent.com/5512054/144922209-1f10fec7-5c40-43e1-9257-3516d15d79dc.png)
![2](https://user-images.githubusercontent.com/5512054/144922204-76b1ffdd-c8c6-4a36-bdfc-62f928c40a46.png)
![3](https://user-images.githubusercontent.com/5512054/144922206-9294f145-b9c2-4319-9892-5431221a9cdd.png)
![4](https://user-images.githubusercontent.com/5512054/144922201-b5927183-fd1c-4989-93b1-b3779ee11a0c.png)

Editor:

![editor](https://user-images.githubusercontent.com/5512054/140579835-f617973e-f796-4628-b049-6f834cf46e86.png)

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
