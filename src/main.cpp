#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "colors.h"
#include "config.h"
#include "fire_animation.h"

#define ONBOARD_LED D0            // Led in NodeMCU at pin GPIO16 (D0).
#define NEOPIXEL_PIN 4
#define BRIGHTNESS  20

#define SLIDESHOW_MODE 0
#define FIRE_MODE 1
#define SNAKE_MODE 1

#define TRANSITION_MOVE 0
#define TRANSITION_REVEAL 1
#define TRANSITION_WIPE 2

int current_x = 0;
int current_y = 0;

const char* ssid = "Cortesi Wifi";
const char* password = "chrysler";

//Your Domain name with URL path or IP address with path
HTTPClient http;
WiFiClient client;
const String serverName = "http://tinyiot.digitalsoup.org";
String config_path = "PixelArt/config.json";

unsigned long pixelLastTime = 0;
unsigned long httpFetchLastTime = 0;
unsigned long drawImgLastTime = 0;
unsigned long ledBlinkLastTime = 0;
unsigned long pixelTimer = 50;
unsigned long httpFetchTimer = 10000;
unsigned long ledBlinkTimer = 1000;
unsigned long slideshow_timer = 5000;
int mode = SLIDESHOW_MODE;
// int mode = FIRE_MODE;
bool ledIsOn = true;

uint8_t slideshow_images[NEOPIXEL_MAX_IMAGES][NEOPIXEL_HEIGHT][NEOPIXEL_WIDTH] = {0}; 
String slideshow_sha[NEOPIXEL_MAX_IMAGES] = {""};
int slideshow_transitions[NEOPIXEL_MAX_IMAGES] = {TRANSITION_MOVE};

unsigned int slideshow_num_images = 0;
unsigned int neopixel_current_image = 0;

CRGB leds[NEOPIXEL_WIDTH*NEOPIXEL_HEIGHT];
StaticJsonDocument<10000> json_config;

void wifiSetup() {
    pinMode(ONBOARD_LED, OUTPUT);    // LED pin as output.
    Serial.println("Wifi Setup!");
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void pixelsSetup() {
    FastLED.addLeds<WS2811, NEOPIXEL_PIN, GRB>(leds, NEOPIXEL_WIDTH*NEOPIXEL_HEIGHT).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(BRIGHTNESS);
    leds[0] = CRGB::White;
    leds[1] = CRGB::White;
    FastLED.show();
}

void setup() {    
    Serial.begin(115200); 
    Serial.println("Starting!");
    wifiSetup();
    pixelsSetup();
}

void print_pixels() {
    for (int row_num=0;row_num<NEOPIXEL_HEIGHT;row_num++) {
        for (int col_num=0;col_num<NEOPIXEL_WIDTH;col_num++) {
            Serial.print(slideshow_images[neopixel_current_image][row_num][col_num]); Serial.print(",");
        }
        Serial.println();
    }
}

int calculate_position(int column, int row) {
    int flipped_row = (row + 1 - NEOPIXEL_HEIGHT)*-1;
    if (row % 2) {
        return (flipped_row*NEOPIXEL_WIDTH) + ((column +1 - NEOPIXEL_WIDTH)*-1);
    } else {
        return (flipped_row*NEOPIXEL_WIDTH) + column;
    }
}

void play_slideshow_img(int imgid) {
    Serial.println("Drawing Image "+String(imgid));
    FastLED.clear();
    for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
        for (int column=0;column<NEOPIXEL_WIDTH;column++) {
            if (slideshow_images[imgid][row][column]>0) {
                leds[calculate_position(column, row)] = get_color(slideshow_images[imgid][row][column]);
            }
        }
    }
    FastLED.show();
}

void transition_slideshow(int original_image, int new_image, int transition) {
    int nextval = 0;
    for (int offset=0;offset<NEOPIXEL_WIDTH;offset++) {
        FastLED.clear();
        // Draw Original Image
        if (transition == TRANSITION_WIPE) {
            for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
                for (int column=offset;column<NEOPIXEL_WIDTH;column++) {
                    if (slideshow_images[original_image][row][column]>0) {
                        leds[calculate_position(column, row)] = get_color(slideshow_images[original_image][row][column]);
                    }
                }
            }
        } else if (transition == TRANSITION_REVEAL || transition == TRANSITION_MOVE) {
            for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
                for (int column=0;column<NEOPIXEL_WIDTH-offset;column++) {
                    if (slideshow_images[original_image][row][column]>0) {
                        leds[calculate_position(column+offset, row)] = get_color(slideshow_images[original_image][row][column]);
                    }
                }
            }
        }
        // Draw New Image
        for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
            for (int column=0;column<offset;column++) {
                if (transition == TRANSITION_MOVE) {
                    nextval = slideshow_images[new_image][row][NEOPIXEL_WIDTH-offset+column];
                } else if (transition == TRANSITION_REVEAL || transition == TRANSITION_WIPE) {
                    nextval = slideshow_images[new_image][row][column];
                }
                if (nextval>0) {
                    leds[calculate_position(column,row)] = get_color(nextval);
                }
            }
        }
        FastLED.show();
        delay(50);
    }
}

void play_fire() {
    draw_fireplace();
    FastLED.clear();
    for (int row=0;row<NEOPIXEL_HEIGHT;row++) {
        for (int column=0;column<NEOPIXEL_WIDTH;column++) {
            Serial.print(get_fireplace_pixel(row,column));
            leds[calculate_position(column, row)] = get_fireplace_pixel(row,column);
        }
    }
    FastLED.show();
}

