/* Tag.cpp
 *
 * A class for loading and accessing NBT data
 *
 * TODO tests for NULL-pointer, range, etc.
 * TODO write tag to file and chunk
 *
 * by Gjum <gjum42@gmail.com>
 */

#include "Tag.h"

#include <stdexcept> // TODO make string to int conversion better

//#define DEBUG if (1)
#ifndef DEBUG
#define DEBUG if (0)
#endif

namespace NBT {

    Tag::Tag() {
        name = "";
        type = tagTypeInvalid;
        payload = NULL;
        payloadToBeDeleted = true;
    }

    Tag::Tag(std::string name_, TagType type_, int64_t val) {
        name = name_;
        type = type_;
        payload = new Payload;
        payload->tagInt = val;
        payloadToBeDeleted = true;
    }

    Tag::Tag(std::string name_, TagType type_, double val) {
        name = name_;
        type = type_;
        payload = new Payload;
        payload->tagFloat = val;
        payloadToBeDeleted = true;
    }

    Tag::Tag(std::string name_, TagType type_, std::string val) {
        name = name_;
        type = type_;
        payload = new Payload;
        payload->tagString = new std::string(val);
        payloadToBeDeleted = true;
    }

    Tag::Tag(std::string name_, TagType type_, TagType valueType, std::vector<Payload *> * values) {
        name = name_;
        type = type_;
        payload = new Payload;
        payload->tagList.type = valueType;
        payload->tagList.values = values;
        payloadToBeDeleted = false;
    }

    Tag::Tag(std::string name_, TagType type_, std::vector<Tag *> * tags) {
        name = name_;
        type = type_;
        payload = new Payload;
        payload->tagCompound = tags;
        payloadToBeDeleted = false;
    }

    Tag::~Tag() {
        safeRemovePayload();
    }

    //========== create tag ==========

    // reads an uncompressed or gzipped file
    Tag * Tag::loadFromFile(std::string path) {
        DEBUG printf("Reading file '%s' ...\n", path.c_str());
        // read buffer from file
        gzFile file = gzopen(path.c_str(), "rb");
        if (file == NULL) {
            DEBUG printf("ERROR: Could not open file\n");
            return this;
        }
        const unsigned short stepSize = 4096;
        unsigned long int bufSize = stepSize;
        char * buffer = new char[bufSize];
        for (;;) { // breaks on success
            int bytesRead = gzread(file, buffer+(bufSize-stepSize), stepSize);
            // error while ungzipping?
            // TODO better error handling
            if (bytesRead < 0) {
                printf("Error in gzread (readBytes < 0)\n");
                return this;
            }
            if (bytesRead < stepSize) break; // EOF, success
            // buffer too small, increase buffer size
            unsigned long int oldSize = bufSize;
            bufSize += stepSize;
            char * oldBuffer = buffer;
            buffer = new char[bufSize];
            memcpy(buffer, oldBuffer, oldSize);
            delete[] oldBuffer;
        }
        gzclose_w(file);

        // parse data
        loadFromBytestream(new Bytestream(buffer, bufSize));
        delete[] buffer;
        DEBUG printf("File reading successful.\n");
        return this;
    }

    // reads from uncompressed array
    Tag * Tag::loadFromBytestream(Bytestream * data) {
        //DEBUG printf("readTag: data=%#x\n", data);
        safeRemovePayload();
        name = "";
        type = static_cast<TagType>(data->get());
        DEBUG printf("type=%i\n", type);
        if (type < 0 || type > 11) {
            printf("ERROR: invalid type %i %#x\n", (int) type, (int) type);
            return this;
        }
        else if (type != tagTypeEnd) {
            // read tag name
            int16_t nameSize = 0;
            data->getInverseEndian(&nameSize, 2);
            DEBUG printf("nameSize=%i\n", nameSize);
            // name = ""; was done before
            for (int i = 0; i < nameSize; i++)
                name += data->get();
            DEBUG printf("name=%s\n", name.c_str());
            // read payload
            payload = readPayload(type, data);
            // TODO test if payload could be read
            payloadToBeDeleted = true;
        }
        return this;
    }

