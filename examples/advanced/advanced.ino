/**
 * @file GFX_Lite_Advanced_Example.ino
 * @brief Advanced example demonstrating GFX_Lite improvements and new features
 */

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <GFX_Layer.hpp>

#define PANEL_RES_X 64      
#define PANEL_RES_Y 32     
#define PANEL_CHAIN 1      
 
MatrixPanel_I2S_DMA *dma_display = nullptr;

// Layers own only their framebuffer; presentation is performed explicitly[cite: 1].
GFX_Layer background_layer(PANEL_RES_X, PANEL_RES_Y);
GFX_Layer foreground_layer(PANEL_RES_X, PANEL_RES_Y);
GFX_Layer effect_layer(PANEL_RES_X, PANEL_RES_Y);
GFX_Layer output_layer(PANEL_RES_X, PANEL_RES_Y);
GFX_LayerCompositor compositor;

uint32_t frame_count = 0;
unsigned long last_fps_time = 0;
unsigned long last_effect_time = 0;

// Forward declarations
void createGradientBackground();
void animatePixelEffect();
void animateScrollingText();
void performanceOptimizedLoop();
void printPerformanceStats();

void setup() {
    Serial.begin(115200);
    Serial.println("GFX_Lite Advanced Example Starting...");

    // Display setup
    HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;
    mxconfig.latch_blanking = 4;

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(255);
    dma_display->clearScreen();

    // Verify layer initialization
    if (!background_layer.isInitialized() || !foreground_layer.isInitialized() ||
        !effect_layer.isInitialized() || !output_layer.isInitialized()) {
        Serial.println("ERROR: One or more layers failed to initialize!");
        return;
    }

    foreground_layer.setColorKey(CRGB::Black);
    foreground_layer.clear(CRGB::Black);
    effect_layer.setAlpha(200);

    // Print memory usage information[cite: 1]
    Serial.printf("Background layer memory: %d bytes\n", background_layer.getMemoryUsage());
    Serial.printf("Foreground layer memory: %d bytes\n", foreground_layer.getMemoryUsage());
    Serial.printf("Effect layer memory: %d bytes\n", effect_layer.getMemoryUsage());
    
    Serial.println("All layers initialized successfully!");
    
    // Initialize background with gradient[cite: 1]
    createGradientBackground();

    // Print it for demo purposes
    background_layer.present([](int16_t x, int16_t y, CRGB color) {
        dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    });

    delay(1000);
    
}

void createGradientBackground() {
    // Create a rainbow gradient background using optimized functions[cite: 1]
    for (int y = 0; y < PANEL_RES_Y; y++) {
        for (int x = 0; x < PANEL_RES_X; x++) {
            uint8_t hue = (x * 255) / PANEL_RES_X;
            uint8_t sat = 255;
            uint8_t val = (y * 255) / PANEL_RES_Y;
            
            CHSV hsv_color(hue, sat, val);
            CRGB rgb_color = hsv_color;
            
            background_layer.setPixel(x, y, rgb_color);
        }
    }
}

void animatePixelEffect() {
    // Apply blur to all existing pixels so they slowly fade away each frame[cite: 1, 2, 3]
    effect_layer.blur(32);
    
    // Pick a random X and Y coordinate on the panel
    int x = random16(PANEL_RES_X);
    int y = random16(PANEL_RES_Y);
    
    // Generate a random vibrant color using HSV
    CRGB random_color = CHSV(random8(), 255, 255);

    // Light up the chosen random pixel[cite: 1, 2, 3]
    effect_layer.setPixel(x, y, random_color);
}

void animateScrollingText() {
    // Clear foreground layer completely to prevent ghosting/artifacts
    foreground_layer.clear(CRGB::Black);
    
    // Draw scrolling text single-line safely
    static int text_x = PANEL_RES_X;
    const char* message = "GFX_Lite Improved!";
    
    foreground_layer.setTextWrap(false); // CRITICAL: Prevents breaking onto a second line
    foreground_layer.setTextSize(1);
    foreground_layer.setTextColor(CRGB(255, 255, 255));

    // Center vertically on the 32px high panel
    foreground_layer.setCursor(text_x, (PANEL_RES_Y - 8) / 2);
    foreground_layer.print(message);
    
    // Scroll text smoothly to the left
    text_x -= 1;

    // Reset when text has completely scrolled past the left edge
    // (Approximate character length * font width 6px)
    if (text_x < -110) {
        text_x = PANEL_RES_X;
    }
}

void performanceOptimizedLoop() {
    // Use a slow FastLED sine beat for a smooth fade up and down (one cycle per 10 seconds).
    effect_layer.setAlpha(beatsin8(6, 0, 255));

    // Update effect animations at controlled rate
    if (millis() - last_effect_time > 40) {
        animatePixelEffect();
        last_effect_time = millis();
    }
    
    // Update text every frame for smooth scrolling
    animateScrollingText();
    
    // Composite background and effect layers
    compositor.composite(background_layer, effect_layer, output_layer);
    
    // Draw text over the output in place using the foreground layer's opacity.
    compositor.composite(output_layer, foreground_layer, output_layer);

    // Present final pixel buffer to physical HUB75 DMA display matrix[cite: 1, 3]
    output_layer.present([](int16_t x, int16_t y, CRGB color) {
        dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
    });
    
    frame_count++;
}

void printPerformanceStats() {
    if (millis() - last_fps_time >= 5000) {  // Every 5 seconds[cite: 1]
        float fps = frame_count / 5.0f;
        Serial.printf("Performance: %.1f FPS, Average color: ", fps);
        
        CRGB avg_bg = background_layer.getAverageColor();
        Serial.printf("BG(R:%d,G:%d,B:%d) ", avg_bg.r, avg_bg.g, avg_bg.b);
        
        CRGB avg_fg = foreground_layer.getAverageColor();
        Serial.printf("FG(R:%d,G:%d,B:%d)\n", avg_fg.r, avg_fg.g, avg_fg.b);
        
        frame_count = 0;
        last_fps_time = millis();
    }
}

void loop() {
    performanceOptimizedLoop();
    printPerformanceStats();
}
