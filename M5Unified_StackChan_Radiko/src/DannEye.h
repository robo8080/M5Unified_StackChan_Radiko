// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef DANNEYE_H_
#define DANNEYE_H_

#include <M5Unified.h>
#include "DrawContext.h"
#include "Drawable.h"

namespace m5avatar {

class DannEye final : public Drawable {
 private:
  uint16_t r;
  bool isLeft;

 public:
  // constructor
  DannEye() = delete;
  DannEye(uint16_t x, uint16_t y, uint16_t r, bool isLeft);  // deprecated
  DannEye(uint16_t r, bool isLeft);
  ~DannEye() = default;
  DannEye(const DannEye &other) = default;
  DannEye &operator=(const DannEye &other) = default;
  void draw(M5Canvas *spi, BoundingRect rect,
            DrawContext *drawContext) override;
//  void draw(M5Canvas *spi, BoundingRect rect,
//            DrawContext *drawContext) override;
  // void draw(M5Canvas *spi, DrawContext *drawContext) override; // deprecated
};

}  // namespace m5avatar

#endif  // DANNEYE_H_
