/* main.cpp
 *
 * Prints a tree of the provided file.
 *
 * by Gjum <gjum42@gmail.com>
 */

#include "NbtTag.h"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Usage: %s <file> [-u (uncompressed)]", argv[0]);
        return 0;
    }
    bool compressed = true;
    if (argc > 2) compressed = false;
    NbtTag rootTag;
    rootTag.loadFromFile(argv[1], compressed);
    rootTag.printTag(0);
    return 0;
}

