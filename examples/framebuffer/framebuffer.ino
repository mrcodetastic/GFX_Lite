// Hardware-independent GFX_Lite example: draw into an in-memory CRGB canvas.
#include <GFX_Lite.h>

class Canvas : public GFX {
public:
  Canvas() : GFX(64, 32) {}

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    drawPixel(x, y, color565_to_CRGB(color));
  }

  void drawPixel(int16_t x, int16_t y, CRGB color) override {
    if (x >= 0 && x < width() && y >= 0 && y < height()) {
      pixels[y][x] = color;
    }
  }

  CRGB pixels[32][64];
};

Canvas canvas;
CRGBPalette16 palette = RainbowColors_p;

void setup() {
  canvas.fillScreen(CRGB::Black);
  canvas.setTextColor(CRGB::White);
  canvas.setCursor(0, 10);
  canvas.print("GFX_Lite");

  for (int16_t x = 0; x < canvas.width(); ++x) {
    canvas.drawPixel(x, 20, ColorFromPalette(palette, static_cast<uint8_t>(x * 4)));
  }
}

void loop() {}
