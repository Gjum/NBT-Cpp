/* map.cpp
 *
 * Renders a map (item) into a png file (map_#.png).
 * Optionally prints `scale`, `dimension`, `xCenter` and `zCenter` of the map.
 * 
 * Arguments: <worldpath> <mapnr> [zoom=5] [infoSize=0]
 *
 * Example:
 * map saves/Legio-Umbra/ 4 5 12
 * Renders `saves/Legio-Umbra/data/map_4.dat` with `5x5` pixel size and printing various map data in font size `12`.
 *
 * by Gjum <gjum42@gmail.com>
 */

#include <cairo/cairo.h>
#include "NbtTag.h"

int main(int argc, char* argv[]) {
    // read args
    int mapnr = -1;
    unsigned int zoom = 5;
    unsigned int infoSize = 0;
    if (argc <= 2) {
        printf("Usage: %s <worldpath> <mapnr> [zoom=5] [infoSize=0]", argv[0]);
        return 0;
    }
    mapnr = atoi(argv[2]);
    if (argc > 3) zoom = atoi(argv[3]);
    if (argc > 4) infoSize = atoi(argv[4]);
    //printf("Args: worldpath=%s mapnr=%i zoom=%i infoSize=%i\n", argv[1], mapnr, zoom, infoSize);

    // read map
    char buffer[256];
    sprintf(buffer, "%s/data/map_%i.dat", argv[1], mapnr);
    NbtTag rootTag;
    if (!rootTag.loadFromFile(buffer, true))
        return -1;
    
    // read values
    int16_t width = rootTag.getTagAt("data.width")->getInt();
    int16_t height = rootTag.getTagAt("data.height")->getInt();
    int32_t listSize = rootTag.getTagAt("data.colors")->getListSize();
    NbtPayload** colorValues = rootTag.getTagAt("data.colors")->getListValues();

    //printf("width=%i\n", width);
    //printf("height=%i\n", height);
    //printf("listSize=%i\n", listSize);

    if (width*height != listSize) {
        printf("Invaid map file or error while reading\n");
        return -1;
    }

    // build color table
    unsigned char baseColors[] = {
        // original colors
        0, 0, 0,
        127, 178, 56,
        247, 233, 163,
        167, 167, 167,
        255, 0, 0,
        160, 160, 255,
        167, 167, 167,
        0, 124, 0,
        // colors since 1.7
        255, 255, 255,
        164, 168, 184,
        183, 106, 47,
        112, 112, 112,
        64, 64, 255,
        104, 83, 50,
        255, 252, 245,
        216, 127, 51,
        178, 76, 216,
        102, 153, 216,
        229, 229, 51,
        127, 204, 25,
        242, 127, 165,
        76, 76, 76,
        153, 153, 153,
        76, 127, 153,
        127, 63, 178,
        51, 76, 178,
        102, 76, 51,
        102, 127, 51,
        153, 51, 51,
        25, 25, 25,
        250, 238, 77,
        92, 219, 213,
        74, 128, 255,
        0, 217, 58,
        21, 20, 31,
        112, 2, 0
    };
    unsigned char colors[sizeof(baseColors)*4];
    for (int i = 0; i < sizeof(baseColors)/3; i++) { // every [(rgb)(rgb)(rgb)(rgb)] group in colors[]
        for (int j = 0; j < 3; j++) { // r, g, and b
            colors[i*12+0+j] = 180.0*baseColors[i*3+j]/255.0;
            colors[i*12+3+j] = 220.0*baseColors[i*3+j]/225.0;
            colors[i*12+6+j] = 255.0*baseColors[i*3+j]/255.0;
            colors[i*12+9+j] = 135.0*baseColors[i*3+j]/255.0;
        }
    }

    // print map
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width*zoom, height*zoom);
    cairo_t *cr = cairo_create(surface);
    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            char id = colorValues[x+y*width]->tagByte;
            if (id < 4) continue; // transparent
            float r = colors[3*id] / 256.0;
            float g = colors[3*id+1] / 256.0;
            float b = colors[3*id+2] / 256.0;
            cairo_set_source_rgb (cr, r,g,b);
            cairo_rectangle (cr, x*zoom,y*zoom, zoom,zoom);
            cairo_fill (cr);
        }
    }

    // print map info
    if (infoSize) {
        cairo_select_font_face (cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size (cr, infoSize);
        cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);

        sprintf(buffer, "scale=%i", rootTag.getTagAt("data.scale")->getInt());
        cairo_move_to (cr, 0.0, infoSize);
        cairo_show_text (cr, buffer);

        sprintf(buffer, "dimension=%i", rootTag.getTagAt("data.dimension")->getInt());
        cairo_move_to (cr, 0.0, infoSize*2);
        cairo_show_text (cr, buffer);

        sprintf(buffer, "xCenter=%i", rootTag.getTagAt("data.xCenter")->getInt());
        cairo_move_to (cr, 0.0, infoSize*3);
        cairo_show_text (cr, buffer);

        sprintf(buffer, "zCenter=%i", rootTag.getTagAt("data.zCenter")->getInt());
        cairo_move_to (cr, 0.0, infoSize*4);
        cairo_show_text (cr, buffer);
    }

    // finish
    cairo_destroy (cr);
    sprintf(buffer, "map_%i.png", mapnr);
    cairo_surface_write_to_png (surface, buffer);
    cairo_surface_destroy (surface);

    return 0;
}