    // loads the chunk at (x,z) of the world at the path
    // returns 0 if chunk is empty or any other error occured
    // TODO check if chunk is populated or empty
    Tag * Tag::loadFromChunk(std::string worldpath, long int chunkx, long int chunkz) {
        // open file
        int regx = chunkx >> 5;
        int regz = chunkz >> 5;
        long int chunkxPositive = chunkx;
        long int chunkzPositive = chunkz;
        while (chunkxPositive < 0) chunkxPositive += 32;
        while (chunkzPositive < 0) chunkzPositive += 32;
        unsigned int chunkID = (chunkxPositive % 32) + (chunkzPositive % 32) * 32;
        DEBUG printf("chunk %i %i\nregion %i %i\nid %i",
                chunkx, chunkz, regx, regz, chunkID);
        std::string filePath = worldpath + "/region/r." + std::to_string(regx) + "." + std::to_string(regz) + ".mca";
        std::ifstream file(filePath, std::ifstream::in | std::ifstream::binary);

        // TODO test if file is open

        // read file header
        int32_t chunkPos = 0;
        file.seekg(chunkID*4);
        file.read((char*)(&chunkPos), 4);
        DEBUG printf("chunkPos before: %i %1$8x\n", chunkPos);
        Tag::swapBytes((unsigned char*) &chunkPos, 4);
        chunkPos = chunkPos >> 8;
        DEBUG printf("chunkPos=%i %1$8x\n", chunkPos);

        if (chunkPos == 0) return this;

        // TODO read chunk date?

        // read chunk header
        int32_t lengthCompressed = 0;
        file.seekg(chunkPos*4096);
        file.read((char*)(&lengthCompressed), 4);
        Tag::swapBytes((unsigned char*) &lengthCompressed, 4);

        if (lengthCompressed == 0) return this;

        // TODO check compression type

        // read compressed chunk data
        unsigned char bufferCompressed[int(lengthCompressed*1.1)];
        file.seekg(chunkPos*4096+5);
        file.read((char*) bufferCompressed, lengthCompressed);
        file.close();

        // print bufferCompressed
        DEBUG {
            printf("lengthCompressed=%i\n", lengthCompressed);
            for (int i = 0; i < lengthCompressed && i < 512; i++) {
                printf("%2x ", (unsigned char) bufferCompressed[i]);
                if (i%8 == 7) printf(" ");
                if (i%32 == 31) printf("\n");
            }
        }

        // uncompress buffer
        unsigned int step = 4096;
        long unsigned int lengthUncompressed = 65536;
        unsigned char *bufferUncompressed = new unsigned char[lengthUncompressed];
        for (;;) { // breaks on success
            int result = uncompress(
                    bufferUncompressed,
                    &lengthUncompressed,
                    bufferCompressed,
                    lengthCompressed);
            // error while unzipping?
            // TODO better error handling
            if (result == Z_MEM_ERROR) {
                //printf("Error in region file! Z_MEM_ERROR at chunk %i %i\n", chunkx, chunkz);
                delete[] bufferUncompressed;
                return this;
            }
            else if (result == Z_DATA_ERROR) {
                //printf("Error in region file! Z_DATA_ERROR at chunk %i %i\n", chunkx, chunkz);
                delete[] bufferUncompressed;
                return this;
            }
            else if (result != Z_BUF_ERROR) break; // success
            // buffer too small, increase buffer size
            lengthUncompressed += step;
            delete[] bufferUncompressed;
            bufferUncompressed = new unsigned char[lengthUncompressed];
        }

        // print bufferUncompressed
        DEBUG {
            printf("lengthUncompressed=%i\n", lengthUncompressed);
            for (long unsigned int i = 0; i < lengthUncompressed && i < 512; i++) {
                printf("%2x ", bufferUncompressed[i]);
                if (i%8 == 7) printf(" ");
                if (i%32 == 31) printf("\n");
            }
        }

        // create chunk from buffer
        Bytestream * data = (new Bytestream)->loadFromByteArray((char *) bufferUncompressed, lengthUncompressed);
        loadFromBytestream(data);

        // cleanup
        delete data;
        //delete[] bufferUncompressed;
        return this;
    }

    //========== write tag ==========

    // writes to an uncompressed file
    bool writeToFileUncompressed(std::string path) {
        return false;
    }

    // writes to a gzipped file
    bool writeToFileCompressed(std::string path) {
        return false;
    }

    // writes to a Bytestream
    Bytestream * writeToBytestream() {
        return NULL;
    }

    // writes the chunk at (x,z) of the world at the path
    bool writeToChunk(std::string path, long int chunkx, long int chunkz) {
        return false;
    }

    //========== get information ==========

    // get tag name
    std::string Tag::getName() const {
        return name;
    }

    // get tag type
    TagType Tag::getType() const {
        return type;
    }

    //========== get content ==========

