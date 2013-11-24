NBT-Cpp
=======

The NbtTag class is a class to load and print NBT files.

From Notch's original spec:
> NBT (Named Binary Tag) is a tag based binary format designed to carry large amounts of binary data with smaller amounts of additional data.

Included programs
-----------------

####`main.cpp`

Arguments: `<path/to/file> [uncompressed]`

The `main.cpp` program is a simple program which takes a compressed or uncompressed NBT file as input and prints out the contents in a json-like format.
The flag `uncompressed` can be ignored because uncompressed files can also be read as compressed, that is because the gzip converter does not try to uncompress an uncompressed file.

**Example:**

`main bigtest.nbt`

Prints the content of `bigtest.nbt` , which is too long and can be found here: <http://wiki.vg/NBT#bigtest.nbt>

####`map.cpp`

Arguments: `<worldpath> <mapnr> [zoom=5] [infoSize=0]`

The `map.cpp` program takes a minecraft world path and map id and renders the map item into a `.PNG` file.
Optionally prints `scale`, `dimension`, `xCenter` and `zCenter` of the map.

**Example:**

`map saves/Legio-Umbra/ 4 5 12`

Renders `saves/Legio-Umbra/data/map_4.dat` with `5x5` pixel size and printing various map data in font size `12`.

