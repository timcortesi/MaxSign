#include "fire_animation.h"
#include "config.h"
#include <Arduino.h>
#include <FastLED.h>

#define FIRE_COLOR_CHOICES 5

unsigned int fire_img[NEOPIXEL_HEIGHT][NEOPIXEL_WIDTH] = {0}; 
uint8_t choice_weight[FIRE_COLOR_CHOICES][NEOPIXEL_HEIGHT] = {
    {4,4,4,4,3,3,3,3,2,2,1,1,0,0,0,0,0,0}, // red
    // {1,1,2,2,3,4,4,3,3,3,2,1,1,0,0,0,0,0}, // redorange
    {0,1,1,1,2,2,3,3,4,4,3,3,2,1,0,0,0,0}, // orange
    {0,0,0,1,2,2,2,3,3,4,4,4,2,1,1,0,0,0}, // yellow
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1}, // white
    {0,0,0,0,1,1,1,2,2,2,2,3,3,6,7,20,20,20}, // black
};

int32 calculate_weights(int row, int previous_color) {
    int num_choices = FIRE_COLOR_CHOICES;
    int sum_of_weight = 0;
    int i;
    for(i=0; i<num_choices; i++) {
       sum_of_weight += choice_weight[i][row];
    }
    int rnd = random(0,sum_of_weight);
    for(i=0; i<num_choices; i++) {
        if(rnd < choice_weight[i][row]) {
            return i;
        } else {
            rnd -= choice_weight[i][row];
        }
    }
    return 0;
}

void translate_colors() {
    unsigned int lookup_table[] = {CRGB::DarkRed,CRGB::OrangeRed,CRGB::Orange,CRGB::White,CRGB::Black};
    for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
        for (int col=0;col<NEOPIXEL_WIDTH;col++) {
            fire_img[row][col] = lookup_table[fire_img[row][col]];
        }
    }
}

void draw_fireplace() {
    int random_color;
    for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
        for (int col=0;col<NEOPIXEL_WIDTH;col++) {
            if (row == 0) {
                random_color = calculate_weights(row,0);
            } else {
                random_color = calculate_weights(row,fire_img[-1*(row-NEOPIXEL_HEIGHT)][col]);
            }
            fire_img[-1*(row+1-NEOPIXEL_HEIGHT)][col] = random_color;
        }
    }
    translate_colors();
}

unsigned int get_fireplace_pixel(int row, int column) {
    return fire_img[row][column];
}