    // get tag as string (type, name, and value)
    // prints compounds and lists as json-style tree
    std::string Tag::toString() const {
        return tagTypeToString(type) + "('" + name + "'): " + asString();
    }

    // get value if numeric
    // 0 if not
    // may be rounded if floating point number
    int64_t Tag::asInt() const {
        if (type == tagTypeByte
                || type == tagTypeShort
                || type == tagTypeInt
                || type == tagTypeLong)
            return payload->tagInt;
        if (type == tagTypeFloat
                || type == tagTypeDouble)
            return payload->tagFloat;
        return 0;
    }

    // get value if numeric
    // 0 if not
    double Tag::asFloat() const {
        if (type == tagTypeByte
                || type == tagTypeShort
                || type == tagTypeInt
                || type == tagTypeLong)
            return payload->tagInt;
        if (type == tagTypeFloat
                || type == tagTypeDouble)
            return payload->tagFloat;
        return 0;
    }

    // get value as string
    // might contain '\n' if list or compound
    std::string Tag::asString() const {
        if (isIntType(type)) return std::to_string(payload->tagInt);
        else if (isFloatType(type)) return std::to_string(payload->tagFloat);
        else if (type == tagTypeString) return *payload->tagString;
        else if (isListType(type) || type == tagTypeCompound) {
            std::string str = std::to_string(getListSize()) + " entries\n{\n";
            for (int32_t i = 0; i < getListSize(); i++) {
                // limit output to 10-15 lines
                if (isListType(type) && i >= 10 && getListSize() > 15) {
                    str += "  ... and " + std::to_string(getListSize()-10) + " more\n";
                    break;
                }
                Tag * tag = getListItemAsTag(i);
                if (tag != NULL) {
                    std::string content = "  " + tag->toString();
                    // indent content
                    for (size_t pos = content.find_first_of('\n'); pos != std::string::npos; pos = content.find_first_of('\n', pos+1)) {
                        content = content.replace(pos, 1, "\n  ");
                    }
                    str += content + "\n";
                }
                else str += "  ERROR\n";
            }
            str += "}";
            return str;
        }
        return "";
    }

    // gets child at the given path if type is compound or list
    // returns 0 on error
    // format: "list.42.intHolder..myInt."
    // (multiple dots are like one dot, dots at the end are ignored)
    Tag * Tag::getSubTag(std::string path) {
        size_t dotPos = path.find_first_of('.');
        std::string first = path.substr(0, dotPos);
        std::string rest  = "";
        if (dotPos < path.length()) rest = path.substr(dotPos+1);
        DEBUG printf("getSubTag: path='%s', first='%s', rest='%s' at tag '%s'\n", path.c_str(), first.c_str(), rest.c_str(), name.c_str());
        if (path.length() <= 0) return this; // recursion anchor
        if (first.length() <= 0) return getSubTag(rest); // allows "foo..bar." == "foo.bar"
        Tag * tag = NULL;
        if (isListType(type) || type == tagTypeCompound) {
            for (int32_t i = 0; i < getListSize(); i++) {
                tag = getListItemAsTag(i);
                if (tag == NULL) continue; // didn't get a tag ... why?
                if (first.compare(tag->getName()) == 0)
                    break;
                else tag = NULL; // do not remember incorrect tags
            }
        }
        // we still found no matching tag, try interpreting first as number and getting n-th tag
        if (tag == NULL) {
            try {
                tag = getListItemAsTag(stoi(first));
                DEBUG printf("getting tag #%i\n", stoi(first));
            }
            catch (const std::invalid_argument& ia) {
                tag = NULL;
            }
        }
        // still no luck? we're out of ideas, return nothing
        if (tag == NULL) return NULL;
        return tag->getSubTag(rest);
    }

    // get the size of the list or compound
    // 0 if no list or compound
    int32_t Tag::getListSize() const {
        if (isListType(type)) {
            return payload->tagList.values->size();
        }
        if (type == tagTypeCompound) {
            return payload->tagCompound->size();
        }
        return 0;
    }

    // gets the ith item of a number list
    // 0 if no such type or out of bounds
    // may be rounded if list of floating point numbers
    int64_t Tag::getListItemAsInt(int32_t i) const {
        if (isIntType(payload->tagList.type))
            return payload->tagList.values->at(i)->tagInt;
        if (isFloatType(payload->tagList.type))
            return payload->tagList.values->at(i)->tagFloat;
        return 0;
    }

