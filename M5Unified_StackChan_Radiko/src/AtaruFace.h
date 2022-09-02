// Copyright (c) robo8080. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef FACES_ATARUFACE_H_
#define FACES_ATARUFACE_H_

#include <M5Unified.h>
#include "AtaruEye.h"
#include "AtaruMouth.h"
#include "AtaruEyeblow.h"

namespace m5avatar {
class AtaruFace : public Face {
 public:
  AtaruFace()
      : Face(new AtaruMouth(50, 100, 4, 60), new BoundingRect(150, 163), 
             new AtaruEye(23, false), new BoundingRect(93, 90),
             new AtaruEye(23, true),  new BoundingRect(93, 230),
             new AtaruEyeblow(20, 3, false), new BoundingRect(55, 90),
             new AtaruEyeblow(20, 3, true),  new BoundingRect(55, 230)) {}
  AtaruFace(LGFX_Device* device)
      : Face(new AtaruMouth(50, 100, 4, 60), new BoundingRect(150, 163), 
             new AtaruEye(23, false), new BoundingRect(93, 90),
             new AtaruEye(23, true),  new BoundingRect(93, 230),
             new AtaruEyeblow(20, 3, false), new BoundingRect(55, 90),
             new AtaruEyeblow(20, 3, true),  new BoundingRect(55, 230), device) {}
};

}  // namespace m5avatar

#endif  // FACES_ATARUFACE_H_
