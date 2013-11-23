
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <zfstream.h>
#include <zlib.h>

//#define DEBUG if (1)
#ifndef DEBUG
#define DEBUG if (0)
#endif

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

NbtTag::NbtTag() {
	name = 0;
	payload = 0;
	type = tagTypeInvalid;
}

NbtTag::~NbtTag() {
	if (name != 0) delete name;
	safeRemovePayload();
}

// read file, output tree
void NbtTag::loadFromFile(char *path, bool gzipped) {
	printf("\nReading file '%s'...\n", path);
	DEBUG {
		printf("\nDebugging only, don't look!\n");
		gzifstream *file = new gzifstream;
		file->open(path);
		for (int i = 0; i < 64; i++) {
			if (i%16 == 0) printf("\n");
			char c = file->get();
			printf("%2x ", c);
		}
		file->close();
		delete file;
		printf("\n\nFile reading successful.\n\n");
	}
	if (gzipped) {
		gzifstream *file = new gzifstream;
		file->open(path);
		if (!file->is_open()) {
			printf("ERROR: Could not open file\n");
			return;
		}
		readTag(file);
		file->close();
		delete file;
	}
	else {
		std::ifstream *file = new std::ifstream;
		file->open(path);
		if (!file->is_open()) {
			printf("ERROR: Could not open file\n");
			return;
		}
		readTag(file);
		file->close();
		delete file;
	}
	printf("File reading successful.\n\n");
}

// read type from stream
NbtTagType NbtTag::readTag(std::istream *file) {
	//DEBUG printf("readTag: file=%#x\n", file);
	safeRemovePayload();
	type = static_cast<NbtTagType>(file->get());
	DEBUG printf("type=%i\n", type);
	if (type < 0 || type > 11) {
		printf("ERROR: invalid type %i %#x\n", (int) type, (int) type);
		return tagTypeInvalid;
	}
	else if (type == tagTypeEnd) {
		if (name != 0) delete[] name;
		name = 0;
	}
	else {
		// read tag name
		int nameSize = (uint16_t(file->get()) << 8) | file->get();
		DEBUG printf("nameSize=%i\n", nameSize);
		if (name != 0) delete[] name;
		name = new char[nameSize+1];
		for (int i = 0; i < nameSize; i++)
			name[i] = file->get();
		name[nameSize] = '\0';
		DEBUG printf("name=%s\n", name);
		// read payload
		payload = readPayload(type, file);
	}
	return type;
}

// print tree
// indent with with <depth*2> spaces
void NbtTag::printTag(unsigned int depth) {
	printPayload(type, payload, depth);
}

void NbtTag::printPayload(NbtTagType type, NbtPayload* payload, unsigned int depth) {
	// some formatting
	char spacing[128];
	spacing[0] = '\0';
	for (unsigned int i = 0; i < depth; i++)
		sprintf(spacing, "%s  ", spacing);
	printf("%s", spacing);
	// payload
	if (type == tagTypeEnd) {
		printf("TAG_End\n");
	}
	else if (type == tagTypeByte) {
		printf("TAG_Byte('%s'): %i\n", name, payload->tagByte);
	}
	else if (type == tagTypeShort) {
		printf("TAG_Short('%s'): %i\n", name, (int) payload->tagShort);
	}
	else if (type == tagTypeInt) {
		printf("TAG_Int('%s'): %i\n", name, payload->tagInt);
	}
	else if (type == tagTypeLong) {
		printf("TAG_Long('%s'): %ld\n", name, payload->tagLong);
	}
	else if (type == tagTypeFloat) {
		printf("TAG_Float('%s'): %.2f\n", name, payload->tagFloat);
	}
	else if (type == tagTypeDouble) {
		printf("TAG_Double('%s'): %.2f\n", name, payload->tagDouble);
	}
	else if (type == tagTypeString) {
		printf("TAG_String('%s'): %s\n", name, payload->tagString);
	}
	else if (type == tagTypeList
			|| type == tagTypeByteArray
			|| type == tagTypeIntArray) {
		if (type == tagTypeList)
			printf("TAG_List");
		else if (type == tagTypeByteArray)
			printf("TAG_ByteArray");
		else if (type == tagTypeIntArray)
			printf("TAG_IntArray");
		printf("('%s'): %i entries\n",
				name,
				payload->tagList.size);
		printf("%s{\n", spacing);
		for (int32_t i = 0; i < payload->tagList.size; i++) {
			// limit output to 10-15 lines
			if (i >= 10 && payload->tagList.size > 15) {
				printf("%s  ... and %i more\n", spacing, payload->tagList.size-10);
				break;
			}
			// TODO why does it output the List/Array name?
			printPayload(
					payload->tagList.type,
					payload->tagList.values[i],
					depth+1);
		}
		printf("%s}\n", spacing);
	}
	else if (type == tagTypeCompound) {
		printf("TAG_Compound('%s'): %i entries\n",
				name,
				payload->tagCompound->size());
		printf("%s{\n", spacing);
		for (int i = 0; i < payload->tagCompound->size(); i++) {
			payload->tagCompound->at(i)->printTag(depth+1);
		}
		printf("%s}\n", spacing);
	}
	else {
		printf("ERROR: unknown type %i %#x\n", (int) type, (int) type);
	}
}