    // gets the ith item of a number list
    // 0.0 if no such type or out of bounds
    double Tag::getListItemAsFloat(int32_t i) const {
        if (isIntType(payload->tagList.type))
            return payload->tagList.values->at(i)->tagInt;
        if (isFloatType(payload->tagList.type))
            return payload->tagList.values->at(i)->tagFloat;
        return 0.0;
    }

    // gets the ith item of a list as string
    // "" if out of bounds
    // may contain '\n' if list or compound
    std::string Tag::getListItemAsString(int32_t i) const {
        if (isIntType(payload->tagList.type))
            return std::to_string(payload->tagList.values->at(i)->tagInt);
        if (isFloatType(payload->tagList.type))
            return std::to_string(payload->tagList.values->at(i)->tagFloat);
        if (payload->tagList.type == tagTypeString)
            return *payload->tagList.values->at(i)->tagString;
        // no primitive type, use Tag::asString()
        Tag * tag = getListItemAsTag(i);
        if (tag) {
            std::string str = tag->asString();
            delete tag;
            return str;
        }
        return "";
    }

    // gets the ith item of a list or compound
    // NULL if out of bounds or no list or compound
    Tag * Tag::getListItemAsTag(int32_t i) const {
        if (i < 0 || i >= getListSize()) return NULL;
        if (isListType(type)) {
            return createTagFromPayload(std::to_string(i),
                    payload->tagList.type,
                    payload->tagList.values->at(i));
        }
        if (type == tagTypeCompound) {
            return payload->tagCompound->at(i);
        }
        return NULL;
    }

    //========== useful functions ==========

    // converts a TagType into a human-readable string
    std::string Tag::tagTypeToString(TagType type) {
        std::string strings[] = {
            "TAG_End",
            "TAG_Byte",
            "TAG_Short",
            "TAG_Int",
            "TAG_Long",
            "TAG_Float",
            "TAG_Double",
            "TAG_ByteArray",
            "TAG_String",
            "TAG_List",
            "TAG_Compound",
            "TAG_IntArray"
        };
        char tagID = static_cast<char>(type);
        if (tagID < 0 || tagID > 11) return "TAG_Invalid";
        return strings[tagID];
    }

    // changes the endianness of a variable of any type
    void Tag::swapBytes(void * data, unsigned char length) {
        char * addr = reinterpret_cast<char *>(data);
        for (unsigned char i = 0; i < length/2; i++) {
            unsigned char j = length-i-1;
            char swap = addr[i];
            addr[i]  = addr[j];
            addr[j] = swap;
        }
    }

    // ========== private functions ========== 

    // returns true if type is
    // isIntType:   tagTypeByte, tagTypeShort, tagTypeInt, or tagTypeLong
    // isFloatType: tagTypeFloat or tagTypeDouble
    // isListType:  tagTypeByteArray, tagTypeIntArray, or tagTypeList
    bool Tag::isIntType(TagType type) {
        return (type == tagTypeByte
                || type == tagTypeShort
                || type == tagTypeInt
                || type == tagTypeLong);
    }

    bool Tag::isFloatType(TagType type) {
        return (type == tagTypeFloat
                || type == tagTypeDouble);
    }

    bool Tag::isListType(TagType type) {
        return (type == tagTypeByteArray
                || type == tagTypeIntArray
                || type == tagTypeList);
    }

