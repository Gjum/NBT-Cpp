/* NbtTag.h
 *
 * A class for loading and accessing NBT data
 *
 * by Gjum <gjum42@gmail.com>
 */
#ifndef NBTTAG_H
#define NBTTAG_H

#include <stdint.h>
#include <fstream>
#include <vector>
#include <zfstream.h>

enum NbtTagType {
    tagTypeInvalid = -1,
    tagTypeEnd = 0,
    tagTypeByte = 1,
    tagTypeShort = 2,
    tagTypeInt = 3,
    tagTypeLong = 4,
    tagTypeFloat = 5,
    tagTypeDouble = 6,
    tagTypeByteArray = 7,
    tagTypeString = 8,
    tagTypeList = 9,
    tagTypeCompound = 10,
    tagTypeIntArray = 11
};

class NbtTag;
union NbtPayload {
    int8_t  tagByte;
    int16_t tagShort;
    int32_t tagInt;
    int64_t tagLong;
    float   tagFloat;
    double  tagDouble;
    char*   tagString;
    struct {
        NbtTagType type;
        int32_t size;
        NbtPayload **values;
    } tagList;
    std::vector<NbtTag*> *tagCompound;
};

class NbtTag {
    public:
        NbtTag();
        ~NbtTag();
        void loadFromFile(char *path, bool gzipped);
        NbtTagType readTag(std::istream *file);
        void printTag(unsigned int depth);
    private:
        NbtTagType type;
        char *name;
        NbtPayload *payload;
        void printPayload(NbtTagType type, NbtPayload *payload, unsigned int depth);
        NbtPayload *readPayload(NbtTagType type, std::istream *file);
        void safeRemovePayload();
};

#endif

