#include "colors.h"
#include <stdint.h>

const uint8_t colortable[16][3] = {
    {0,0,0},
    {255,0,0},
    {211,45,0},
    {192,64,0},
    {128,128,0},     
    // {255,200,0},
    {87,168,0},
    {0,255,0},
    {0,128,128},
    {0,64,192},
    {0,0,255},
    {64,0,192},
    {128,0,128},
    {64,64,64}
};

unsigned int get_color(int 	color_index) {
    unsigned int color = colortable[color_index][0] << 16 | colortable[color_index][1] << 8 | colortable[color_index][2];
    return color;
}