/* Tag.h
 *
 * A class for loading and accessing NBT data
 *
 * TODO better error handling
 *      (error class, getting and resetting recent errors)
 *      make it clearer and understandable when to delete tags and when not
 *
 * by Gjum <gjum42@gmail.com>
 */
#ifndef NBT_TAG_H
#define NBT_TAG_H

#include <vector>
#include <fstream>
#include <stdint.h>
#include <string.h>
#include <zlib.h>

namespace NBT {

    enum TagType {
        tagTypeInvalid   = -1,
        tagTypeEnd       =  0,
        tagTypeByte      =  1,
        tagTypeShort     =  2,
        tagTypeInt       =  3,
        tagTypeLong      =  4,
        tagTypeFloat     =  5,
        tagTypeDouble    =  6,
        tagTypeByteArray =  7,
        tagTypeString    =  8,
        tagTypeList      =  9,
        tagTypeCompound  = 10,
        tagTypeIntArray  = 11
    };

    class Tag; // forward declaration for use in tagCompound vector
    union Payload {
        int64_t     tagInt;
        double      tagFloat;
        std::string * tagString;
        struct {
            TagType type;
            std::vector<Payload *> * values;
        } tagList;
        std::vector<Tag *> * tagCompound;
    };

    class Bytestream {
        public:
            char * data;
            unsigned long int cursor, length;

            Bytestream() {
                loadFromByteArray(NULL, 0);
            }
            Bytestream(char * array, unsigned long int length) {
                loadFromByteArray(array, length);
            }
            ~Bytestream() {
                if (data != NULL) delete[] data;
            }
            Bytestream * loadFromByteArray(char * array, unsigned long int _length) {
                data = array;
                cursor = 0;
                length = _length;
                return this;
            }
            char get() {
                return data[cursor++];
            }
            void * getInverseEndian(void * addr, unsigned int length) {
                for (int i = length-1; i >= 0; i--) {
                    ((char*)addr)[i] = get();
                }
                return addr;
            }
            static unsigned char * swapBytes(unsigned char * addr, unsigned int length) {
                for (unsigned int i = 0; i < length/2; i++) {
                    unsigned int j = length-i-1;
                    unsigned char swap = addr[i];
                    addr[i]  = addr[j];
                    addr[j] = swap;
                }
                return addr;
            }
    };

    class Tag {
        public:
            Tag();
            Tag(std::string name_, TagType type_, int64_t val);
            Tag(std::string name_, TagType type_, double val);
            Tag(std::string name_, TagType type_, std::string val);
            Tag(std::string name_, TagType type_, TagType valueType, std::vector<Payload *> * values);
            Tag(std::string name_, TagType type_, std::vector<Tag *> * tags);
            ~Tag();

            //========== create tag ==========

            // reads an uncompressed or gzipped file
            Tag * loadFromFile(std::string path);

            // reads from uncompressed array
            Tag * loadFromBytestream(Bytestream * data);

            // loads the chunk at (x,z) of the world at the path
            Tag * loadFromChunk(std::string path, long int chunkx, long int chunkz);

            //========== write tag ==========

            // writes to an uncompressed file
            bool writeToFileUncompressed(std::string path);

            // writes to a gzipped file
            bool writeToFileCompressed(std::string path);

            // writes to a Bytestream
            Bytestream * writeToBytestream();

            // writes the chunk at (x,z) of the world at the path
            bool writeToChunk(std::string path, long int chunkx, long int chunkz);

            //========== get information ==========

            // get tag name
            std::string getName() const;

            // get tag type
            TagType getType() const;

            //========== get content ==========

            // get tag as string (type, name, and value)
            // prints compounds and lists as json-style tree
            std::string toString() const;

            // get value if numeric
            // 0 if not
            // may be rounded if floating point number
            int64_t asInt() const;

            // get value if numeric
            // 0 if not
            double asFloat() const;

            // get value as string
            // might contain '\n' if list or compound
            std::string asString() const;

            // gets child if type is compound or list
            // format: "list.42.intHolder..myInt."
            // (multiple dots are like one dot, dots at the end are ignored)
            Tag * getSubTag(std::string path);

            // get the size of the list
            // 0 if no list
            int32_t getListSize() const;

            // get the type of the list
            // tagTypeInvalid if no list
            TagType getListType() const;

            // gets the ith item of a number list
            // 0 if no such type or out of bounds
            // may be rounded if list of floating point numbers
            int64_t getListItemAsInt(int32_t i) const;

            // gets the ith item of a number list
            // 0.0 if no such type or out of bounds
            double getListItemAsFloat(int32_t i) const;

            // gets the ith item of a list as string
            // "" if out of bounds
            // may contain '\n' if list or compound
            std::string getListItemAsString(int32_t i) const;

            // gets the ith item of a list
            // 0 if out of bounds
            Tag * getListItemAsTag(int32_t i) const;

            //========== useful functions ==========

            // converts a TagType into a human-readable string
            static std::string tagTypeToString(TagType type);

            // changes the endianness of a variable of any type
            static void swapBytes(void * data, unsigned char length);

        private:
            TagType type;
            std::string name;
            Payload * payload;
            bool payloadToBeDeleted;
            // If true, any pointer in the payload will be removed when destroyed.
            // If false, assume the value is copied from elsewhere and deleted there.

            //========== private functions ==========

            // returns true if type is
            // isIntType:   tagTypeByte, tagTypeShort, tagTypeInt, or tagTypeLong
            // isFloatType: tagTypeFloat or tagTypeDouble
            // isListType:  tagTypeByteArray, tagTypeIntArray, or tagTypeList
            static bool isIntType(TagType type);
            static bool isFloatType(TagType type);
            static bool isListType(TagType type);

            // reads the payload from the Bytestream and returns it
            Payload * readPayload(TagType type, Bytestream * data);

            // creates a tag from the supplied payload
            // returns NULL if invalid type or payload
            static Tag * createTagFromPayload(std::string name, TagType type, Payload * payload);

            // free compound, list, or array before deleting payload pointer
            void safeRemovePayload();
            void safeRemovePayload(Payload * payloadi, TagType type);
    };

}

#endif

