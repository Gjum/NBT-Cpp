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

To do list
----------

- read region chunk
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

