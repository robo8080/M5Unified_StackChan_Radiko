// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.
// modified by robo8080

#include "DannMouth.h"

namespace m5avatar {

DannMouth::DannMouth(uint16_t minWidth, uint16_t maxWidth, uint16_t minHeight,
             uint16_t maxHeight)
    : minWidth{minWidth},
      maxWidth{maxWidth},
      minHeight{minHeight},
      maxHeight{maxHeight} {}

void DannMouth::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  Expression exp = ctx->getExpression();
  uint32_t primaryColor = ctx->getColorPalette()->get(COLOR_PRIMARY);
  uint32_t backgroundColor = ctx->getColorPalette()->get(COLOR_BACKGROUND);
  float breath = _min(1.0f, ctx->getBreath());
  float openRatio = ctx->getMouthOpenRatio();
  int h = minHeight + (maxHeight - minHeight) * openRatio;
  int h1 = h/4;
  int w = maxWidth;
  int x = rect.getLeft() - w / 2;
  int y = rect.getTop() - h / 2 + breath * 2;
  int y1 = rect.getTop() - h1 / 2 + breath * 2;

  if (openRatio == 0.0)
  {
    h = 8;
    spi->fillEllipse(x+w/2, y+h/2, w/2, h/2, (uint16_t)primaryColor);
    spi->fillEllipse(x+w/2, y+h/2, w/2-3, h/2-3, 3);
    spi->fillRect(x, y+h/2- minHeight*6-8, w+3, minHeight*6+8, (uint16_t)backgroundColor);
    spi->fillRect(x, rect.getTop()+ breath * 2, maxWidth, 3, (uint16_t)primaryColor);
  
  } else {
    spi->fillEllipse(x+w/2, y+h/2, w/2, h/2, (uint16_t)primaryColor);
    spi->fillEllipse(x+w/2, y+h/2, w/2-3, h/2-3, (uint16_t)3);
    spi->fillRect(x, y+h/2- minHeight*6-8, w+3, minHeight*6+8, (uint16_t)backgroundColor);
    spi->fillRect(x, rect.getTop()+ breath * 2, maxWidth, 3, (uint16_t)primaryColor);
  }
  spi->fillTriangle(x+w/2, y1-50, x+w/2+20, y1-20, x+w/2-20, y1-20, (uint16_t)primaryColor);
  spi->fillTriangle(x+w/2, y1-47, x+w/2+17, y1-23, x+w/2-17, y1-23, (uint16_t)backgroundColor);
}

}  // namespace m5avatar
