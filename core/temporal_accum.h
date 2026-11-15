// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "op.h"
#include "image.h"

OIDN_NAMESPACE_BEGIN

  // Temporal accumulation: blends the current denoised frame with the reprojected
  // previous output (history) using motion vectors. Invalid history samples
  // (disocclusions, fast motion, color outliers) are rejected to avoid ghosting
  // and flickering.
  class TemporalAccum : public BaseOp
  {
  public:
    void setCur(const Ref<Image>& cur) { this->cur = cur; }
    void setPrev(const Ref<Image>& prev) { this->prev = prev; }
    void setMotion(const Ref<Image>& motion) { this->motion = motion; }
    void setAlbedo(const Ref<Image>& albedo) { this->albedo = albedo; }
    void setPrevAlbedo(const Ref<Image>& prevAlbedo) { this->prevAlbedo = prevAlbedo; }
    void setNormal(const Ref<Image>& normal) { this->normal = normal; }
    void setPrevNormal(const Ref<Image>& prevNormal) { this->prevNormal = prevNormal; }
    void setDst(const Ref<Image>& dst) { this->dst = dst; }
    void setBlend(float blend) { this->blend = blend; }

  protected:
    void check()
    {
      if (!cur || !prev || !motion || !dst)
        throw std::logic_error("temporal accumulation source/destination not set");
      if (prev->getH() != cur->getH() || prev->getW() != cur->getW() ||
          motion->getH() != cur->getH() || motion->getW() != cur->getW() ||
          dst->getH() != cur->getH() || dst->getW() != cur->getW())
        throw std::logic_error("temporal accumulation image sizes do not match");
      if (motion->getC() != 2)
        throw std::logic_error("temporal accumulation motion vectors must have 2 channels");
    }

    Ref<Image> cur;        // current denoised frame
    Ref<Image> prev;       // previous accumulated output (history)
    Ref<Image> motion;     // motion vectors from the current to the previous frame (in pixels)
    Ref<Image> albedo;     // current frame albedo (optional, for disocclusion detection)
    Ref<Image> prevAlbedo; // previous frame albedo (optional, for disocclusion detection)
    Ref<Image> normal;     // current frame normal (optional, for disocclusion detection)
    Ref<Image> prevNormal; // previous frame normal (optional, for disocclusion detection)
    Ref<Image> dst;        // accumulated output
    float blend = 0.8f;    // maximum weight of the history in [0, 1]
  };

OIDN_NAMESPACE_END
