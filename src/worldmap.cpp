/* worldmap.cpp
 *
 * Takes a minecraft world path and renders the map into "worldmap.png".
 * Rendered are widthxheight blocks around the center, each block zoomxzoom pixel large.
 * Optionally prints center x, center z, width, and height of the map.
 * WARNING: Not all blocks are in the color table, unknown blocks are rendered transparent.
 * The current color data is from Lahwran's ZanMinimap project: <http://github.com/lahwran/zanmini>.
 *
 * Arguments: <worldpath> [center x=0] [center z=0] [width=256] [height=256] [zoom=1] [info text size=10]
 *
 * - worldpath: The path to the Minecraft world.
 *     - Example: "saves/Legio-Umbra/"
 * - center x: The x coordinate of the block at the center of the image.
 *     - Example: 500
 * - center z: The z coordinate of the block at the center of the image.
 *     - Example: -432
 * - width: The x range of the blocks in the image.
 *     - Example: 600
 * - height: The z range of the blocks in the image.
 *     - Example: 400
 * - zoom: The size of each map pixel.
 *     - Example: 5
 * - info text size: The font size to use for the info text. 0 for no text.
 *     - Example: 12
 *
 * Example: worldmap saves/Legio-Umbra/ 500 -432 600 400 5 12
 *
 * Renders "saves/Legio-Umbra/" with 5x5 block size and prints various data in font size 12.
 * The image contains all blocks between 200,-632 and 800,-232 inclusive.
 *
 * by Gjum <gjum42@gmail.com> <http://gjum.sytes.net/>
 */

#include <cairo/cairo.h>
#include "nbt/Tag.h"

enum TintType {
    NONE, GRASS, FOLIAGE, PINE, BIRCH, REDSTONE, COLORMULT
};

class BlockColor {
    public:
        int color;
        char alpha;
        TintType tintType;

        BlockColor(int color_, char alpha_, TintType tintType_) {
            color = color_;
            alpha = alpha_;
            tintType = tintType_;
        }
        ~BlockColor() {}
};

BlockColor * blockColors[4096]; // 2^(8+4), id has 8 bit, meta has 4 bit

int blockColorID(int id, int meta) {
    return id | (meta << 8);
}

BlockColor * blockColorOf(int id, int meta) {
    return blockColors[blockColorID(id, meta)];
}

unsigned char toRed(BlockColor * color) {
    if (color == NULL) return 0;
    return (color->color >> 16) & 0xff;
}

unsigned char toGreen(BlockColor * color) {
    if (color == NULL) return 0;
    return (color->color >> 8) & 0xff;
}

unsigned char toBlue(BlockColor * color) {
    if (color == NULL) return 0;
    return (color->color) & 0xff;
}

class ChunkContainer {
    public:
        ChunkContainer(std::string worldpath, long int blockx, long int blockz, long int blockwidth, long int blockheight) {
            x = blockx >> 4;
            z = blockz >> 4;
            width  = (blockx+blockwidth  >> 4) + 1 - x;
            height = (blockz+blockheight >> 4) + 1 - z;
            chunks = new NBT::Tag*[width*height];
            printf("Loading chunks %i,%i %ix%i ...\n", x, z, width, height);
            int unloadedCounter = 0;
            for (int j = 0; j < height; j++) {
                for (int i = 0; i < width; i++) {
                    NBT::Tag * chunk = (new NBT::Tag)->loadFromChunk(worldpath, i+x, j+z);
                    chunks[i+j*width] = chunk;
                    if (chunk == NULL) {
                        //printf("Chunk %3i,%-3i not loaded: chunk=%x\n", i+x, j+z, chunk);
                        unloadedCounter++;
                    }
                    //else printf("Chunk %3i,%-3i loaded: chunk=%x inArray=%x\n", i+x, j+z, chunk, chunks[i+j*width]);
                }
            }
            printf("%i of %i Chunks loaded.\n", width*height-unloadedCounter, width*height);
        }
        ~ChunkContainer() {
            for (int i = 0; i < width*height; i++) {
                if (chunks[i] != NULL) delete chunks[i];
            }
            delete[] chunks;
        }
        NBT::Tag * getChunkFromBlockPos(long int blockx, long int blockz) {
            int chunkx = blockx >> 4;
            int chunkz = blockz >> 4;
            //printf("Requested block %i,%i which is in chunk %i,%i\n", blockx, blockz, chunkx, chunkz);
            if (chunkx < x || chunkz < z || chunkx >= x+width || chunkz >= z+height) {
                printf("Out of range, chunk not loaded\n");
                return NULL;
            }
            NBT::Tag * chunk = chunks[chunkx-x+(chunkz-z)*width];
            if (chunk == NULL) {
                printf("Chunk not loaded\n");
                return NULL;
            }
            return chunk;
        }
    private:
        long int x, z, width, height;
        NBT::Tag ** chunks;
};

