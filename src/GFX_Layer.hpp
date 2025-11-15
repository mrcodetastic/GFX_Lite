/**
 * Experimental layer class to do play with pixel in an off-screen buffer before painting to the display 
 *
 * Requires GFX_Lite
 *
 * Codetastic 2020

     ██████╗ ██████╗ ██████╗ ███████╗████████╗ █████╗ ███████╗████████╗██╗ ██████╗
    ██╔════╝██╔═══██╗██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██╔════╝╚══██╔══╝██║██╔════╝
    ██║     ██║   ██║██║  ██║█████╗     ██║   ███████║███████╗   ██║   ██║██║     
    ██║     ██║   ██║██║  ██║██╔══╝     ██║   ██╔══██║╚════██║   ██║   ██║██║     
    ╚██████╗╚██████╔╝██████╔╝███████╗   ██║   ██║  ██║███████║   ██║   ██║╚██████╗
     ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝

*/


#ifndef DISPLAY_MATRIX_LAYER
#define DISPLAY_MATRIX_LAYER

#include <functional>
#include <new>
#include "GFX_Lite.h"

#define BLACK_BACKGROUND_PIXEL_COLOUR CRGB(0,0,0)

enum textPosition { TOP, MIDDLE, BOTTOM };

/* To help with direct pixel referencing by width and height */
struct layerPixels {
    CRGB **data;
    CRGB *contiguous_memory;  // Pointer to contiguous memory block
    uint16_t width;
    uint16_t height;
};
class GFX_Layer : public GFX
{
    public:
        GFX_Layer(uint16_t width, uint16_t height, 
                  std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> cb) 
            : GFX(width, height), _width(width), _height(height), callback(cb) {
            
            // Input validation
            if (width == 0 || height == 0) {
                // Handle error - set to minimum viable size
                _width = width == 0 ? 1 : width;
                _height = height == 0 ? 1 : height;
            }
            
            // Check for memory allocation risks
            size_t memory_needed = (size_t)_width * _height * sizeof(CRGB);
            const size_t MAX_LAYER_MEMORY = 1024 * 1024; // 1MB limit
            
            if (memory_needed > MAX_LAYER_MEMORY) {
                // Could implement scaling or throw error
                // For now, proceed but could add warning
            }
            
            if (!init()) {
                // Handle initialization failure
                _width = _height = 0;
            }
        }

        inline bool init()
        {
            // Attempt to allocate layerPixels structure
            pixels = new(std::nothrow) layerPixels();
            if (!pixels) {
                return false;
            }
            
            pixels->width = _width;
            pixels->height = _height;
            
            // Allocate contiguous memory for better cache performance
            CRGB* contiguous_data = new(std::nothrow) CRGB[_width * _height];
            if (!contiguous_data) {
                delete pixels;
                pixels = nullptr;
                return false;
            }
            
            pixels->data = new(std::nothrow) CRGB*[_height];
            if (!pixels->data) {
                delete[] contiguous_data;
                delete pixels;
                pixels = nullptr;
                return false;
            }
            
            for (int i = 0; i < _height; i++) {
                pixels->data[i] = &contiguous_data[i * _width];
            }
            
            // Store the contiguous pointer for cleanup
            pixels->contiguous_memory = contiguous_data;
            
            //Serial.printf("Allocated memory for layerPixels: %d x %d\r\n", _width, _height);
            return true;
        }

        void drawPixel(int16_t x, int16_t y, CRGB color) {				// overwrite GFX_Lite implementation	

            if( x >= _width 	|| x < 0) return;
            if( y >= _height 	|| y < 0) return;
            
            pixels->data[y][x] = color;
        }

