/* main.cpp
 *
 * Prints a json-like tree of the provided file.
 * Supports uncompressed and gzip compressed files.
 *
 * Arguments: <path/to/file> [tag path=""]
 *
 * - path/to/file: The file to load the tag from.
 *   Example: "testdata/bigtest.nbt"
 * - tag path: The path to the tag that will be printed.
 *   Example: "nested compound test.ham.name"
 *
 * Example: main bigtest.nbt
 *
 *   Prints the content of `bigtest.nbt`, which is too long and can be found here: <http://wiki.vg/NBT#bigtest.nbt>
 *
 * by Gjum <gjum42@gmail.com>
 */

#include <stdio.h>
#include "nbt/Tag.h"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Usage: %s <file> [tag path=\"\"]", argv[0]);
        return 0;
    }
    char * tagPath = "";
    if (argc > 2) tagPath = argv[2];
    NBT::Tag * rootTag = (new NBT::Tag)->loadFromFile(argv[1])->getSubTag(tagPath);
    if (!rootTag) {
        printf("There is no such tag \"%s\" in file \"%s\".\n", tagPath, argv[1]);
        return 0;
    }
    printf("%s\n", rootTag->toString().c_str());
    // we do not clean up allocated memory in such a short program
    return 0;
}

