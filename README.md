# GFX_Lite
A slimmed-down version of AdaFruit_GFX and FastLED combined together to make a simple to use graphics library that supports 24bpp colour.

The root/core graphics primitive functions extracted from the Adafruit GFX Library. Originally created by [Jean-Marc Zingg] (https://github.com/ZinggJM/GFX_Root).

![Overview](image.jpg)

## Use Cases
This library is designed to be a drop-in replacement for AdaFruit_GFX and FastLED when using [the ESP32-HUB75-MatrixPanel-DMA library](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) 

Simply compile the ESP32-HUB75-MatrixPanel-DMA library with the compile-time flag `USE_GFX_LITE` to enable this.

## Benefits over using FastLED + AdaFruit GFX

* FastLED and AdaFruit GFX include a lot of code for specific hardware platforms given their use as libraries to control displays or long strings of LEDs. But as a result, there’s a lot of ‘bloat' and compile time warnings. In certain cases these libraries fail to compile at all (i.e. FastLED on the ESP32S3).

* This merged library takes all the drawing functions and strips out the hardware specific junk, so you are free to implement this library on whatever device you want.

* This merged library includes all the draw routines from AdaFruit GFX and additionally enables the use of these with FastLED's CRGB (24bpp) colour struct.

* The overall code size is a lot smaller.

## New Feature - Layers and LayerCompositor!
The library includes the ability to write to off-screen memory based CRGB buffers. You create the layers, and pass a callback function which is of the format `(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b)` to the layer. In this example it's called 'mbi_set_pixel.

Then when you've applied all the relevant FastLED colour functions over the layer, or portions of the layer, you call `gfx_layer.display()` to write the pixels to the output hardware via the callback.

In addition there's a 'LayerCompositor' which is essentially a class that takes two layers and composites them together and sends the output direct to the hardware via the callback.

```
// Setup Global GFX_Layer object
GFX_Layer gfx_layer_bg(PANEL_PHY_RES_X, PANEL_PHY_RES_Y, mbi_set_pixel); // background
GFX_Layer gfx_layer_fg(PANEL_PHY_RES_X, PANEL_PHY_RES_Y, mbi_set_pixel); // foreground

// Create the compositor class
GFX_LayerCompositor gfx_compositor(mbi_set_pixel);


// Draw stuff to the 'foreground layer'
gfx_layer_fg.clear();            
gfx_layer_fg.drawCentreText("COOOL!", MIDDLE, &FreeSansBold9pt7b, CRGB(254,254,254));
gfx_layer_fg.autoCenterX(); // because I don't trust AdaFruit to perfectly place the contents in the middle

// Draw a red background to the background layer
gfx_layer_bg.fillScreen(CRGB(255,0,0));

// Blend them together 50%. This will also send to the output via the callback, immediately
gfx_compositor.Blend(gfx_layer_bg, gfx_layer_fg, 127); // this will send to panel output as well.
 
```
A layer will use 3 bytes of memory for every pixel, so don't get carried away!



