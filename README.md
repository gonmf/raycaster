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
Key `E` | Open doors
Key `M` | Open map
Key `P` | Pause game
Key `Esc` | Close game

## Screenshots
![1](https://user-images.githubusercontent.com/5512054/144130039-ef11e0ec-2871-4ec4-831c-2d3f1b0302c8.png)
![2](https://user-images.githubusercontent.com/5512054/140586136-67068fab-ef16-4ff1-8235-09f4d0fd09c3.png)
![3](https://user-images.githubusercontent.com/5512054/140626177-be5135c0-5685-4077-80f8-f749977e1e0e.png)
![4](https://user-images.githubusercontent.com/5512054/140586137-22eeef83-7fc1-4252-9f69-2b135652b6fc.png)

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
