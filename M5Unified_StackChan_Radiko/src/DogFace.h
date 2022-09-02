// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef FACES_DOGFACE_H_
#define FACES_DOGFACE_H_

#define LGFX_USE_V1
#include <M5GFX.h>
//#include <M5Unified.h> // TODO(meganetaaan): include only the Sprite function not a whole library
#include "BoundingRect.h"
#include "DrawContext.h"
#include "Drawable.h"

namespace m5avatar {
class DogEyeblow : public Drawable {
 private:
  uint16_t width;
  uint16_t height;
  bool isLeft;

 public:

DogEyeblow(uint16_t w, uint16_t h, bool isLeft)
    : width{w}, height{h}, isLeft{isLeft} {}

void draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  Expression exp = ctx->getExpression();
  float openRatio = ctx->getMouthOpenRatio();
  uint32_t x = rect.getLeft();
  uint32_t y = rect.getTop();
  Gaze g = ctx->getGaze();
  uint32_t offsetX = g.getHorizontal() * 5;
  uint32_t offsetY = g.getVertical() * 7;
  x += offsetX;
  y += offsetY;
  uint32_t primaryColor = ctx->getColorPalette()->get(COLOR_PRIMARY);
  if (width == 0 || height == 0) {
    return;
  }
  // draw two triangles to make rectangle
  if ( exp == Expression::Neutral ) {
    int x1, y1, x2, y2, x3, y3, x4, y4;
    int a = isLeft ? -1 : 1;
    int dx = (int)((float)(a * 3)*openRatio);
    int dy = (int)((float)(a * 5)*openRatio);
    x1 = x - width / 2;
    x2 = x1 - dx;
    x4 = x + width / 2;
    x3 = x4 + dx;
    y1 = y - height / 2 - dy;
    y2 = y + height / 2 - dy;
    y3 = y - height / 2 + dy;
    y4 = y + height / 2 + dy;
    spi->fillTriangle(x1, y1, x2, y2, x3, y3, (uint16_t)primaryColor);
    spi->fillTriangle(x2, y2, x3, y3, x4, y4, (uint16_t)primaryColor);    
  } else
  if (exp == Expression::Angry || exp == Expression::Sad) {
    int x1, y1, x2, y2, x3, y3, x4, y4;
    int a = isLeft ^ (exp == Expression::Sad) ? -1 : 1;
    int dx = a * 3;
    int dy = a * 5;
    x1 = x - width / 2;
    x2 = x1 - dx;
    x4 = x + width / 2;
    x3 = x4 + dx;
    y1 = y - height / 2 - dy;
    y2 = y + height / 2 - dy;
    y3 = y - height / 2 + dy;
    y4 = y + height / 2 + dy;
    spi->fillTriangle(x1, y1, x2, y2, x3, y3, (uint16_t)primaryColor);
    spi->fillTriangle(x2, y2, x3, y3, x4, y4, (uint16_t)primaryColor);
  } else if(exp == Expression::Doubt ||exp == Expression::Sleepy) {
//  } else if(exp == Expression::Doubt || exp == Expression::Neutral || exp == Expression::Sleepy) {
    if (exp == Expression::Sleepy) {
      y = y + 20;
    }
    int x1 = x - width / 2;
    int y1 = y - height / 2;
    spi->fillRect(x1, y1, width, height, (uint16_t)primaryColor);
  } else {
//    int x1 = x - width / 2;
//    int y1 = y - height / 2;
    if (exp == Expression::Happy) {
//      y1 = y1 - 5;
        y = y - 5;
    }
//    spi->fillRect(x1, y1, width, height, primaryColor);
    int x1, y1, x2, y2, x3, y3, x4, y4;
    int a = isLeft ^ (exp == Expression::Happy) ? -1 : 1;
    int dx = a * 1;
    int dy = a * 2;
    x1 = x - width / 2;
    x2 = x1 - dx;
    x4 = x + width / 2;
    x3 = x4 + dx;
    y1 = y - height / 2 - dy;
    y2 = y + height / 2 - dy;
    y3 = y - height / 2 + dy;
    y4 = y + height / 2 + dy;
    spi->fillTriangle(x1, y1, x2, y2, x3, y3, (uint16_t)primaryColor);
    spi->fillTriangle(x2, y2, x3, y3, x4, y4, (uint16_t)primaryColor);
  }
}
};

