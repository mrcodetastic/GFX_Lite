#include "GFX_Layer.hpp"

#include <math.h>

namespace {

uint8_t mixChannel(uint8_t from, uint8_t to, uint8_t amount) {
    const uint32_t inverse = 255U - amount;
    return static_cast<uint8_t>((static_cast<uint32_t>(from) * inverse +
                                 static_cast<uint32_t>(to) * amount + 127U) / 255U);
}

uint8_t multiplyChannel(uint8_t a, uint8_t b) {
    return static_cast<uint8_t>((static_cast<uint32_t>(a) * b + 127U) / 255U);
}

int32_t clampInt32(int32_t value, int32_t lower, int32_t upper) {
    return value < lower ? lower : (value > upper ? upper : value);
}

} // namespace

GFX_Layer::GFX_Layer(uint16_t layer_width, uint16_t layer_height)
    : GFX((layer_width <= INT16_MAX && layer_height <= INT16_MAX)
              ? static_cast<int16_t>(layer_width) : 0,
          (layer_width <= INT16_MAX && layer_height <= INT16_MAX)
              ? static_cast<int16_t>(layer_height) : 0),
      _alpha(255),
      _color_key(CRGB::Black),
      _transparency_mode(TransparencyMode::Opaque) {
    if (layer_width == 0 || layer_height == 0 ||
        layer_width > INT16_MAX || layer_height > INT16_MAX ||
        requiredBytes(layer_width, layer_height) > GFX_LITE_MAX_LAYER_MEMORY ||
        requiredBytes(layer_width, layer_height) >
            static_cast<uint64_t>(static_cast<size_t>(-1))) {
        invalidateDimensions();
        return;
    }

    const size_t count = static_cast<size_t>(layer_width) * layer_height;
    _pixels.reset(allocatePixels(count));
    if (!_pixels) {
        invalidateDimensions();
        return;
    }
    for (size_t i = 0; i < count; ++i) _pixels[i] = CRGB::Black;
}

CRGB* GFX_Layer::allocatePixels(size_t count) {
    if (count == 0) return nullptr;
    const size_t bytes = count * sizeof(CRGB);
#if GFX_LITE_LAYER_USE_PSRAM
    // Prefer external PSRAM; fall back to internal RAM if PSRAM is absent or
    // exhausted. Both paths use heap_caps_* so GFX_LayerPixelDeleter can
    // release either.
    CRGB* pixels = static_cast<CRGB*>(
        heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!pixels) {
        pixels = static_cast<CRGB*>(heap_caps_malloc(bytes, MALLOC_CAP_8BIT));
    }
    return pixels;
#else
    return new (std::nothrow) CRGB[count];
#endif
}

uint64_t GFX_Layer::requiredBytes(uint16_t layer_width, uint16_t layer_height) {
    const uint64_t count = static_cast<uint64_t>(layer_width) * layer_height;
    return count * sizeof(CRGB);
}

void GFX_Layer::invalidateDimensions() {
    _pixels.reset();
    GFX::WIDTH = GFX::HEIGHT = 0;
    GFX::_width = GFX::_height = 0;
}

size_t GFX_Layer::pixelCount() const {
    if (GFX::WIDTH <= 0 || GFX::HEIGHT <= 0) return 0;
    return static_cast<size_t>(GFX::WIDTH) * static_cast<size_t>(GFX::HEIGHT);
}

size_t GFX_Layer::rawPixelIndex(uint16_t x, uint16_t y) const {
    return static_cast<size_t>(y) * static_cast<size_t>(GFX::WIDTH) + x;
}

size_t GFX_Layer::pixelIndex(int16_t x, int16_t y) const {
    uint16_t raw_x = static_cast<uint16_t>(x);
    uint16_t raw_y = static_cast<uint16_t>(y);
    switch (getRotation()) {
        case 1:
            raw_x = static_cast<uint16_t>(y);
            raw_y = static_cast<uint16_t>(GFX::HEIGHT - 1 - x);
            break;
        case 2:
            raw_x = static_cast<uint16_t>(GFX::WIDTH - 1 - x);
            raw_y = static_cast<uint16_t>(GFX::HEIGHT - 1 - y);
            break;
        case 3:
            raw_x = static_cast<uint16_t>(GFX::WIDTH - 1 - y);
            raw_y = static_cast<uint16_t>(x);
            break;
        default:
            break;
    }
    return rawPixelIndex(raw_x, raw_y);
}

