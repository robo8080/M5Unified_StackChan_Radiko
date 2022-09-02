// Copyright (c) robo8080. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef ATARUEYEBLOW_H_
#define ATARUEYEBLOW_H_

#include <M5Unified.h>
#include "BoundingRect.h"
#include "DrawContext.h"
#include "Drawable.h"

namespace m5avatar {
class AtaruEyeblow final : public Drawable {
 private:
  uint16_t width;
  uint16_t height;
  bool isLeft;

 public:
  // constructor
  AtaruEyeblow() = delete;
  AtaruEyeblow(uint16_t w, uint16_t h, bool isLeft);
  ~AtaruEyeblow() = default;
  AtaruEyeblow(const AtaruEyeblow &other) = default;
  AtaruEyeblow &operator=(const AtaruEyeblow &other) = default;
  void draw(M5Canvas *spi, BoundingRect rect,
            DrawContext *drawContext) override;
};

}  // namespace m5avatar

#endif  // ATARUEYEBLOW_H_
