// Copyright (c) robo8080. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.
#ifndef FACES_DANNFACE_H_
#define FACES_DANNFACE_H_

#include <M5Unified.h>
#include "DannEye.h"
#include "DannMouth.h"
#include "DannEyeblow.h"

namespace m5avatar {
class DannFace : public Face {
 public:
  DannFace()
      : Face(new DannMouth(50, 100, 4, 65), new BoundingRect(160, 163),
             new DannEye(23, false), new BoundingRect(93, 90),
             new DannEye(23, true),  new BoundingRect(93, 230),
             new DannEyeblow(50, 6, false), new BoundingRect(55, 90),
             new DannEyeblow(50, 6, true), new BoundingRect(55, 230)) {}
  DannFace(LGFX_Device* device)
      : Face(new DannMouth(50, 100, 4, 65), new BoundingRect(160, 163),
             new DannEye(23, false), new BoundingRect(93, 90),
             new DannEye(23, true),  new BoundingRect(93, 230),
             new DannEyeblow(50, 6, false), new BoundingRect(55, 90),
             new DannEyeblow(50, 6, true), new BoundingRect(55, 230), device) {}
             
};

}  // namespace m5avatar

#endif  // FACES_DANNFACE_H_