bool GFX_Layer::isValidCoordinate(int16_t x, int16_t y) const {
    return isInitialized() && x >= 0 && y >= 0 && x < width() && y < height();
}

void GFX_Layer::drawPixel(int16_t x, int16_t y, CRGB color) {
    setPixel(x, y, color);
}

void GFX_Layer::drawPixel(int16_t x, int16_t y, uint16_t color) {
    setPixel(x, y, color565_to_CRGB(color));
}

bool GFX_Layer::setPixel(int16_t x, int16_t y, CRGB color) {
    if (!isValidCoordinate(x, y)) return false;
    const size_t index = pixelIndex(x, y);
    _pixels[index] = color;
    return true;
}

CRGB GFX_Layer::getPixel(int16_t x, int16_t y) const {
    if (!isValidCoordinate(x, y)) return CRGB::Black;
    return _pixels[pixelIndex(x, y)];
}

void GFX_Layer::setColorKey(CRGB color) {
    _color_key = color;
    _transparency_mode = TransparencyMode::ColorKey;
}

void GFX_Layer::disableTransparency() {
    _transparency_mode = TransparencyMode::Opaque;
}

bool GFX_Layer::isTransparent(int16_t x, int16_t y) const {
    if (!isValidCoordinate(x, y)) return true;
    if (_alpha == 0) return true;
    const size_t index = pixelIndex(x, y);
    return _transparency_mode == TransparencyMode::ColorKey && _pixels[index] == _color_key;
}

void GFX_Layer::clear(CRGB color) {
    if (!isInitialized()) return;
    const size_t count = pixelCount();
    for (size_t i = 0; i < count; ++i) _pixels[i] = color;
}

void GFX_Layer::fillTransparent() {
    if (!isInitialized()) return;
    if (_transparency_mode == TransparencyMode::ColorKey) {
        clear(_color_key);
    } else {
        _alpha = 0;
    }
}

void GFX_Layer::dim(uint8_t scale) {
    adjustBrightness(scale);
}

void GFX_Layer::moveX(int delta) {
    const int32_t shift = clampInt32(delta, INT16_MIN, INT16_MAX);
    scrollX(static_cast<int16_t>(shift));
}

void GFX_Layer::moveY(int delta) {
    const int32_t shift = clampInt32(delta, INT16_MIN, INT16_MAX);
    scrollY(static_cast<int16_t>(shift));
}

void GFX_Layer::autoCenterX() {
    if (!isInitialized()) return;
    int16_t left = -1;
    int16_t right = -1;
    for (int16_t x = 0; x < width(); ++x) {
        for (int16_t y = 0; y < height(); ++y) {
            if (!isTransparent(x, y) && getPixel(x, y) != CRGB::Black) {
                if (left < 0) left = x;
                right = x;
            }
        }
    }
    if (left < 0) return;
    const int16_t shift = static_cast<int16_t>((width() - (right - left + 1)) / 2 - left);
    moveX(shift);
}

void GFX_Layer::copyRect(int16_t src_x, int16_t src_y, int16_t dst_x, int16_t dst_y,
                         int16_t copy_width, int16_t copy_height) {
    if (!isInitialized() || copy_width <= 0 || copy_height <= 0) return;

    int32_t sx = src_x, sy = src_y, dx = dst_x, dy = dst_y;
    int32_t w = copy_width, h = copy_height;
    if (sx < 0) { const int32_t d = -sx; sx = 0; dx += d; w -= d; }
    if (sy < 0) { const int32_t d = -sy; sy = 0; dy += d; h -= d; }
    if (dx < 0) { const int32_t d = -dx; dx = 0; sx += d; w -= d; }
    if (dy < 0) { const int32_t d = -dy; dy = 0; sy += d; h -= d; }
    if (sx + w > width()) w = width() - sx;
    if (dx + w > width()) w = width() - dx;
    if (sy + h > height()) h = height() - sy;
    if (dy + h > height()) h = height() - dy;
    if (w <= 0 || h <= 0) return;

    const int32_t y_start = (dy > sy) ? h - 1 : 0;
    const int32_t y_end = (dy > sy) ? -1 : h;
    const int32_t y_step = (dy > sy) ? -1 : 1;
    for (int32_t row = y_start; row != y_end; row += y_step) {
        const int32_t x_start = (dy == sy && dx > sx) ? w - 1 : 0;
        const int32_t x_end = (dy == sy && dx > sx) ? -1 : w;
        const int32_t x_step = (dy == sy && dx > sx) ? -1 : 1;
        for (int32_t col = x_start; col != x_end; col += x_step) {
            const size_t source = pixelIndex(static_cast<int16_t>(sx + col),
                                             static_cast<int16_t>(sy + row));
            const size_t destination = pixelIndex(static_cast<int16_t>(dx + col),
                                                  static_cast<int16_t>(dy + row));
            copyPixel(source, destination);
        }
    }
}