BlockColor * colorAt(int x, int z, ChunkContainer * chunkContainer) {
    // get the block data
    //printf("\ncolorAt: x=%i, z=%i\n", x, z);
    NBT::Tag * chunk = chunkContainer->getChunkFromBlockPos(x, z);
    if (chunk == NULL) {
        printf("colorAt: chunk=NULL\n");
        return blockColors[0];
    }
    //printf("colorAt: chunk=%x\n", chunk);

    int xInChunk = x; while (xInChunk < 0){xInChunk += 16;} xInChunk = xInChunk%16;
    int zInChunk = z; while (zInChunk < 0){zInChunk += 16;} zInChunk = zInChunk%16;
    //printf("colorAt: InChunk: %i,%i\n", xInChunk, zInChunk);

    NBT::Tag * subTag = chunk->getSubTag("Level.HeightMap");
    if (subTag == NULL) {
        printf("colorAt: subTag=NULL\n");
        return blockColors[0];
    }
    //printf("colorAt: subTag=%x\n", subTag);

    int y = subTag->getListItemAsInt(xInChunk+zInChunk*16)-1;
    int sectionID = y/16;
    int blockIndex = xInChunk+zInChunk*16+(y%16)*16*16;
    //printf("colorAt: y=%i section=%i block=%i\n", y, sectionID, blockIndex);

    int id = (unsigned char) chunk->getSubTag("Level.Sections")->getListItemAsTag(sectionID)->getSubTag("Blocks")->getListItemAsInt(blockIndex);
    if (id < 0 || id > 0xff) {
        printf("colorAt: Invalid id: %i\n", id);
        return blockColors[0];
    }

    // upper/lower 4 bit of the byte
    int meta = (chunk->getSubTag("Level.Sections")->getListItemAsTag(sectionID)->getSubTag("Data")->getListItemAsInt(blockIndex/2) >> (blockIndex%2)*4) & 0x0F;
    if (meta < 0 || meta > 0xf) {
        printf("colorAt: Invalid meta: %i\n", meta);
        return blockColors[0];
    }

    //printf("colorAt: xc=%i zc=%i y=%i blockIndex=%i id=%i meta=%i\n", xInChunk, zInChunk, y, blockIndex, id, meta);

    // finally we get the color of the block
    BlockColor * color = blockColors[blockColorID(id, meta)];
    if (color == NULL) {
        color = blockColors[blockColorID(id, 0)];
        if (color == NULL) {
            printf("colorAt: Didn't get color: block %i:%i, colorID=%i\n", id, meta, blockColorID(id, meta));
            return blockColors[0];
        }
        //printf("colorAt: Didn't get color: block %i:%i, colorID=%i, used meta=0\n", id, meta, blockColorID(id, meta));
        return color;
    }
    return color;
}

