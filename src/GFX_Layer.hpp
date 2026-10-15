#ifndef GFX_LITE_LAYER_HPP
#define GFX_LITE_LAYER_HPP

#include <memory>
#include <new>
#include <stdint.h>
#include <stddef.h>
#include <utility>
#include "GFX_Lite.h"

#ifndef GFX_LITE_MAX_LAYER_MEMORY
#define GFX_LITE_MAX_LAYER_MEMORY (1024UL * 1024UL)
#endif

enum textPosition { TOP, MIDDLE, BOTTOM };

/** A drawable, in-memory CRGB framebuffer with per-layer opacity. */
class GFX_Layer : public GFX {
public:
    enum class TransparencyMode : uint8_t {
        Opaque,
        ColorKey
    };

    GFX_Layer(uint16_t width, uint16_t height);
    virtual ~GFX_Layer() = default;

    GFX_Layer(const GFX_Layer&) = delete;
    GFX_Layer& operator=(const GFX_Layer&) = delete;
    GFX_Layer(GFX_Layer&&) = delete;
    GFX_Layer& operator=(GFX_Layer&&) = delete;

    void drawPixel(int16_t x, int16_t y, CRGB color) override;
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    bool setPixel(int16_t x, int16_t y, CRGB color);
    CRGB getPixel(int16_t x, int16_t y) const;
    void setAlpha(uint8_t alpha) { _alpha = alpha; }
    uint8_t getAlpha() const { return _alpha; }
    bool isTransparent(int16_t x, int16_t y) const;

    void setColorKey(CRGB color);
    void disableTransparency();
    TransparencyMode transparencyMode() const { return _transparency_mode; }
    CRGB colorKey() const { return _color_key; }

    void clear(CRGB color = CRGB::Black);
    void fillTransparent();
    void dim(uint8_t scale);
    void moveX(int delta);
    void moveY(int delta);
    void autoCenterX();
    void copyRect(int16_t src_x, int16_t src_y, int16_t dst_x, int16_t dst_y,
                  int16_t width, int16_t height);
    void scrollX(int16_t pixels, CRGB fill_color = CRGB::Black);
    void scrollY(int16_t pixels, CRGB fill_color = CRGB::Black);
    void flipHorizontal();
    void flipVertical();
    void blur(uint8_t blur_amount = 64);

    void fastFillRect(int16_t x, int16_t y, int16_t width, int16_t height, CRGB color);
    void fastFillScreen(CRGB color);
    void adjustBrightness(uint8_t scale);
    void adjustGamma(float gamma = 2.2f);
    void applyColorMatrix(const float matrix[3][3]);
    CRGB getAverageColor() const;
    CRGB getDominantColor() const;
    uint32_t getPixelCount(CRGB target_color) const;

    void drawCentreText(const char* text, textPosition position = BOTTOM,
                        const GFXfont* font = nullptr, CRGB color = CRGB(128, 128, 128),
                        int y_adjust = 0);

    /** Present pixels through a caller-owned sink; no callback is retained. */
    template<typename Sink>
    void present(Sink&& sink, bool skip_transparent = false) const {
        if (!isInitialized()) return;
        for (int16_t y = 0; y < height(); ++y) {
            for (int16_t x = 0; x < width(); ++x) {
                if (skip_transparent && isTransparent(x, y)) continue;
                sink(x, y, getPixel(x, y));
            }
        }
    }

    uint16_t getWidth() const { return static_cast<uint16_t>(width()); }
    uint16_t getHeight() const { return static_cast<uint16_t>(height()); }
    bool isValidCoordinate(int16_t x, int16_t y) const;
    bool isInitialized() const { return static_cast<bool>(_pixels); }
    size_t getMemoryUsage() const;

private:
    static uint64_t requiredBytes(uint16_t width, uint16_t height);
    size_t pixelCount() const;
    size_t pixelIndex(int16_t x, int16_t y) const;
    size_t rawPixelIndex(uint16_t x, uint16_t y) const;
    void invalidateDimensions();
    void copyPixel(size_t source, size_t destination);

    std::unique_ptr<CRGB[]> _pixels;
    uint8_t _alpha;
    CRGB _color_key;
    TransparencyMode _transparency_mode;
};

/** Stateless pixel compositor. Results are written to a destination layer. */
class GFX_LayerCompositor {
public:
    bool composite(const GFX_Layer& background, const GFX_Layer& foreground,
                   GFX_Layer& destination) const;
    bool compositeMasked(const GFX_Layer& background, const GFX_Layer& foreground,
                         const GFX_Layer& mask, GFX_Layer& destination) const;

private:
    static CRGB mix(CRGB from, CRGB to, uint8_t amount);
    bool compositeImpl(const GFX_Layer& background, const GFX_Layer& foreground,
                        const GFX_Layer* mask, GFX_Layer& destination) const;
};

#endif // GFX_LITE_LAYER_HPP
