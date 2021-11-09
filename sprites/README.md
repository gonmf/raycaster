## Sprites

The images must be valid Windows Bitmaps with 24bpp, with the sprite size as
configured in inc/global.h.

Each image must be accompanied by a text file with the exact following format:

```
number of columns
number of rows
1 or 0 indicating if a color should be made transparent
color to be made transparent or ignored
```

A level file must then specify what sprite packss to associate with what functions in the game, and
they will be automatically loaded and unloaded.


### Credits

Bellow is where the original files were downloaded from:

Wall Textures, Author Ultimecia
https://www.textures-resource.com/pc_computer/wolf3d/texture/1375/

Objects, Author Lotos
https://www.spriters-resource.com/pc_computer/wolfenstein3d/sheet/29106/

Normal Soldier, Author Lotos
https://www.spriters-resource.com/pc_computer/wolfenstein3d/sheet/27846/

Machinegun Soldier, Author Hades666
https://www.spriters-resource.com/pc_computer/wolfenstein3d/sheet/65590/
