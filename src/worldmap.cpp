/* worldmap.cpp
 *
 * Takes a minecraft world path and renders the map into "worldmap.png".
 * Rendered are widthxheight blocks around the center, each block zoomxzoom pixel large.
 * Optionally prints center x, center z, width, and height of the map.
 * The current color data is from the ingame map renderer, slightly adjusted by me.
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
 * The image contains all blocks from 200,-632 to 799,-231.
 *
 * by Gjum <gjum42@gmail.com> <http://gjum.sytes.net/>
 */

#include <omp.h>
#include <cairo/cairo.h>
#include "nbt/Tag.h"

struct BlockColor {
    unsigned char red, green, blue;
};

BlockColor * blockColors[4096]; // 2^(8+4), id has 8 bit, meta has 4 bit
BlockColor * blockColorsDark[4096];

int blockColorID(int id, int meta) {
    return id | (meta << 8);
}

void SetColor(unsigned char id, unsigned char meta, unsigned int value) {
    BlockColor * color = new BlockColor;
    color->red   = (value >> 16) & 0xff;
    color->green = (value >> 8) & 0xff;
    color->blue  = value & 0xff;
    blockColors[blockColorID(id, meta)] = color;

    // darker color
    BlockColor * colorDark = new BlockColor;
    colorDark->red   = int(color->red  ) * 95 / 100;
    colorDark->green = int(color->green) * 95 / 100;
    colorDark->blue  = int(color->blue ) * 95 / 100;
    blockColorsDark[blockColorID(id, meta)] = colorDark;
}

void buildColorTable() {
    for (unsigned int i = 0; i < 4096; i++) {
        blockColors[i] = NULL;
        blockColorsDark[i] = NULL;
    }
    // too many colors, I put them in an extra file
    // they are included at compile time
#include "MapColors.txt"
}

BlockColor * blockColorOf(int id, int meta) {
    return blockColors[blockColorID(id, meta)];
}
BlockColor * darkBlockColorOf(int id, int meta) {
    return blockColorsDark[blockColorID(id, meta)];
}

BlockColor * colorInChunk(char x, char z, NBT::Tag * chunk) {
    if (chunk == NULL) return NULL; // no chunk loaded

    NBT::Tag * level = chunk->getSubTag("Level");
    if (level == NULL) return NULL; // chunk is not generated

    // block position
    int y = level->getSubTag("HeightMap")->getListItemAsInt(x+z*16)-1;
    if (y < 0) return NULL; // no blocks present, render transparent

    int sectionID = y/16;
    int blockIndex = x+z*16+(y%16)*16*16;

    NBT::Tag * section = level->getSubTag("Sections")->getListItemAsTag(sectionID);
    if (section == NULL) return NULL; // section is not generated

    // block id
    int id = (unsigned char) section->getSubTag("Blocks")->getListItemAsInt(blockIndex);
    if (id < 0 || id > 0xff) {
        printf("colorAt: Invalid id: %i\n", id);
        return NULL;
    }

    // block metadata (upper/lower 4 bit of the byte)
    int meta = (section->getSubTag("Data")->getListItemAsInt(blockIndex/2) >> (blockIndex%2)*4) & 0x0F;
    if (meta < 0 || meta > 0xf) {
        printf("colorAt: Invalid meta: %i\n", meta);
        return NULL;
    }

    delete section; // we have to clean up the allocated data to prevent memory leaking

    // finally we get the color of the block
    BlockColor * color = blockColorOf(id, meta);

    // heightmap visualization
    if (y%2 == 0) {
        color = darkBlockColorOf(id, meta);
    }

    // error handling
    if (color == NULL) {
        // could not find the color, although there are blocks
        // unknown metadata? fallback to meta=0
        color = blockColorOf(id, 0);
        if (color == NULL) {
            // unknown block id and meta? render transparent
            printf("colorAt: Didn't get color: block %3i:%i\n", id, meta);
            return NULL;
        }
    }

    return color;
}

void drawOnMap(cairo_t * cr, BlockColor * color, int x, int z, int size) {
    float r = color->red   / 256.0;
    float g = color->green / 256.0;
    float b = color->blue  / 256.0;
    cairo_set_source_rgb(cr, r,g,b);
    cairo_rectangle(cr, x, z, size, size);
    cairo_fill(cr);
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
    printf("Arguments: worldpath=%s centerx=%i centerz=%i width=%i height=%i zoom=%i info=%i\n", worldpath, centerx, centerz, width, height, zoom, infoSize);

    printf("Building color table ...\n");
    buildColorTable();

    // render map
    printf("Rendering map ...\n");
    cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width*zoom, height*zoom);
    cairo_t * cr = cairo_create(surface);
    int left = centerx-width/2;
    int top  = centerz-height/2;

    omp_lock_t lck;
    omp_init_lock(&lck);
#pragma omp parallel for shared(cr)
    for (int chunkz = top >> 4; chunkz <= (top+height) >> 4; chunkz++) {
        for (int chunkx = left >> 4; chunkx <= (left+width) >> 4; chunkx++) {
            //printf("Rendering: %i %i\n", chunkx, chunkz);
            NBT::Tag * chunk = (new NBT::Tag)->loadFromChunk(worldpath, chunkx, chunkz);
            if (chunk == NULL) continue; // could not reserve memory or no chunk present
            if (chunk->getSubTag("Level") == NULL) { // no chunk
                delete chunk;
                continue;
            }
            for (char inChunkz = 0; inChunkz < 16; inChunkz++) {
                for (char inChunkx = 0; inChunkx < 16; inChunkx++) {
                    // get color values
                    BlockColor * color = colorInChunk(inChunkx, inChunkz, chunk);
                    if (color == NULL) continue; // error or transparent (no block)
                    omp_set_lock(&lck);
                    drawOnMap(cr, color, (inChunkx+chunkx*16-left)*zoom, (inChunkz+chunkz*16-top)*zoom, zoom);
                    omp_unset_lock(&lck);
                }
            }
            delete chunk;
        }
    }
    omp_destroy_lock(&lck);

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