void GFX_Layer::copyPixel(size_t source, size_t destination) {
    _pixels[destination] = _pixels[source];
}

void GFX_Layer::scrollX(int16_t pixels, CRGB fill_color) {
    if (!isInitialized() || pixels == 0) return;
    const int32_t shift = pixels;
    const int32_t w = width();
    const int32_t h = height();
    if (shift >= w || shift <= -w) {
        fastFillScreen(fill_color);
        return;
    }
    for (int32_t y = 0; y < h; ++y) {
        if (shift > 0) {
            for (int32_t x = w - 1; x >= shift; --x)
                copyPixel(pixelIndex(x - shift, y), pixelIndex(x, y));
            for (int32_t x = 0; x < shift; ++x) {
                setPixel(x, y, fill_color);
            }
        } else {
            const int32_t left = -shift;
            for (int32_t x = 0; x < w - left; ++x)
                copyPixel(pixelIndex(x + left, y), pixelIndex(x, y));
            for (int32_t x = w - left; x < w; ++x) {
                setPixel(x, y, fill_color);
            }
        }
    }
}

void GFX_Layer::scrollY(int16_t pixels, CRGB fill_color) {
    if (!isInitialized() || pixels == 0) return;
    const int32_t shift = pixels;
    const int32_t w = width();
    const int32_t h = height();
    if (shift >= h || shift <= -h) {
        fastFillScreen(fill_color);
        return;
    }
    if (shift > 0) {
        for (int32_t y = h - 1; y >= shift; --y)
            for (int32_t x = 0; x < w; ++x)
                copyPixel(pixelIndex(x, y - shift), pixelIndex(x, y));
        for (int32_t y = 0; y < shift; ++y)
            for (int32_t x = 0; x < w; ++x) {
                setPixel(x, y, fill_color);
            }
    } else {
        const int32_t up = -shift;
        for (int32_t y = 0; y < h - up; ++y)
            for (int32_t x = 0; x < w; ++x)
                copyPixel(pixelIndex(x, y + up), pixelIndex(x, y));
        for (int32_t y = h - up; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                setPixel(x, y, fill_color);
            }
    }
}

void GFX_Layer::flipHorizontal() {
    if (!isInitialized()) return;
    for (int16_t y = 0; y < height(); ++y) {
        for (int16_t x = 0; x < width() / 2; ++x) {
            const size_t a = pixelIndex(x, y);
            const size_t b = pixelIndex(static_cast<int16_t>(width() - 1 - x), y);
            CRGB color = _pixels[a]; _pixels[a] = _pixels[b]; _pixels[b] = color;
        }
    }
}

void GFX_Layer::flipVertical() {
    if (!isInitialized()) return;
    for (int16_t y = 0; y < height() / 2; ++y) {
        for (int16_t x = 0; x < width(); ++x) {
            const size_t a = pixelIndex(x, y);
            const size_t b = pixelIndex(x, static_cast<int16_t>(height() - 1 - y));
            CRGB color = _pixels[a]; _pixels[a] = _pixels[b]; _pixels[b] = color;
        }
    }
}

void GFX_Layer::fastFillRect(int16_t x, int16_t y, int16_t rect_width,
                             int16_t rect_height, CRGB color) {
    if (!isInitialized() || rect_width <= 0 || rect_height <= 0) return;
    int32_t left = x, top = y, right = left + rect_width, bottom = top + rect_height;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > width()) right = width();
    if (bottom > height()) bottom = height();
    for (int32_t row = top; row < bottom; ++row)
        for (int32_t col = left; col < right; ++col)
            setPixel(static_cast<int16_t>(col), static_cast<int16_t>(row), color);
}

