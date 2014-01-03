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


####`worldmap.cpp`

Takes a minecraft world path and renders the map into `worldmap.png`.
Rendered are `width`x`height` blocks around the `center`, each block `zoom`x`zoom` pixel large.
Optionally prints `center x`, `center z`, `width`, and `height` of the map.
The current color data is from the ingame map renderer, slightly adjusted by me.

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