BlockColor * colorAtChunk(int x, int z, NBT::Tag * chunk) {
    if (chunk == NULL) return blockColors[0];

    NBT::Tag * level = chunk->getSubTag("Level");
    if (level == NULL) return blockColors[0]; // The chunk is a spy! but not a chunk

    // block position
    int y = level->getSubTag("HeightMap")->getListItemAsInt(x+z*16)-1;
    if (y <= 0) return blockColors[0]; // no blocks present
    int sectionID = y/16;
    int blockIndex = x+z*16+(y%16)*16*16;

    NBT::Tag * section = level->getSubTag("Sections")->getListItemAsTag(sectionID);
    if (section == NULL) return blockColors[0]; // The section is a spy! but not a section

    // block id
    int id = (unsigned char) section->getSubTag("Blocks")->getListItemAsInt(blockIndex);
    if (id < 0 || id > 0xff) {
        printf("colorAt: Invalid id: %i\n", id);
        return blockColors[0];
    }

    // block metadata (upper/lower 4 bit of the byte)
    int meta = (section->getSubTag("Data")->getListItemAsInt(blockIndex/2) >> (blockIndex%2)*4) & 0x0F;
    if (meta < 0 || meta > 0xf) {
        printf("colorAt: Invalid meta: %i\n", meta);
        return blockColors[0];
    }

    //delete section;

    // finally we get the color of the block
    BlockColor * color = blockColors[blockColorID(id, meta)];
    if (color == NULL) {
        // unknown metadata? fallback to meta=0
        color = blockColors[blockColorID(id, 0)];
        if (color == NULL) {
            // unknown block id? draw air
            //printf("colorAt: Didn't get color: block %i:%i, colorID=%i\n", id, meta, blockColorID(id, meta));
            return blockColors[0];
        }
        //printf("colorAt: Didn't get color: block %i:%i, colorID=%i, used meta=0\n", id, meta, blockColorID(id, meta));
        return color;
    }
    return color;
}