void GFX_Layer::fastFillScreen(CRGB color) {
    if (!isInitialized()) return;
    const size_t count = pixelCount();
    for (size_t i = 0; i < count; ++i) _pixels[i] = color;
}

void GFX_Layer::adjustBrightness(uint8_t scale) {
    if (!isInitialized()) return;
    for (size_t i = 0; i < pixelCount(); ++i) _pixels[i].nscale8(scale);
}

void GFX_Layer::blur(uint8_t blur_amount) {
    if (!isInitialized() || blur_amount == 0) return;
    const size_t count = pixelCount();
    PixelBuffer source(allocatePixels(count));
    if (!source) return;
    for (size_t i = 0; i < count; ++i) source[i] = _pixels[i];
    for (int32_t y = 0; y < GFX::HEIGHT; ++y) {
        for (int32_t x = 0; x < GFX::WIDTH; ++x) {
            uint32_t red = 0, green = 0, blue = 0;
            for (int32_t dy = -1; dy <= 1; ++dy) {
                for (int32_t dx = -1; dx <= 1; ++dx) {
                    const int32_t sample_x = x + dx;
                    const int32_t sample_y = y + dy;
                    if (sample_x < 0 || sample_x >= GFX::WIDTH ||
                        sample_y < 0 || sample_y >= GFX::HEIGHT) continue;
                    const CRGB pixel = source[rawPixelIndex(x + dx, y + dy)];
                    red += pixel.r; green += pixel.g; blue += pixel.b;
                }
            }
            const CRGB average(red / 9, green / 9, blue / 9);
            const size_t index = rawPixelIndex(x, y);
            _pixels[index] = blend(source[index], average, blur_amount);
        }
    }
}

void GFX_Layer::adjustGamma(float gamma) {
    if (!isInitialized() || gamma <= 0.0f) return;
    for (size_t i = 0; i < pixelCount(); ++i) {
        for (uint8_t channel = 0; channel < 3; ++channel) {
            const float normalized = static_cast<float>(_pixels[i][channel]) / 255.0f;
            float corrected = ::pow(normalized, gamma) * 255.0f + 0.5f;
            if (corrected < 0.0f) corrected = 0.0f;
            if (corrected > 255.0f) corrected = 255.0f;
            _pixels[i][channel] = static_cast<uint8_t>(corrected);
        }
    }
}

void GFX_Layer::applyColorMatrix(const float matrix[3][3]) {
    if (!isInitialized() || !matrix) return;
    for (size_t i = 0; i < pixelCount(); ++i) {
        const float red = static_cast<float>(_pixels[i].r);
        const float green = static_cast<float>(_pixels[i].g);
        const float blue = static_cast<float>(_pixels[i].b);
        uint8_t output[3];
        for (uint8_t row = 0; row < 3; ++row) {
            float value = matrix[row][0] * red + matrix[row][1] * green + matrix[row][2] * blue;
            if (value < 0.0f) value = 0.0f;
            if (value > 255.0f) value = 255.0f;
            output[row] = static_cast<uint8_t>(value + 0.5f);
        }
        _pixels[i].setRGB(output[0], output[1], output[2]);
    }
}

CRGB GFX_Layer::getAverageColor() const {
    if (!isInitialized() || pixelCount() == 0) return CRGB::Black;
    uint64_t red = 0, green = 0, blue = 0;
    const size_t count = pixelCount();
    for (size_t i = 0; i < count; ++i) {
        red += _pixels[i].r; green += _pixels[i].g; blue += _pixels[i].b;
    }
    return CRGB(red / count, green / count, blue / count);
}

CRGB GFX_Layer::getDominantColor() const {
    if (!isInitialized()) return CRGB::Black;
    uint32_t histogram[64] = {};
    uint32_t visible_count = 0;
    for (int16_t y = 0; y < height(); ++y) {
        for (int16_t x = 0; x < width(); ++x) {
            if (isTransparent(x, y)) continue;
            const CRGB color = getPixel(x, y);
            const uint8_t bin = static_cast<uint8_t>((color.r >> 6) << 4 |
                                                     (color.g >> 6) << 2 |
                                                     (color.b >> 6));
            ++histogram[bin];
            ++visible_count;
        }
    }
    if (visible_count == 0) return CRGB::Black;
    uint8_t dominant = 0;
    for (uint8_t i = 1; i < 64; ++i)
        if (histogram[i] > histogram[dominant]) dominant = static_cast<uint8_t>(i);
    return CRGB(((dominant >> 4) & 0x03) * 85 + 42,
                ((dominant >> 2) & 0x03) * 85 + 42,
                (dominant & 0x03) * 85 + 42);
}