void play_snake() {
    FastLED.clear();
    leds[current_x][current_y] = get_color(0);
    current_x++;
    if (current_x >= NEOPIXEL_WIDTH) {
        current_x = 0;
        current_y++;
    }
    if (current_y >= NEOPIXEL_HEIGHT) {
        current_y = 0;
    }
    leds[current_x][current_y] = get_color(12);
    FastLED.show();
}

void custom_json_parser(String &payload, int img_index) {
    bool outer_array_found = false;
    bool ignore_pixels = false;
    int row_num = -1;
    int col_num = 0;
    uint8_t current_number = 0;
    for (unsigned int strpos = 0; strpos < payload.length(); strpos++) {
        if (payload[strpos] == '[') {
            if (!outer_array_found) {
                outer_array_found = true;
            } else {
                row_num++;
                col_num = 0;
                ignore_pixels = false;
                if (row_num >= NEOPIXEL_HEIGHT) {
                    ignore_pixels = true;
                }
            }
        } else if (isdigit(payload[strpos]) && !ignore_pixels) {
            current_number *= 10;
            current_number += (int)(payload[strpos])-48; // Subtract 48 (ASCII for 0)
        } else if (payload[strpos] == ',' || payload[strpos] == ']') {
            slideshow_images[img_index][row_num][col_num] = current_number;
            col_num++;
            current_number = 0;
            if (col_num >= NEOPIXEL_WIDTH) {
                ignore_pixels = true;
            }
        }
    }
}

void fetch_img(String &filename, int img_index) {
    Serial.println("Trying to fetch "+filename);
    if(WiFi.status()== WL_CONNECTED) {
        http.begin(client, serverName+'/'+filename+"?cb="+millis());
        int httpResponseCode = http.GET();
        if (httpResponseCode>0) {
            String payload = http.getString();
            custom_json_parser(payload, img_index);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    } 
}

void parse_config(String &payload) {
    String transition;
    DeserializationError error = deserializeJson(json_config, payload);
    if (error) {
        Serial.println(F("Failed to read file, using default configuration"));
    }
    Serial.println("Setting Mode to: "+json_config["mode"].as<String>());
    if (json_config["mode"] == "slideshow") {
        mode = SLIDESHOW_MODE;
    } else if (json_config["mode"] == "fire") {
        mode = FIRE_MODE;
    } else if (json_config["mode"] == "snake") {
        mode = SNAKE_MODE;
    }
    int files_iterator = 0;
    String current_file = "";
    while(json_config["files"][files_iterator]) {
        transition = json_config["files"][files_iterator]["transition"].as<String>();
        if (transition == "move") {
            slideshow_transitions[files_iterator] = TRANSITION_MOVE;
        } else if (transition == "reveal") {
            slideshow_transitions[files_iterator] = TRANSITION_REVEAL;
        } else if (transition == "wipe") {
            slideshow_transitions[files_iterator] = TRANSITION_WIPE;
        }
        if (json_config["files"][files_iterator]["sha"] != slideshow_sha[files_iterator]) {
            current_file = json_config["files"][files_iterator]["path"].as<String>();
            fetch_img(current_file,files_iterator);
            slideshow_sha[files_iterator] = json_config["files"][files_iterator]["sha"].as<String>();
        } else {
            Serial.println("Sha hash is unchanged!");
        }
        files_iterator++;
    }
    slideshow_num_images = files_iterator;
}

void fetch_config() {
    Serial.println("Trying to fetch Config!");
    if(WiFi.status()== WL_CONNECTED) {
        http.begin(client, serverName+'/'+config_path+"?cb="+millis());
        int httpResponseCode = http.GET();
        if (httpResponseCode>0) {
            String payload = http.getString();
            parse_config(payload);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    } 
}

void blink_led() {
    ledIsOn = !ledIsOn;
    digitalWrite(ONBOARD_LED, ledIsOn);
}

void loop() {
    unsigned long pixelElapsedTime = millis() - pixelLastTime;
    unsigned long httpFetchElapsedTime = millis() - httpFetchLastTime;
    unsigned long drawImgElapsedTime = millis() - drawImgLastTime;
    unsigned long ledBlinkElapsedTime = millis() - ledBlinkLastTime;

    if (mode == FIRE_MODE && pixelElapsedTime > pixelTimer) {
        play_fire();
        pixelLastTime = millis();
    }
    if (mode == SLIDESHOW_MODE && drawImgElapsedTime > slideshow_timer) {
        unsigned int previous_image = (neopixel_current_image>0)?neopixel_current_image-1:slideshow_num_images-1;
        transition_slideshow(previous_image,neopixel_current_image,slideshow_transitions[previous_image]);
        play_slideshow_img(neopixel_current_image);
        neopixel_current_image++;
        if (neopixel_current_image >= slideshow_num_images) {
            neopixel_current_image = 0;
        }
        drawImgLastTime = millis();
    }  
    if (httpFetchElapsedTime > httpFetchTimer) {
        fetch_config();
        httpFetchLastTime = millis();
    }  
    if (ledBlinkElapsedTime > ledBlinkTimer) {
        blink_led();
        ledBlinkLastTime = millis();
    }  

      
}