        void setPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
            if( x >= _width 	|| x < 0) return;
            if( y >= _height 	|| y < 0) return;
            drawPixel(x,y, CRGB(r,g,b));
        }

        // Fast unsafe pixel access for performance-critical operations
        inline void drawPixelUnsafe(int16_t x, int16_t y, CRGB color) __attribute__((always_inline)) {
            pixels->data[y][x] = color;
        }

        // Get pixel color with bounds checking
        CRGB getPixel(int16_t x, int16_t y) {
            if( x >= _width 	|| x < 0) return CRGB::Black;
            if( y >= _height 	|| y < 0) return CRGB::Black;
            return pixels->data[y][x];
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
        
        // Advanced layer operations
        void copyRect(int16_t src_x, int16_t src_y, int16_t dst_x, int16_t dst_y, 
                      int16_t w, int16_t h);
        void scrollX(int16_t pixels, CRGB fill_color = CRGB::Black);
        void scrollY(int16_t pixels, CRGB fill_color = CRGB::Black);
        void flipHorizontal();
        void flipVertical();
        void rotate90();
        void blur(uint8_t blur_amount = 64);
        
        // Performance optimizations
        void fastFillRect(int16_t x, int16_t y, int16_t w, int16_t h, CRGB color);
        void fastFillScreen(CRGB color);
        
        // Color operations
        void adjustBrightness(uint8_t scale);
        void adjustGamma(float gamma = 2.2f);
        void applyColorMatrix(const float matrix[3][3]);
        
        // Analysis functions
        CRGB getAverageColor() const;
        CRGB getDominantColor() const;
        uint32_t getPixelCount(CRGB target_color) const;

        // For layer composition - accessed publically
        CRGB 		transparency_colour 	= BLACK_BACKGROUND_PIXEL_COLOUR;
        bool		transparency_enabled 	= true;
        layerPixels *pixels;

        ~GFX_Layer(void); 

        // used by the compositor really.
        uint16_t getWidth() { return _width; }
        uint16_t getHeight() { return _height; }
        
        // Utility functions
        bool isValidCoordinate(int16_t x, int16_t y) const {
            return (x >= 0 && x < _width && y >= 0 && y < _height);
        }
        
        size_t getMemoryUsage() const {
            return (size_t)_width * _height * sizeof(CRGB) + sizeof(layerPixels) + _height * sizeof(CRGB*);
        }
        
        bool isInitialized() const {
            return (pixels != nullptr && pixels->data != nullptr && pixels->contiguous_memory != nullptr);
        }
        
        // Debug/diagnostic functions
        void printMemoryInfo() const {
            if (!pixels) return;
            //Serial.printf("Layer: %dx%d, Memory: %d bytes, Initialized: %s\n", 
            //    _width, _height, getMemoryUsage(), isInitialized() ? "Yes" : "No");
        }


    private:
	
        uint16_t _width;
        uint16_t _height;
		
    
        // Member variable to store the callback
        std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> callback;
		
};




/* Merge FastLED layers into a super layer and display. */
// A class that will take a callback function
class GFX_LayerCompositor {
public:
    // Blend modes for advanced compositing
    enum BlendMode {
        BLEND_NORMAL,
        BLEND_MULTIPLY,
        BLEND_SCREEN, 
        BLEND_OVERLAY
    };

private:
    std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> callback;
    
    // Advanced blending function
    CRGB blendPixels(CRGB base, CRGB overlay, BlendMode mode, uint8_t opacity = 255);

public:

    // New constructor
    GFX_LayerCompositor(const std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> cb) : callback(cb) {}

    void Stack(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, bool writeToBgLayer = false);
    void Siloette(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer);
    void Blend(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, uint8_t ratio = 127);
    
    // Advanced compositing methods
    void BlendAdvanced(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, BlendMode mode, uint8_t opacity = 255);
    void Mask(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, GFX_Layer &_maskLayer);
    void AlphaComposite(GFX_Layer &_bgLayer, GFX_Layer &_fgLayer, uint8_t alpha);
    
    // Multi-layer compositing (up to 4 layers)
    void CompositeMultiple(GFX_Layer* layers[], uint8_t count, BlendMode modes[], uint8_t opacities[]);
};


#endif