    // reads the payload from the Bytestream and returns it
    Payload * Tag::readPayload(TagType type, Bytestream * data) {
        Payload * payload = new Payload;
        payload->tagInt = 0;
        if (type == tagTypeByte) {
            payload->tagInt = data->get();
            DEBUG printf("tagByte=%i\n", payload->tagInt);
        }
        // integers
        else if (type == tagTypeShort) {
            int16_t value = 0;
            data->getInverseEndian(&value, 2);
            payload->tagInt = value;
            DEBUG printf("tagShort=%i\n", payload->tagInt);
        }
        else if (type == tagTypeInt) {
            int32_t value = 0;
            data->getInverseEndian(&value, 4);
            payload->tagInt = value;
            DEBUG printf("tagInt=%i\n", payload->tagInt);
        }
        else if (type == tagTypeLong) {
            int64_t value = 0;
            data->getInverseEndian(&value, 8);
            payload->tagInt = value;
            DEBUG printf("tagLong=%lld\n", payload->tagInt);
        }
        // floating point numbers
        else if (type == tagTypeFloat) {
            float value = 0;
            data->getInverseEndian(&value, 4);
            payload->tagFloat = value;
            DEBUG printf("tagFloat=%.2f\n", payload->tagFloat);
        }
        else if (type == tagTypeDouble) {
            double value = 0;
            data->getInverseEndian(&value, 8);
            payload->tagFloat = value;
            DEBUG printf("tagDouble=%.2f\n", payload->tagFloat);
        }
        // string type
        else if (type == tagTypeString) {
            int strLen = (uint16_t(data->get()) << 8) + data->get();
            payload->tagString = new std::string("");
            for (int i = 0; i < strLen; i++)
                *payload->tagString += data->get();
            DEBUG printf("tagString=%s\n", payload->tagString->c_str());
        }
        // tag holding types
        else if (isListType(type)) {
            DEBUG printf("tagTypeList...\n");
            TagType listType = tagTypeInvalid;
            if (type == tagTypeByteArray)     listType = tagTypeByte;
            else if (type == tagTypeIntArray) listType = tagTypeInt;
            else if (type == tagTypeList) {
                // read type
                listType = static_cast<TagType>(data->get());
                DEBUG printf("listType=%i\n", listType);
            }
            // read size
            int32_t size = 0;
            data->getInverseEndian(&size, 4);
            // read values
            payload->tagList.type = listType;
            payload->tagList.values = new std::vector<Payload *>;
            DEBUG printf("type=%i, size=%i\n", listType, size);
            for (int32_t i = 0; i < size; i++)
                payload->tagList.values->push_back(readPayload(listType, data));
            DEBUG printf("tagTypeList end\n");
        }
        else if (type == tagTypeCompound) {
            DEBUG printf("tagCompound...\n");
            TagType tagType;
            payload->tagCompound = new std::vector<Tag*>;
            while (1) { // breaks on TAG_End or error
                Tag *subTag = (new Tag)->loadFromBytestream(data);
                tagType = subTag->getType();
                if (tagType == tagTypeEnd) {
                    delete subTag;
                    break;
                }
                if (tagType < 0 || tagType > 11) {
                    printf("ERROR: unknown type %i %#x\n",
                            (int) tagType, (int) tagType);
                    delete subTag;
                    break;
                }
                payload->tagCompound->push_back(subTag);
            }
            DEBUG printf("tagCompound end\n");
        }
        else {
            printf("ERROR: unknown type %i %#x\n", (int) type, (int) type);
        }
        return payload;
    }

    // creates a tag from the supplied payload
    // returns NULL if invalid type or payload
    Tag * Tag::createTagFromPayload(std::string name, TagType type, Payload * payload) {
        if (payload == NULL) return NULL;
        if (type == tagTypeByte
                || type == tagTypeShort
                || type == tagTypeInt
                || type == tagTypeLong) {
            return new Tag(name, type, payload->tagInt);
        }
        else if (type == tagTypeFloat
                || type == tagTypeDouble) {
            return new Tag(name, type, payload->tagFloat);
        }
        else if (type == tagTypeString) {
            return new Tag(name, type, *payload->tagString);
        }
        else if (isListType(type)) {
            return new Tag(name, type, payload->tagList.type, payload->tagList.values);
        }
        else if (type == tagTypeCompound) {
            return new Tag(name, type, payload->tagCompound);
        }
        return NULL;
    }

    // free compound, list, or array before deleting payload pointer
    void Tag::safeRemovePayload() {
        safeRemovePayload(payload, type);
    }
    void Tag::safeRemovePayload(Payload * payload, TagType type) {
        DEBUG printf("Deleting payload of '%s' ...\n", name.c_str());
        if (payload != NULL) {
            if (type == tagTypeString && payloadToBeDeleted) {
                delete payload->tagString;
            }
            else if (type == tagTypeCompound && payloadToBeDeleted) {
                DEBUG printf("Deleting tagCompound, size=%i\n", payload->tagCompound->size());
                for (int i = 0; i < payload->tagCompound->size(); i++)
                    delete payload->tagCompound->at(i);
                delete payload->tagCompound;
            }
            else if (isListType(type) && payloadToBeDeleted) {
                DEBUG printf("Deleting list type, size=%i\n", payload->tagList.values->size());
                for (int i = 0; i < payload->tagList.values->size(); i++) {
                    DEBUG printf("Here goes entry %i ...\n", i);
                    safeRemovePayload(payload->tagList.values->at(i), payload->tagList.type);
                    //delete payload->tagList.values->at(i);
                }
                delete payload->tagList.values;
            }
            else DEBUG printf("Deleting primitive type, type=%s\n", tagTypeToString(type).c_str());
            delete payload;
        }
        else DEBUG printf("Not deleting, payload == NULL\n");
    }

}

