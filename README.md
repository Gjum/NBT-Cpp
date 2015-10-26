NBT-Cpp
=======

The NBT::Tag class is a class to load, create, modify, save, and print NBT files.

From Notch's original spec:
> NBT (Named Binary Tag) is a tag based binary format designed to carry large amounts of binary data with smaller amounts of additional data.

Features
--------

- read raw filestream
- read gzip filestream
- read byte array
- get tag name
- get tag type
- get tag value
- get compound subtag
- get list item
- get tag content as string
- print tag tree as json
- read region chunk

To do list
----------

- read region timestamps
- write raw filestream
- write gzip filestream
- write region chunk

Included programs
-----------------

![worldmap example](http://lunarco.de/minecraft/nbt-worldmap_Songburrow_+450-2050_500x700_2x2.png)

####`worldmap.cpp`

Takes a minecraft world path and renders the map into `worldmap.png`.
Rendered are `width` by `height` blocks around the `center`, each block `zoom` by `zoom` pixels large.
Optionally prints `center x`, `center z`, `width`, and `height` of the map.
The current color data is from the default texture pack, slightly adjusted by me.
The renderer even calculates block transparency and does a bit of height mapping.

**Arguments:**

`<worldpath> [center x=0] [center z=0] [width=256] [height=256] [zoom=1] [info text size=10]`

- `worldpath`: The path to the Minecraft world.
    - Example: `saves/Legio-Umbra/`
- `center x`: The x coordinate of the block at the center of the image.
    - Example: `500`
- `center z`: The z coordinate of the block at the center of the image.
    - Example: `-432`
- `width`: The x range of the blocks in the image.
    - Example: `600`
- `height`: The z range of the blocks in the image.
    - Example: `400`
- `zoom`: The size of each map pixel.
    - Example: `5`
- `info text size`: The font size to use for the info text. `0` for no text.
    - Example: `12`

**Example:**

`worldmap saves/Legio-Umbra/ 500 -432 600 400 5 12`

Renders `saves/Legio-Umbra/` with `5x5` block size and prints various data in font size `12`.
The image contains all blocks from 200,-632 to 799,-231.


####`main.cpp`

Prints a json-like tree of the provided file.
Supports uncompressed and gzip compressed files.

**Arguments:**

`<path/to/file> [tag path=""]`

- `path/to/file`: The file to load the tag from.
    - Example: `testdata/bigtest.nbt`
- `tag path`: The path to the tag that will be printed.
    - Example: `nested compound test.ham.name`

**Example:**

`main bigtest.nbt`

Prints the content of `bigtest.nbt`, which is too long and can be found here: <http://wiki.vg/NBT#bigtest.nbt>


####`map.cpp`

Takes a minecraft world path and map id and renders the map item into `map_#.png`.
Optionally prints `scale`, `dimension`, `xCenter` and `zCenter` of the map.

**Arguments:**

`<worldpath> <mapnr> [zoom=5] [info text size=0]`

- `worldpath`: The path to the Minecraft world.
    - Example: `saves/Legio-Umbra/`
- `mapnr`: The id of the map item.
    - Example: `4`
- `zoom`: The size of each map pixel.
    - Example: `5`
- `info text size`: The font size to use for the info text. `0` for no text.
    - Example: `12`

**Example:**

`map saves/Legio-Umbra/ 4 5 12`

Renders `saves/Legio-Umbra/data/map_4.dat` with `5x5` pixel size and prints various map data in font size `12`.

---

Copyright (c) 2015, Gjum <code.gjum@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