uint32_t GFX_Layer::getPixelCount(CRGB target_color) const {
    if (!isInitialized()) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < pixelCount(); ++i)
        if (_pixels[i] == target_color) ++count;
    return count;
}

size_t GFX_Layer::getMemoryUsage() const {
    if (!isInitialized()) return 0;
    return pixelCount() * sizeof(CRGB);
}

void GFX_Layer::drawCentreText(const char* text, textPosition position,
                               const GFXfont* font, CRGB color, int y_adjust) {
    if (!isInitialized() || !text) return;
    int16_t x1 = 0, y1 = 0;
    uint16_t text_width = 0, text_height = 0;
    setTextWrap(false);
    setFont(font);
    getTextBounds(text, 0, 0, &x1, &y1, &text_width, &text_height);
    if (!font) {
        if (position == TOP) setCursor((width() - text_width) / 2, 0);
        else if (position == BOTTOM) setCursor((width() - text_width) / 2, height() - text_height);
        else setCursor((width() - text_width) / 2, (height() - text_height) / 2);
    } else {
        const int16_t centered_x = static_cast<int16_t>((width() - text_width) / 2);
        if (position == TOP) setCursor(centered_x, text_height + y_adjust);
        else if (position == BOTTOM) setCursor(centered_x + 1, height() - 1 + y_adjust);
        else setCursor(centered_x, (height() / 2) + (text_height / 2) + y_adjust);
    }
    setTextColor(color);
    print(text);
}

CRGB GFX_LayerCompositor::mix(CRGB from, CRGB to, uint8_t amount) {
    return CRGB(mixChannel(from.r, to.r, amount),
                mixChannel(from.g, to.g, amount),
                mixChannel(from.b, to.b, amount));
}

bool GFX_LayerCompositor::composite(const GFX_Layer& background,
                                   const GFX_Layer& foreground,
                                   GFX_Layer& destination) const {
    return compositeImpl(background, foreground, nullptr, destination);
}

bool GFX_LayerCompositor::compositeMasked(const GFX_Layer& background,
                                          const GFX_Layer& foreground,
                                          const GFX_Layer& mask,
                                          GFX_Layer& destination) const {
    return compositeImpl(background, foreground, &mask, destination);
}

bool GFX_LayerCompositor::compositeImpl(const GFX_Layer& background,
                                        const GFX_Layer& foreground,
                                        const GFX_Layer* mask,
                                        GFX_Layer& destination) const {
    if (!background.isInitialized() || !foreground.isInitialized() ||
        !destination.isInitialized() || (mask && !mask->isInitialized())) return false;

    int16_t width = background.width();
    int16_t height = background.height();
    if (foreground.width() < width) width = foreground.width();
    if (destination.width() < width) width = destination.width();
    if (foreground.height() < height) height = foreground.height();
    if (destination.height() < height) height = destination.height();
    if (mask) {
        if (mask->width() < width) width = mask->width();
        if (mask->height() < height) height = mask->height();
    }
    if (width <= 0 || height <= 0) return false;

    for (int16_t y = 0; y < height; ++y) {
        for (int16_t x = 0; x < width; ++x) {
            const CRGB base = background.getPixel(x, y);
            const CRGB over = foreground.getPixel(x, y);
            uint8_t alpha = foreground.getAlpha();
            if (foreground.isTransparent(x, y)) alpha = 0;

            if (mask) {
                if (mask->isTransparent(x, y)) alpha = 0;
                else {
                    const CRGB mask_color = mask->getPixel(x, y);
                    const uint8_t luminance = static_cast<uint8_t>(
                        (static_cast<uint16_t>(mask_color.r) + mask_color.g + mask_color.b) / 3U);
                    alpha = multiplyChannel(alpha, luminance);
                    alpha = multiplyChannel(alpha, mask->getAlpha());
                }
            }

            destination.setPixel(x, y, mix(base, over, alpha));
        }
    }
    return true;
}
