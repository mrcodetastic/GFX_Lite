/**
 * @file GFX_Lite_Advanced_Example.ino
 * @brief Advanced example demonstrating GFX_Lite improvements and new features
 * 
 * This example showcases:
 * - Layer memory optimizations
 * - Advanced blending modes
 * - Performance improvements
 * - Error handling
 * - New utility functions
 */

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64      
#define PANEL_RES_Y 32     
#define PANEL_CHAIN 1      
 
MatrixPanel_I2S_DMA *dma_display = nullptr;

// Layer objects with improved memory management
GFX_Layer background_layer(PANEL_RES_X, PANEL_RES_Y, 
    [](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        dma_display->drawPixelRGB888(x, y, r, g, b);
    });

GFX_Layer foreground_layer(PANEL_RES_X, PANEL_RES_Y, 
    [](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        dma_display->drawPixelRGB888(x, y, r, g, b);
    });

GFX_Layer effect_layer(PANEL_RES_X, PANEL_RES_Y, 
    [](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        dma_display->drawPixelRGB888(x, y, r, g, b);
    });

// Compositor with advanced blending
GFX_LayerCompositor compositor([](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    dma_display->drawPixelRGB888(x, y, r, g, b);
});

uint32_t frame_count = 0;
unsigned long last_fps_time = 0;
unsigned long last_effect_time = 0;

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
    if (!background_layer.isInitialized()) {
        Serial.println("ERROR: Background layer failed to initialize!");
        return;
    }
    if (!foreground_layer.isInitialized()) {
        Serial.println("ERROR: Foreground layer failed to initialize!");
        return;
    }
    if (!effect_layer.isInitialized()) {
        Serial.println("ERROR: Effect layer failed to initialize!");
        return;
    }

    // Print memory usage information
    Serial.printf("Background layer memory: %d bytes\n", background_layer.getMemoryUsage());
    Serial.printf("Foreground layer memory: %d bytes\n", foreground_layer.getMemoryUsage());
    Serial.printf("Effect layer memory: %d bytes\n", effect_layer.getMemoryUsage());
    
    Serial.println("All layers initialized successfully!");
    
    // Initialize background with gradient
    createGradientBackground();
}

void createGradientBackground() {
    // Create a rainbow gradient background using optimized functions
    for (int y = 0; y < PANEL_RES_Y; y++) {
        for (int x = 0; x < PANEL_RES_X; x++) {
            uint8_t hue = (x * 255) / PANEL_RES_X;
            uint8_t sat = 255;
            uint8_t val = (y * 255) / PANEL_RES_Y;
            
            CHSV hsv_color(hue, sat, val);
            CRGB rgb_color = hsv_color; // FastLED automatic conversion
            
            background_layer.drawPixelUnsafe(x, y, rgb_color);
        }
    }
}

void animateFireEffect() {
    // Clear effect layer
    effect_layer.fastFillScreen(CRGB::Black);
    
    // Create animated fire effect at bottom
    uint32_t time_factor = millis() / 20;
    
    for (int x = 0; x < PANEL_RES_X; x++) {
        for (int y = PANEL_RES_Y - 8; y < PANEL_RES_Y; y++) {
            // Use noise for fire effect
            uint8_t heat = inoise8(x * 30, y * 30, time_factor);
            heat = qadd8(heat, (PANEL_RES_Y - y) * 32);
            
            CRGB fire_color = HeatColor(heat);
            effect_layer.drawPixelUnsafe(x, y, fire_color);
        }
    }
    
    // Apply blur for smoother fire effect
    effect_layer.blur(32);
}

void animateScrollingText() {
    // Clear foreground layer
    foreground_layer.fastFillScreen(CRGB::Black);
    
    // Set transparency
    foreground_layer.transparency_colour = CRGB::Black;
    
    // Draw scrolling text
    static int text_x = PANEL_RES_X;
    const char* message = "GFX_Lite Improved!";
    
    foreground_layer.setCursor(text_x, PANEL_RES_Y / 2);
    foreground_layer.setTextColor(CRGB::White);
    foreground_layer.setTextSize(1);
    foreground_layer.print(message);
    
    // Scroll text
    text_x -= 1;
    if (text_x < -200) {  // Reset when text is off screen
        text_x = PANEL_RES_X;
    }
}

void demonstrateBlendModes() {
    static uint8_t blend_mode_index = 0;
    static unsigned long last_blend_change = 0;
    
    // Change blend mode every 3 seconds
    if (millis() - last_blend_change > 3000) {
        blend_mode_index = (blend_mode_index + 1) % 4;
        last_blend_change = millis();
        
        // Print current blend mode
        const char* blend_names[] = {
            "NORMAL", "MULTIPLY", "SCREEN", "OVERLAY"
        };
        Serial.printf("Blend mode: %s\n", blend_names[blend_mode_index]);
    }
    
    GFX_LayerCompositor::BlendMode modes[] = {
        GFX_LayerCompositor::BLEND_NORMAL,
        GFX_LayerCompositor::BLEND_MULTIPLY,
        GFX_LayerCompositor::BLEND_SCREEN,
        GFX_LayerCompositor::BLEND_OVERLAY
    };
    
    // Use advanced blending to composite layers
    compositor.BlendAdvanced(background_layer, effect_layer, modes[blend_mode_index], 200);
}

void performanceOptimizedLoop() {
    // Update animations at different rates for better performance
    if (millis() - last_effect_time > 50) {  // 20 FPS for effects
        animateFireEffect();
        last_effect_time = millis();
    }
    
    // Update text every frame for smooth scrolling
    animateScrollingText();
    
    // Composite layers with current blend mode
    demonstrateBlendModes();
    
    // Stack text on top using normal blending
    compositor.Stack(background_layer, foreground_layer);
    
    frame_count++;
}

void printPerformanceStats() {
    if (millis() - last_fps_time >= 5000) {  // Every 5 seconds
        float fps = frame_count / 5.0f;
        Serial.printf("Performance: %.1f FPS, Average color: ", fps);
        
        // Demonstrate new analysis functions
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
    
    // Optional: Add small delay to prevent overwhelming the system
    delay(1);
}

// Additional utility functions to demonstrate new features

void demonstrateLayerOperations() {
    // Example of using new layer operations
    
    // Scroll effect layer up to create rising fire effect
    effect_layer.scrollY(-1, CRGB::Black);
    
    // Adjust brightness based on time
    uint8_t brightness = beatsin8(60);  // Breathe effect
    effect_layer.adjustBrightness(brightness);
    
    // Copy a section of one layer to another
    background_layer.copyRect(0, 0, 32, 0, 32, 16);  // Copy left half to right
}

void demonstrateColorAnalysis() {
    // Get dominant color from background
    CRGB dominant = background_layer.getDominantColor();
    
    // Count specific colors
    uint32_t black_pixels = background_layer.getPixelCount(CRGB::Black);
    
    Serial.printf("Dominant color: R:%d G:%d B:%d, Black pixels: %d\n", 
                  dominant.r, dominant.g, dominant.b, black_pixels);
}
