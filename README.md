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