// TODO ensure the variables are in big endian
NbtPayload * NbtTag::readPayload(NbtTagType type, std::istream *file) {
	NbtPayload *payload = new NbtPayload;
	if (type == tagTypeByte) {
		payload->tagByte = file->get();
		DEBUG printf("tagByte=%i\n", payload->tagByte);
	}
	// integers
	else if (type == tagTypeShort) {
		for (int i = 16; i >= 8; i -= 8)
			payload->tagShort |= uint16_t(file->get()) << (i-8);
		DEBUG printf("tagShort=%i\n", payload->tagShort);
	}
	else if (type == tagTypeInt) {
		for (int i = 32; i >= 8; i -= 8)
			payload->tagInt |= uint32_t(file->get()) << (i-8);
		DEBUG printf("tagInt=%i\n", payload->tagInt);
	}
	else if (type == tagTypeLong) {
		for (int i = 64; i >= 8; i -= 8)
			payload->tagLong |= int64_t(file->get()) << i-8;
		DEBUG printf("tagLong=%lld\n",
				payload->tagLong);
	}
	// floating point numbers
	else if (type == tagTypeFloat) {
		for (int i = 32; i >= 8; i -= 8)
			*((int32_t*)&payload->tagFloat) |= int32_t(file->get()) << i-8;
		DEBUG printf("tagFloat=%.2f\n", payload->tagFloat);
	}
	else if (type == tagTypeDouble) {
		for (int i = 64; i >= 8; i -= 8)
			*((int64_t*)&payload->tagDouble) |= int64_t(file->get()) << i-8;
		DEBUG printf("tagDouble=%.2f\n", payload->tagDouble);
	}
	// string type
	else if (type == tagTypeString) {
		int strLen = (uint16_t(file->get()) << 8) + file->get();
		payload->tagString = new char[strLen+1];
		for (int i = 0; i < strLen; i++)
			payload->tagString[i] = file->get();
		payload->tagString[strLen] = '\0';
		DEBUG printf("tagString=%s\n", payload->tagString);
	}
	// tag holding types
	else if (type == tagTypeByteArray) {
		DEBUG printf("tagTypeByteArray...\n");
		NbtTagType type = tagTypeByte;
		// read size
		int32_t size = 0;
		for (int i = 32; i >= 8; i -= 8)
			size |= int32_t(file->get()) << (i-8);
		DEBUG printf("size=%i\n", size);
		// read values
		payload->tagList.values = new NbtPayload*[size];
		payload->tagList.type = type;
		payload->tagList.size = size;
		DEBUG printf("size=%i\n", payload->tagList.size);
		for (int32_t i = 0; i < size; i++)
			payload->tagList.values[i] = readPayload(type, file);
		DEBUG printf("tagTypeByteArray end\n");
	}
	else if (type == tagTypeIntArray) {
		DEBUG printf("tagTypeIntArray...\n");
		NbtTagType type = tagTypeInt;
		// read size
		int32_t size = 0;
		for (int i = 32; i >= 8; i -= 8)
			size |= int32_t(file->get()) << (i-8);
		DEBUG printf("size=%i\n", size);
		// read values
		payload->tagList.values = new NbtPayload*[size];
		payload->tagList.type = type;
		payload->tagList.size = size;
		DEBUG printf("size=%i\n", payload->tagList.size);
		for (int32_t i = 0; i < size; i++)
			payload->tagList.values[i] = readPayload(type, file);
		DEBUG printf("tagTypeIntArray end\n");
	}
	else if (type == tagTypeList) {
		DEBUG printf("tagList...\n");
		// read type
		NbtTagType type = static_cast<NbtTagType>(file->get());
		//payload->tagList.type = type;
		DEBUG printf("type=%i\n", type);
		// read size
		int32_t size = 0;
		for (int i = 32; i >= 8; i -= 8)
			size |= int32_t(file->get()) << (i-8);
		//payload->tagList.size = size;
		DEBUG printf("size=%i\n", size);
		// read values
		payload->tagList.values = new NbtPayload*[size];
		payload->tagList.type = type;
		payload->tagList.size = size;
		DEBUG printf("type=%i size=%i\n", payload->tagList.type, payload->tagList.size);
		for (int32_t i = 0; i < size; i++)
			payload->tagList.values[i] = readPayload(type, file);
		DEBUG printf("tagList end\n");
	}
	else if (type == tagTypeCompound) {
		DEBUG printf("tagCompound...\n");
		NbtTagType tagType;
		payload->tagCompound = new std::vector<NbtTag*>;
		while (1) { // breaks on TAG_End or error
			NbtTag *subTag = new NbtTag;
			tagType = subTag->readTag(file);
			if (tagType == 0) {
				delete subTag;
				break;
			}
			if (tagType < 0 || tagType > 11) {
				printf("ERROR: unknown type %i %#x\n",
						(int) tagType, (int) tagType);
				delete subTag;
				break;
			}
			payload->tagCompound->insert(
					payload->tagCompound->end(),
					subTag);
		}
		DEBUG printf("tagCompound end\n");
	}
	// unknown type
	else {
		printf("ERROR: unknown type %i %#x\n",
				(int) type, (int) type);
	}
	return payload;
}

// free string, compound, list or array before deleting payload pointer
void NbtTag::safeRemovePayload() {
	if (payload != 0) {
		if (type == tagTypeString) {
			delete[] payload->tagString;
		}
		else if (type == tagTypeCompound) {
			for (int i = 0; i < payload->tagCompound->size(); i++)
				delete payload->tagCompound->at(i);
			delete payload->tagCompound;
		}
		else if (type == tagTypeList
				|| type == tagTypeByteArray
				|| type == tagTypeIntArray) {
			for (int i = 0; i < payload->tagList.size; i++)
				delete payload->tagList.values[i];
			delete[] payload->tagList.values;
		}
		delete payload;
	}
}

// print a tree of the provided file
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