void renderChunk(cairo_t * cr, std::string worldpath, int chunkx, int chunkz, int imgx, int imgz, int zoom) {
    NBT::Tag * chunk = (new NBT::Tag)->loadFromChunk(worldpath, imgx/16+chunkx, imgz/16+chunkz);
    for (int z = 0; z < 16; z++) {
        for (int x = 0; x < 16; x++) {
            // get color values
            BlockColor * color = colorAtChunk(x, z, chunk);
            if (color->alpha == 0) continue; // transparent
            float r = toRed(color)   / 256.0;
            float g = toGreen(color) / 256.0;
            float b = toBlue(color)  / 256.0;
            cairo_set_source_rgb(cr, r,g,b);
            cairo_rectangle(cr, x*zoom+(chunkx)*16, z*zoom+(chunkz)*16, zoom, zoom);
            cairo_fill(cr);
        }
    }
    delete chunk;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Usage: %s <worldpath> [center x=0] [center z=0] [width=256] [height=256] [zoom=1] [info text size=10]\n", argv[0]);
        return 0;
    }
    char * worldpath = argv[1];
    int centerx = 0;
    int centerz = 0;
    int width   = 256;
    int height  = 256;
    unsigned int zoom = 1;
    unsigned int infoSize = 10;
    if (argc > 2) centerx  = atoi(argv[2]);
    if (argc > 3) centerz  = atoi(argv[3]);
    if (argc > 4) width    = atoi(argv[4]);
    if (argc > 5) height   = atoi(argv[5]);
    if (argc > 6) zoom     = atoi(argv[6]);
    if (argc > 7) infoSize = atoi(argv[7]);
    printf("Args: worldpath=%s centerx=%i centerz=%i width=%i height=%i zoom=%i info=%i\n", worldpath, centerx, centerz, width, height, zoom, infoSize);

    // load chunks
    //ChunkContainer * chunkContainer = new ChunkContainer(worldpath, centerx-width/2, centerz-height/2, width, height);

    printf("Building color table with %i colors...\n", blockColorID(91, 16)+1);
    // build color table
    //reused colors
    int wood = 0xbc9862;
    int water = 0x3256ff;
    int lava = 0xd96514;
    //                       id, meta                    r g b  alpha
    blockColors[blockColorID( 0,  0)] = new BlockColor(0xff00ff, 0   , TintType::NONE); //air
    blockColors[blockColorID( 1,  0)] = new BlockColor(0x686868, 0xff, TintType::NONE); //stone
    blockColors[blockColorID( 2,  0)] = new BlockColor(0x74b44a, 0xff, TintType::GRASS); //grass
    blockColors[blockColorID( 3,  0)] = new BlockColor(0x79553a, 0xff, TintType::NONE); //dirt
    blockColors[blockColorID( 3,  1)] = new BlockColor(0x79553a, 0xff, TintType::NONE); //unchangeable dirt
    blockColors[blockColorID( 4,  0)] = new BlockColor(0x959595, 0xff, TintType::NONE); //cobble
    blockColors[blockColorID( 5,  0)] = new BlockColor(wood    , 0xff, TintType::NONE); //wood
    blockColors[blockColorID( 6,  0)] = new BlockColor(0xa2c978, 0x80, TintType::FOLIAGE); //sapling 1
    blockColors[blockColorID( 6,  1)] = new BlockColor(0xa2c978, 0x80, TintType::PINE); //sapling 2
    blockColors[blockColorID( 6,  2)] = new BlockColor(0xa2c978, 0x80, TintType::BIRCH); //sapling 3
    blockColors[blockColorID( 7,  0)] = new BlockColor(0x333333, 0xff, TintType::NONE); //bedrock
    blockColors[blockColorID( 8,  0)] = new BlockColor(water   , 0xc0, TintType::NONE); //water
    blockColors[blockColorID( 9,  0)] = new BlockColor(water   , 0xb0, TintType::NONE); //moving water
    blockColors[blockColorID(10,  0)] = new BlockColor(lava    , 0xff, TintType::NONE); //lava
    blockColors[blockColorID(11,  0)] = new BlockColor(lava    , 0xff, TintType::NONE); //moving lava
    blockColors[blockColorID(12,  0)] = new BlockColor(0xddd7a0, 0xff, TintType::NONE); //sand
    blockColors[blockColorID(12,  1)] = new BlockColor(0xff6600, 0xff, TintType::NONE); //red sand
    blockColors[blockColorID(13,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //gravel
    blockColors[blockColorID(14,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //gold ore
    blockColors[blockColorID(15,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //iron ore
    blockColors[blockColorID(16,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //coal ore
    blockColors[blockColorID(17,  0)] = new BlockColor(0x675132, 0xff, TintType::NONE); //log 1
    blockColors[blockColorID(17,  1)] = new BlockColor(0x342919, 0xff, TintType::NONE); //log 2
    blockColors[blockColorID(17,  2)] = new BlockColor(0xc8c29f, 0xff, TintType::NONE); //log 3
    blockColors[blockColorID(18,  0)] = new BlockColor(0x164d0c, 0xa0, TintType::NONE); //leaf
    blockColors[blockColorID(19,  0)] = new BlockColor(0xe5e54e, 0xff, TintType::NONE); //sponge
    blockColors[blockColorID(20,  0)] = new BlockColor(0xffffff, 0x80, TintType::NONE); //glass
    blockColors[blockColorID(21,  0)] = new BlockColor(0x6d7484, 0xff, TintType::NONE); //lapis ore
    blockColors[blockColorID(22,  0)] = new BlockColor(0x1542b2, 0xff, TintType::NONE); //lapis
    blockColors[blockColorID(23,  0)] = new BlockColor(0x585858, 0xff, TintType::NONE); //dispenser
    blockColors[blockColorID(24,  0)] = new BlockColor(0xc6bd6d, 0xff, TintType::NONE); //sandstone
    blockColors[blockColorID(25,  0)] = new BlockColor(0x784f3a, 0xff, TintType::NONE); //noteblock
    blockColors[blockColorID(26,  0)] = new BlockColor(0xa95d5d, 0xff, TintType::NONE); //bed
    // skip 27, 28, 30, 31, and 32 as they are all nonsolid and notch's height map skips them
    blockColors[blockColorID(35,  0)] = new BlockColor(0xe1e1e1, 0xff, TintType::NONE); //colored wool
    blockColors[blockColorID(35,  1)] = new BlockColor(0xeb8138, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  2)] = new BlockColor(0xc04cca, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  3)] = new BlockColor(0x698cd5, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  4)] = new BlockColor(0xc5b81d, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  5)] = new BlockColor(0x3cbf30, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  6)] = new BlockColor(0xda859c, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  7)] = new BlockColor(0x434343, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  8)] = new BlockColor(0x9fa7a7, 0xff, TintType::NONE);
    blockColors[blockColorID(35,  9)] = new BlockColor(0x277697, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 10)] = new BlockColor(0x7f33c1, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 11)] = new BlockColor(0x26339b, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 12)] = new BlockColor(0x57331c, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 13)] = new BlockColor(0x384e18, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 14)] = new BlockColor(0xa52d28, 0xff, TintType::NONE);
    blockColors[blockColorID(35, 15)] = new BlockColor(0x1b1717, 0xff, TintType::NONE); //end colored wool
    blockColors[blockColorID(37,  0)] = new BlockColor(0xf1f902, 0xff, TintType::NONE); //yellow flower
    blockColors[blockColorID(38,  0)] = new BlockColor(0xf7070f, 0xff, TintType::NONE); //red flower
    blockColors[blockColorID(39,  0)] = new BlockColor(0x916d55, 0xff, TintType::NONE); //brown mushroom
    blockColors[blockColorID(40,  0)] = new BlockColor(0x9a171c, 0xff, TintType::NONE); //red mushroom
    blockColors[blockColorID(41,  0)] = new BlockColor(0xfefb5d, 0xff, TintType::NONE); //gold block
    blockColors[blockColorID(42,  0)] = new BlockColor(0xe9e9e9, 0xff, TintType::NONE); //iron block
    blockColors[blockColorID(43,  0)] = new BlockColor(0xa8a8a8, 0xff, TintType::NONE); //double slabs
    blockColors[blockColorID(43,  1)] = new BlockColor(0xe5ddaf, 0xff, TintType::NONE);
    blockColors[blockColorID(43,  2)] = new BlockColor(0x94794a, 0xff, TintType::NONE);
    blockColors[blockColorID(43,  3)] = new BlockColor(0x828282, 0xff, TintType::NONE);
    blockColors[blockColorID(44,  0)] = new BlockColor(0xa8a8a8, 0xff, TintType::NONE); //single slabs
    blockColors[blockColorID(44,  1)] = new BlockColor(0xe5ddaf, 0xff, TintType::NONE);
    blockColors[blockColorID(44,  2)] = new BlockColor(0x94794a, 0xff, TintType::NONE);
    blockColors[blockColorID(44,  3)] = new BlockColor(0x828282, 0xff, TintType::NONE);
    blockColors[blockColorID(45,  0)] = new BlockColor(0xaa543b, 0xff, TintType::NONE); //brick
    blockColors[blockColorID(46,  0)] = new BlockColor(0xdb441a, 0xff, TintType::NONE); //tnt
    blockColors[blockColorID(47,  0)] = new BlockColor(0xb4905a, 0xff, TintType::NONE); //bookshelf
    blockColors[blockColorID(48,  0)] = new BlockColor(0x1f471f, 0xff, TintType::NONE); //mossy cobble
    blockColors[blockColorID(49,  0)] = new BlockColor(0x101018, 0xff, TintType::NONE); //obsidian
    blockColors[blockColorID(50,  0)] = new BlockColor(0xffd800, 0xff, TintType::NONE); //torch
    blockColors[blockColorID(51,  0)] = new BlockColor(0xc05a01, 0xff, TintType::NONE); //fire
    blockColors[blockColorID(52,  0)] = new BlockColor(0x265f87, 0xff, TintType::NONE); //spawner
    blockColors[blockColorID(53,  0)] = new BlockColor(wood    , 0xff, TintType::NONE); //wood steps
    blockColors[blockColorID(54,  0)] = new BlockColor(0x8f691d, 0xff, TintType::NONE); //chest
    blockColors[blockColorID(55,  0)] = new BlockColor(0x480000, 0xff, TintType::NONE); //redstone wire
    blockColors[blockColorID(56,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //diamond ore
    blockColors[blockColorID(57,  0)] = new BlockColor(0x82e4e0, 0xff, TintType::NONE); //diamond block
    blockColors[blockColorID(58,  0)] = new BlockColor(0xa26b3e, 0xff, TintType::NONE); //craft table
    blockColors[blockColorID(59,  0)] = new BlockColor(0x00e210, 0xff, TintType::NONE); //crops
    blockColors[blockColorID(60,  0)] = new BlockColor(0x633f24, 0xff, TintType::NONE); //cropland
    blockColors[blockColorID(61,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //furnace
    blockColors[blockColorID(62,  0)] = new BlockColor(0x808080, 0xff, TintType::NONE); //furnace, powered
    blockColors[blockColorID(63,  0)] = new BlockColor(0xb4905a, 0xff, TintType::NONE); //fence
    blockColors[blockColorID(64,  0)] = new BlockColor(0x7a5b2b, 0xff, TintType::NONE); //door
    blockColors[blockColorID(65,  0)] = new BlockColor(0xac8852, 0xff, TintType::NONE); //ladder
    blockColors[blockColorID(66,  0)] = new BlockColor(0xa4a4a4, 0xff, TintType::NONE); //track
    blockColors[blockColorID(67,  0)] = new BlockColor(0x9e9e9e, 0xff, TintType::NONE); //cobble steps
    blockColors[blockColorID(68,  0)] = new BlockColor(0x9f844d, 0xff, TintType::NONE); //sign
    blockColors[blockColorID(69,  0)] = new BlockColor(0x695433, 0xff, TintType::NONE); //lever
    blockColors[blockColorID(70,  0)] = new BlockColor(0x8f8f8f, 0xff, TintType::NONE); //stone pressureplate
    blockColors[blockColorID(71,  0)] = new BlockColor(0xc1c1c1, 0xff, TintType::NONE); //iron door
    blockColors[blockColorID(72,  0)] = new BlockColor(wood    , 0xff, TintType::NONE); //wood pressureplate
    blockColors[blockColorID(73,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //redstone ore
    blockColors[blockColorID(74,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //glowing redstone ore
    blockColors[blockColorID(75,  0)] = new BlockColor(0x290000, 0xff, TintType::NONE); //redstone torch, off
    blockColors[blockColorID(76,  0)] = new BlockColor(0xfd0000, 0xff, TintType::NONE); //redstone torch, lit
    blockColors[blockColorID(77,  0)] = new BlockColor(0x747474, 0xff, TintType::NONE); //button
    blockColors[blockColorID(78,  0)] = new BlockColor(0xfbffff, 0xff, TintType::NONE); //snow
    blockColors[blockColorID(79,  0)] = new BlockColor(0x8ebfff, 0xff, TintType::NONE); //ice
    blockColors[blockColorID(80,  0)] = new BlockColor(0xffffff, 0xff, TintType::NONE); //snow block
    blockColors[blockColorID(81,  0)] = new BlockColor(0x11801e, 0xff, TintType::NONE); //cactus
    blockColors[blockColorID(82,  0)] = new BlockColor(0xbbbbcc, 0xff, TintType::NONE); //clay
    blockColors[blockColorID(83,  0)] = new BlockColor(0xa1a7b2, 0xff, TintType::NONE); //reeds
    blockColors[blockColorID(84,  0)] = new BlockColor(0xaadb74, 0xff, TintType::NONE); //record player
    blockColors[blockColorID(85,  0)] = new BlockColor(wood    , 0xff, TintType::NONE); //fence
    blockColors[blockColorID(86,  0)] = new BlockColor(0xa25b0b, 0xff, TintType::NONE); //pumpkin
    blockColors[blockColorID(87,  0)] = new BlockColor(0x582218, 0xff, TintType::NONE); //netherrack
    blockColors[blockColorID(88,  0)] = new BlockColor(0x996731, 0xff, TintType::NONE); //slow sand
    blockColors[blockColorID(89,  0)] = new BlockColor(0xcda838, 0xff, TintType::NONE); //glowstone
    blockColors[blockColorID(90,  0)] = new BlockColor(0x732486, 0xff, TintType::NONE); //portal
    blockColors[blockColorID(91,  0)] = new BlockColor(0xa25b0b, 0xff, TintType::NONE); //jackolantern
    // new colors
    blockColors[blockColorID(133,  0)] = new BlockColor(0x00ff30, 0xff, TintType::NONE); //emerald block
    blockColors[blockColorID(152,  0)] = new BlockColor(0x480000, 0xff, TintType::NONE); //redstone block
    blockColors[blockColorID(161,  0)] = new BlockColor(0x164d0c, 0xff, TintType::NONE); //savanna leaf
    blockColors[blockColorID(159,  0)] = new BlockColor(0xe1e1e1, 0xff, TintType::NONE); //stained clay
    blockColors[blockColorID(159,  1)] = new BlockColor(0xeb8138, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  2)] = new BlockColor(0xc04cca, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  3)] = new BlockColor(0x698cd5, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  4)] = new BlockColor(0xc5b81d, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  5)] = new BlockColor(0x3cbf30, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  6)] = new BlockColor(0xda859c, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  7)] = new BlockColor(0x434343, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  8)] = new BlockColor(0x9fa7a7, 0xff, TintType::NONE);
    blockColors[blockColorID(159,  9)] = new BlockColor(0x277697, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 10)] = new BlockColor(0x7f33c1, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 11)] = new BlockColor(0x26339b, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 12)] = new BlockColor(0x57331c, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 13)] = new BlockColor(0x384e18, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 14)] = new BlockColor(0xa52d28, 0xff, TintType::NONE);
    blockColors[blockColorID(159, 15)] = new BlockColor(0x1b1717, 0xff, TintType::NONE); //end stained clay
    blockColors[blockColorID(171,  0)] = new BlockColor(0xe1e1e1, 0xff, TintType::NONE); //carpet
    blockColors[blockColorID(171,  1)] = new BlockColor(0xeb8138, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  2)] = new BlockColor(0xc04cca, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  3)] = new BlockColor(0x698cd5, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  4)] = new BlockColor(0xc5b81d, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  5)] = new BlockColor(0x3cbf30, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  6)] = new BlockColor(0xda859c, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  7)] = new BlockColor(0x434343, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  8)] = new BlockColor(0x9fa7a7, 0xff, TintType::NONE);
    blockColors[blockColorID(171,  9)] = new BlockColor(0x277697, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 10)] = new BlockColor(0x7f33c1, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 11)] = new BlockColor(0x26339b, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 12)] = new BlockColor(0x57331c, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 13)] = new BlockColor(0x384e18, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 14)] = new BlockColor(0xa52d28, 0xff, TintType::NONE);
    blockColors[blockColorID(171, 15)] = new BlockColor(0x1b1717, 0xff, TintType::NONE); //end carpet
    blockColors[blockColorID(172,  0)] = new BlockColor(0x916d55, 0xff, TintType::NONE); //hardened clay

    // print map
    printf("Rendering map ...\n");
    cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width*zoom, height*zoom);
    cairo_t * cr = cairo_create(surface);
    for (int z = 0; z < height/16+1; z++) {
        for (int x = 0; x < width/16+1; x++) {
            renderChunk(cr, worldpath, x, z, centerx-width/2, centerz-height/2, zoom);
        }
        printf("Progress: %3i%\n", z*100/(height/16+1));
    }
    printf("Progress: 100%\n");

    // print map info
    if (infoSize) {
        printf("Printing info ...\n");
        std::string str;

        cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, infoSize);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5); // semi-transparent black

        str = "Center: (" + std::to_string(centerx) + ", " + std::to_string(centerz) + ")";
        cairo_move_to(cr, 0.0, infoSize);
        cairo_show_text(cr, str.c_str());

        str = "Size: (" + std::to_string(width) + ", " + std::to_string(height) + ")";
        cairo_move_to(cr, 0.0, infoSize*2);
        cairo_show_text(cr, str.c_str());
    }

    // finish
    printf("Saving map as \"worldmap.png\" ...\n");
    cairo_destroy(cr);
    //std::string worldname = std::string(worldpath).; // TODO get world name
    //worldname = "worldmap_" + worldname + ".png";
    std::string worldname = "worldmap.png";
    cairo_surface_write_to_png(surface, worldname.c_str());
    cairo_surface_destroy(surface);

    printf("Done.\n");

    return 0;
}

