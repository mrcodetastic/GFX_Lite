/**
 * Experimental layer class to do play with pixel in an off-screen buffer before painting to the display 
 *
 * Requires GFX_Lite
 *
 * Codetastic 2020
 **/

#ifndef DISPLAY_MATRIX_LAYER
#define DISPLAY_MATRIX_LAYER

#include <functional>
#include "GFX_Lite.h"

#define BLACK_BACKGROUND_PIXEL_COLOUR CRGB(0,0,0)

enum textPosition { TOP, MIDDLE, BOTTOM };

/* To help with direct pixel referencing by width and height */
struct layerPixels {
    CRGB **data;
    uint16_t width;
    uint16_t height;
};
class GFX_Layer : public GFX
{
    public:
        GFX_Layer(uint16_t width, uint16_t height, 
                  std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> cb) 
            : GFX(width, height), _width(width), _height(height), callback(cb) {
            init();
        }

        inline void init()
        {
            pixels = new layerPixels();
            pixels->width = _width;
            pixels->height = _height;
            pixels->data = new CRGB*[_height];
            for (int i = 0; i < _height; i++) {
                pixels->data[i] = new CRGB[_width];
            }
            Serial.printf("Allocated memory for layerPixels: %d x %d\r\n", _width, _height);
        }

        void drawPixel(int16_t x, int16_t y, CRGB color) {				// overwrite GFX_Lite implementation	

            if( x >= _width 	|| x < 0) return; // 0;
            if( y >= _height 	|| y < 0) return; // 0;
            
            pixels->data[y][x] = color;
        }


        void setPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
            drawPixel(x,y, CRGB(r,g,b));
        }

        void drawPixel(int16_t x, int16_t y, uint16_t color) {;   		// overwrite GFX_Lite implementation

            // 565 color conversion
            uint8_t r = ((((color >> 11)  & 0x1F) * 527) + 23) >> 6;
            uint8_t g = ((((color >> 5)   & 0x3F) * 259) + 33) >> 6;
            uint8_t b = (((color & 0x1F)  * 527) + 23) >> 6;

            setPixel(x,y,r,g,b);

        }


        // Font Stuff
        //https://forum.arduino.cc/index.php?topic=642749.0
        void drawCentreText(const char *buf, textPosition textPos = BOTTOM, const GFXfont *f = NULL, CRGB color = 0x8410, int yadjust = 0); // 128,128,128 RGB @ bottom row by default
    
        void dim(byte value);
        void clear();
        inline void display(bool skip_transparent = false) {   //	flush to display / LED matrix via callbacks, skip transparent for performance reasons

            for (int y = 0; y < _height; y++) {
                for (int x = 0; x < _width; x++) {
                    if (skip_transparent && pixels->data[y][x] == transparency_colour) continue;
                    callback(x,y, pixels->data[y][x].r, pixels->data[y][x].g, pixels->data[y][x].b); // send values to callback
            }}
        }

        // override the color of all pixels that aren't the transparent color
        // void overridePixelColor(int r, int g, int b);

        inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        inline void setTransparency(bool t) { transparency_enabled = t; }

        // Effects
        void moveX(int delta);
        void autoCenterX();		
        void moveY(int delta);

        // For layer composition - accessed publically
        CRGB 		transparency_colour 	= BLACK_BACKGROUND_PIXEL_COLOUR;
        bool		transparency_enabled 	= true;
        layerPixels *pixels;

        ~GFX_Layer(void); 

        // used by the compositor really.
        uint16_t getWidth() { return _width; }
        uint16_t getHeight() { return _height; }


    private:
    
        // Member variable to store the callback
        std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> callback;

        uint16_t _width;
        uint16_t _height;
};




/* Merge FastLED layers into a super layer and display. */
// A class that will take a callback function
class GFX_LayerCompositor {
private:
    std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> callback;

public:

    // New constructor
    GFX_LayerCompositor(const std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> cb) : callback(cb) {}

    /*
    void setCallback(const std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)>& cb) {
        callback = cb;
    }


    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    void executeCallback() {
        if (callback) {
            callback(x, y, r, g, b);
        } else {
            assert(false);
        }
    }
    */

    void Stack(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, bool writeToBgLayer = false);
    void Siloette(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer);
    void Blend(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, uint8_t ratio = 127);    

};


#endif