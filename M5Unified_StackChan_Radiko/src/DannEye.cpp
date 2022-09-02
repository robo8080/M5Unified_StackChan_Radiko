// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.
// modified by robo8080

#include "DannEye.h"
namespace m5avatar {

DannEye::DannEye(uint16_t x, uint16_t y, uint16_t r, bool isLeft) : DannEye(r, isLeft) {}

DannEye::DannEye(uint16_t r, bool isLeft) : r{r}, isLeft{isLeft} {}
#if 0
void DannEye::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  Expression exp = ctx->getExpression();
  uint32_t cx = rect.getCenterX();
  uint32_t cy = rect.getCenterY();
  Gaze g = ctx->getGaze();
  ColorPalette *cp = ctx->getColorPalette();
  uint16_t primaryColor = cp->get(COLOR_PRIMARY);
  uint16_t secondaryColor = cp->get(COLOR_SECONDARY);
  uint16_t backgroundColor = COLOR_DEPTH == 1 ? ERACER_COLOR : cp->get(COLOR_BACKGROUND);
  uint32_t offsetX = g.getHorizontal() * 8;
  uint32_t offsetY = g.getVertical() * 5;
  float eor = ctx->getEyeOpenRatio();

  if (eor == 0) {
    // eye closed
    spi->fillRect(cx - 15, cy - 2, 30, 4, primaryColor);
    return;
  }
  if (exp == Expression::Happy) {
    spi->fillEllipse(cx, cy, 30, 25, primaryColor);
    spi->fillEllipse(cx, cy, 24, 19, backgroundColor);
    spi->fillRect(cx-30, cy, 60 ,27, backgroundColor);    
  } else {   
    spi->fillEllipse(cx, cy, 30, 25, primaryColor);
//    spi->fillEllipse(cx, cy, 28, 23, backgroundColor);
    spi->fillEllipse(cx, cy, 28, 23, secondaryColor);

    spi->fillEllipse(cx + offsetX, cy + offsetY, 18, 18,
                     primaryColor);
//    spi->fillEllipse(cx + offsetX - 3, cy + offsetY - 3, 3, 3,
//                     backgroundColor);
    spi->fillEllipse(cx + offsetX - 3, cy + offsetY - 3, 3, 3,
                     secondaryColor);

    if (exp == Expression::Sleepy) {
      spi->fillRect(cx-30, cy-27, 60 ,27, backgroundColor);
    }  
  }  
}
#else
void DannEye::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  Expression exp = ctx->getExpression();
  uint32_t x = rect.getCenterX();
  uint32_t y = rect.getCenterY();
  Gaze g = ctx->getGaze();
  float openRatio = ctx->getEyeOpenRatio();
  uint32_t offsetX = g.getHorizontal() * 5;
  uint32_t offsetY = g.getVertical() * 7;
  uint32_t primaryColor = ctx->getColorPalette()->get(COLOR_PRIMARY);
  uint32_t secondaryColor = ctx->getColorPalette()->get(COLOR_SECONDARY);
  uint32_t backgroundColor = ctx->getColorPalette()->get(COLOR_BACKGROUND);
  if (openRatio > 0) {
//    spi->fillEllipse(x , y , r, r + 8, primaryColor);
    spi->fillEllipse(x , y , r + 8, r , (uint16_t)primaryColor);
    spi->fillEllipse(x , y , r + 6, r-2 , (uint16_t)secondaryColor);
//    spi->fillCircle(x + offsetX, y + offsetY, 10, backgroundColor);
    if (exp != Expression::Happy) {
//      spi->fillCircle(x + offsetX, y + offsetY, 13, (uint16_t)backgroundColor);
      spi->fillCircle(x + offsetX, y + offsetY, 13, (uint16_t)primaryColor);
      spi->fillCircle(x + offsetX-4, y + offsetY-4, 3, (uint16_t)secondaryColor);
    }
    // TODO(meganetaaan): Refactor
    if (exp == Expression::Angry || exp == Expression::Sad) {
      int x0, y0, x1, y1, x2, y2;
//      x0 = x - r ;
      x0 = x - r -8 ;
//      y0 = y - r - 8;
      y0 = y - r ;
      x1 = x0 + (r+8) * 2;
      y1 = y0;
      x2 = !isLeft != !(exp == Expression::Sad) ? x0 : x1;
      y2 = y0 + r;
      spi->fillTriangle(x0, y0, x1, y1, x2, y2, (uint16_t)backgroundColor);
    }
    if (exp == Expression::Happy || exp == Expression::Sleepy) {
      int x0, y0, w, h;
      x0 = x - r;
//      y0 = y - r -5;
      y0 = y - r;
      w = r * 2 + 4;
      h = r + 7;
      if (exp == Expression::Happy) {
        y0 += (r + 5);
//        y0 += r;
        spi->fillEllipse(x , y , r + 8, r , (uint16_t)primaryColor);
        spi->fillEllipse(x , y , r / 1.5, (r + 5) / 1.5, (uint16_t)backgroundColor);
//        spi->fillEllipse(x , y , (r + 8) / 1.5, r / 1.5, (uint16_t)primaryColor);
      }
      spi->fillRect(x0-8, y0-3, w+15, h, (uint16_t)backgroundColor);
//      spi->fillRect(x0-8, y0-3, w+15, h, (uint16_t)primaryColor);
    }
  } else {
    int x1 = x - r + offsetX;
    int y1 = y - 2 + offsetY;
    int w = r * 2;
    int h = 4;
    spi->fillRect(x1, y1, w, h, (uint16_t)primaryColor);
  }
}
#endif
}  // namespace m5avatar
