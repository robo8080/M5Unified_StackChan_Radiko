// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef DANNEYEBLOW_H_
#define DANNEYEBLOW_H_

#include <M5Unified.h>
#include "BoundingRect.h"
#include "DrawContext.h"
#include "Drawable.h"

namespace m5avatar {
class DannEyeblow final : public Drawable {
 private:
  uint16_t width;
  uint16_t height;
  bool isLeft;

 public:
  // constructor
  DannEyeblow() = delete;
  DannEyeblow(uint16_t w, uint16_t h, bool isLeft);
  ~DannEyeblow() = default;
  DannEyeblow(const DannEyeblow &other) = default;
  DannEyeblow &operator=(const DannEyeblow &other) = default;
  void draw(M5Canvas *spi, BoundingRect rect,
            DrawContext *drawContext) override;
};

}  // namespace m5avatar

#endif  // DANNEYEBLOW_H_
