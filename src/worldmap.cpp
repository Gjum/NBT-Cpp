/* worldmap.cpp
 *
 * Takes a minecraft world path and renders the map into "worldmap.png".
 * Rendered are width by height blocks around the center, each block zoom by zoom pixels large.
 * Optionally prints center x, center z, width, and height of the map.
 * The current color data is from the default texture pack, slightly adjusted by me.
 * The renderer even calculates block transparency and does a bit of height mapping.
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // BlockColor
#include <cairo/cairo.h>
#include "nbt/Tag.h"

const unsigned char heightMappingDarknessPercent = 95;

typedef int32_t BlockColor;

BlockColor blockColors[4096]; // 2^(8+4), id has 8 bit, meta has 4 bit

int blockColorID(int id, int meta) {
    return id | (meta << 8);
}

void SetColor(unsigned char id, unsigned char meta, BlockColor value) {
    blockColors[blockColorID(id, meta)] = value;
}

void buildColorTable() {
    for (unsigned int i = 0; i < 4096; i++) {
        blockColors[i] = 0;
    }
    // too many colors, I put them in an extra file
    // they are included at compile time
#include "MapColors.txt"
}

BlockColor blockColorOf(int id, int meta) {
    return blockColors[blockColorID(id, meta)];
}

void drawChunkOnMap(cairo_surface_t * surface, BlockColor chunkColors[], int x, int z, int zoom) {
    cairo_surface_flush(surface);
    BlockColor * imgdata = (BlockColor *) cairo_image_surface_get_data(surface);
    int imgwidth  = cairo_image_surface_get_width(surface);
    int imgheight = cairo_image_surface_get_height(surface);
    for (int i = 0; i < 16*16; i++) { // all blocks in chunk
        if (chunkColors[i] == 0) {
            //printf("Rendering: Tried to render air block, chunk %i,%i block %i, color %x\n", x, z, i, chunkColors[i]);
            continue; // air block
        }
        int imgx = (i%16)*zoom + x;
        int imgy = (i/16)*zoom + z;
        if (imgx < 0 || imgy < 0 || imgx >= imgwidth || imgy >= imgheight) {
            continue; // outside the image, happens because we render chunks completely even if only partly on the image
        }
        int imgIndex = imgx + imgy*imgwidth;
        for (int j = 0; j < zoom*zoom; j++) // rectangle for one block
            imgdata[imgIndex + j%zoom+(j/zoom)*imgwidth] = chunkColors[i];
    }
    cairo_surface_mark_dirty_rectangle(surface, x, z, 16*zoom, 16*zoom);
}

void getColorsFromChunk(NBT::Tag * level, BlockColor chunkColors[]) {
    for (int i = 0; i < 16*16; i++) chunkColors[i] = 0; // clear all colors
    unsigned int colorsFound = 0; // for quick stopping
    // search all sections, begin at the top (assuming they are sorted)
    // loop breaks when all 16*16 visible blocks have been found
    for (int sectionID = 15; sectionID >= 0; sectionID--) {
        //printf("Rendering: section %i\n", sectionID);
        NBT::Tag * section = level->getSubTag("Sections")->getListItemAsTag(sectionID);
        if (section == NULL) continue; // skip empty sections
        NBT::Tag * ids = section->getSubTag("Blocks");
        NBT::Tag * metas = section->getSubTag("Data");
        // search all blocks in section, begin at the top
        for (int b = 16*16*16-1; b >= 0; b--) {
            BlockColor oldColor = chunkColors[b%(16*16)];
            if (oldColor >= 0xff000000) continue; // skip, we are already opaque
            unsigned char id = ids->getListItemAsInt(b);
            if (id == 0) continue; // quick jump for air
            unsigned char meta = (metas->getListItemAsInt(b/2) >> (b%2)*4) & 0x0F;
            BlockColor newColor = blockColorOf(id, meta);
            // error handling
            if (newColor == 0) {
                // could not find the color, although there is a block here
                // maybe only metadata is unknown? try meta=0
                newColor = blockColorOf(id, 0);
                if (newColor == 0) {
                    // unknown block id and meta? get block below
                    //printf("colorAt: Didn't get color: block %3i:%i\n", id, meta);
                    continue;
                }
            }
            if (oldColor == 0) {
                // first time coloring
                // heightmap visualization
                if ((b/256)%2 == 0) {
                    // darker color
                    unsigned char * colorArray = (unsigned char *) &newColor;
                    colorArray[0] = (int)colorArray[0] * heightMappingDarknessPercent / 100;
                    colorArray[1] = (int)colorArray[1] * heightMappingDarknessPercent / 100;
                    colorArray[2] = (int)colorArray[2] * heightMappingDarknessPercent / 100;
                }
            }
            else {
                // we do not have full opacity yet
                // combine block color and current color (transparent)
                unsigned char * oldArray = (unsigned char *) &oldColor;
                unsigned char * newArray = (unsigned char *) &newColor;
                int c = (int(newArray[3]) * int(0xff - oldArray[3]) + oldArray[3] * 0xff) / 0xff;
                if (c > 0xff) c = 0xff;
                newArray[3] = c;
                for (int i = 0; i < 3; i++) {
                    int c = (int(newArray[i]) * int(0xff - oldArray[3]) + oldArray[i] * oldArray[3]) / 0xff;
                    if (c > 0xff) c = 0xff;
                    newArray[i] = c;
                }
            }
            // are we done already?
            if (newColor >= 0xff000000) {
                colorsFound++;
            }
            // save color and go to next block
            chunkColors[b%(16*16)] = newColor;
            if (colorsFound >= 16*16) break;
        }
        delete section; // because we allocate memory when extracting list items as tags
        if (colorsFound >= 16*16) break;
    }
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
    printf("Arguments: worldpath=%s centerx=%i centerz=%i width=%i height=%i zoom=%i infoSize=%i\n", worldpath, centerx, centerz, width, height, zoom, infoSize);

    printf("Building color table ...\n");
    buildColorTable();

    // render map
    printf("Rendering map ...\n");
    cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width*zoom, height*zoom);
    cairo_t * cr = cairo_create(surface);
    int left = centerx-width/2;
    int top  = centerz-height/2;

    unsigned int progress = 0;
    omp_lock_t lck;
    omp_init_lock(&lck);
#pragma omp parallel for shared(cr, progress)
    for (int chunkz = top >> 4; chunkz <= (top+height) >> 4; chunkz++) {
        for (int chunkx = left >> 4; chunkx <= (left+width) >> 4; chunkx++) {
            //printf("Rendering: chunk %i,%i\n", chunkx, chunkz);
            NBT::Tag * chunk = (new NBT::Tag)->loadFromChunk(worldpath, chunkx, chunkz);
            if (chunk == NULL) continue; // could not reserve memory or no chunk present
            NBT::Tag * level = chunk->getSubTag("Level");
            if (level == NULL) { // no chunk
                delete chunk;
                continue;
            }
            BlockColor chunkColors[16*16];
            getColorsFromChunk(level, chunkColors);
            omp_set_lock(&lck);
            drawChunkOnMap(surface, chunkColors, (chunkx*16-left)*zoom, (chunkz*16-top)*zoom, zoom);
            omp_unset_lock(&lck);
            delete chunk;
        }
        omp_set_lock(&lck);
        unsigned int progressOldPercent = 100*progress/(height/16 + 1);
        progress++;
        unsigned int progressPercent    = 100*progress/(height/16 + 1);
        if (progressPercent > progressOldPercent)
            printf("Progress: %i%%\n", progressPercent);
        omp_unset_lock(&lck);
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