class DogEye : public Drawable {
  void draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
    uint32_t cx = rect.getCenterX();
    uint32_t cy = rect.getCenterY();
    Gaze g = ctx->getGaze();
    ColorPalette *cp = ctx->getColorPalette();
    uint16_t primaryColor = COLOR_DEPTH == 1 ? 1 : cp->get(COLOR_PRIMARY);
    uint16_t backgroundColor = COLOR_DEPTH == 1 ? ERACER_COLOR : cp->get(COLOR_BACKGROUND);
    uint32_t offsetX = g.getHorizontal() * 8;
    uint32_t offsetY = g.getVertical() * 5;
    float eor = ctx->getEyeOpenRatio();

    if (eor == 0) {
      // eye closed
      spi->fillRect(cx - 15, cy - 2, 30, 4, primaryColor);
      return;
    }
    spi->fillEllipse(cx, cy, 30, 25, primaryColor);
    spi->fillEllipse(cx, cy, 28, 23, backgroundColor);

    spi->fillEllipse(cx + offsetX, cy + offsetY, 18, 18,
                     primaryColor);
    spi->fillEllipse(cx + offsetX - 3, cy + offsetY - 3, 3, 3,
                     backgroundColor);
  }
};

class DogMouth : public Drawable {
 private:
  uint16_t minWidth;
  uint16_t maxWidth;
  uint16_t minHeight;
  uint16_t maxHeight;

 public:
  DogMouth() : DogMouth(60, 100, 4, 60) {}
  DogMouth(uint16_t minWidth, uint16_t maxWidth, uint16_t minHeight,
           uint16_t maxHeight)
      : minWidth{minWidth},
        maxWidth{maxWidth},
        minHeight{minHeight},
        maxHeight{maxHeight} {}
  void draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
    uint16_t primaryColor = COLOR_DEPTH == 1 ? 1 : ctx->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t backgroundColor = COLOR_DEPTH == 1 ? ERACER_COLOR : ctx->getColorPalette()->get(COLOR_BACKGROUND);
    uint32_t cx = rect.getCenterX();
    uint32_t cy = rect.getCenterY();
    float openRatio = ctx->getMouthOpenRatio();
    uint32_t h = minHeight + (maxHeight - minHeight) * openRatio;
    uint32_t w = minWidth + (maxWidth - minWidth) * (1 - openRatio);
    if (h > minHeight) {
      spi->fillEllipse(cx, cy+7, w / 2, h / 2, primaryColor);
      spi->fillEllipse(cx, cy+7, w / 2 - 4, h / 2 - 4, 3);
      spi->fillRect(cx - w / 2, cy+7 - h / 2, w, h / 2, backgroundColor);
    }
    spi->fillEllipse(cx, cy - 15, 10, 6, primaryColor);
    spi->fillEllipse(cx - 28, cy, 30, 15, primaryColor);
    spi->fillEllipse(cx + 28, cy, 30, 15, primaryColor);
    spi->fillEllipse(cx - 29, cy - 4, 27, 15, backgroundColor);
    spi->fillEllipse(cx + 29, cy - 4, 27, 15, backgroundColor);
  }
};

class DogFace : public Face {
 public:
  DogFace()
      : Face(new DogMouth(), new BoundingRect(168-10, 163),
             new DogEye(),   new BoundingRect(103-10, 80), 
             new DogEye(),   new BoundingRect(106-10, 240), 
             new DogEyeblow(25, 3, false), new BoundingRect(55-10, 90), 
             new DogEyeblow(25, 3, true),  new BoundingRect(55-10, 230)) {}
  DogFace(LGFX_Device* device)
      : Face(new DogMouth(), new BoundingRect(168-10, 163),
             new DogEye(),   new BoundingRect(103-10, 80), 
             new DogEye(),   new BoundingRect(106-10, 240), 
             new DogEyeblow(25, 3, false), new BoundingRect(55-10, 90), 
             new DogEyeblow(25, 3, true),  new BoundingRect(55-10, 230), device) {}
};

}  // namespace m5avatar

#endif  // FACES_DOGFACE_H_
