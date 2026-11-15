// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "cpu_temporal_accum.h"
#include "tasking.h"
#include <cmath>

OIDN_NAMESPACE_BEGIN

  namespace
  {
    // Bilinearly samples an image at a fractional position, clamped to the image bounds
    vec3<float> sampleBilinear(const ImageAccessor& image, float h, float w)
    {
      h = clamp(h, 0.f, float(image.H - 1));
      w = clamp(w, 0.f, float(image.W - 1));

      const int h0 = int(h);
      const int w0 = int(w);
      const int h1 = min(h0 + 1, image.H - 1);
      const int w1 = min(w0 + 1, image.W - 1);
      const float fh = h - float(h0);
      const float fw = w - float(w0);

      const vec3<float> v00 = image.get3(h0, w0);
      const vec3<float> v01 = image.get3(h0, w1);
      const vec3<float> v10 = image.get3(h1, w0);
      const vec3<float> v11 = image.get3(h1, w1);

      return (v00 * (1.f - fw) + v01 * fw) * (1.f - fh) +
             (v10 * (1.f - fw) + v11 * fw) * fh;
    }

    // Returns the maximum relative per-channel difference between two colors
    float relDiff(const vec3<float>& a, const vec3<float>& b)
    {
      const float d = max(max(std::fabs(a.x - b.x), std::fabs(a.y - b.y)), std::fabs(a.z - b.z));
      const float m = max(max(max(std::fabs(a.x), std::fabs(a.y)), std::fabs(a.z)), 1e-3f);
      return d / m;
    }
  }

  CPUTemporalAccum::CPUTemporalAccum(CPUEngine* engine)
    : engine(engine)
  {}

  void CPUTemporalAccum::submitKernels(const Ref<CancellationToken>& ct)
  {
    check();

    ImageAccessor curAcc    = *cur;
    ImageAccessor prevAcc   = *prev;
    ImageAccessor motionAcc = *motion;
    ImageAccessor dstAcc    = *dst;

    ImageAccessor albedoAcc, prevAlbedoAcc, normalAcc, prevNormalAcc;
    const bool hasAlbedo = albedo && prevAlbedo;
    const bool hasNormal = normal && prevNormal;
    if (hasAlbedo)
    {
      albedoAcc     = *albedo;
      prevAlbedoAcc = *prevAlbedo;
    }
    if (hasNormal)
    {
      normalAcc     = *normal;
      prevNormalAcc = *prevNormal;
    }

    const float maxBlend = blend;

    engine->submitFunc([=]
    {
      parallel_for(dstAcc.H, [&](int h)
      {
        for (int w = 0; w < dstAcc.W; ++w)
        {
          const vec3<float> curColor = curAcc.get3(h, w);
          const vec3<float> mv = motionAcc.get3(h, w); // (x, y) for 2 channels

          // Reproject the pixel onto the previous frame
          const float prevH = h + mv.y;
          const float prevW = w + mv.x;

          float weight = maxBlend;
          vec3<float> history(0.f);

          // Reject history reprojected outside the previous frame
          if (prevH < 0.f || prevH > float(prevAcc.H - 1) ||
              prevW < 0.f || prevW > float(prevAcc.W - 1))
          {
            weight = 0.f;
          }
          else
          {
            history = sampleBilinear(prevAcc, prevH, prevW);

            // Fast motion: long motion vectors are less reliable, so the weight of the
            // history is gradually reduced as the motion length grows
            const float motionLength = std::sqrt(mv.x * mv.x + mv.y * mv.y);
            weight *= 1.f / (1.f + 0.1f * motionLength);

            // Disocclusion detection: if the auxiliary features of the reprojected
            // history do not match the current frame, the pixel was likely occluded
            // in the previous frame and the history must be rejected
            if (hasAlbedo &&
                relDiff(albedoAcc.get3(h, w), sampleBilinear(prevAlbedoAcc, prevH, prevW)) > 0.25f)
              weight = 0.f;
            if (hasNormal &&
                relDiff(normalAcc.get3(h, w), sampleBilinear(prevNormalAcc, prevH, prevW)) > 0.5f)
              weight = 0.f;

            // Color consistency: soften the blend where the history disagrees with the
            // current frame to avoid ghosting from stale shading
            if (weight > 0.f)
              weight *= 1.f / (1.f + relDiff(curColor, history));
          }

          dstAcc.set3(h, w, curColor * (1.f - weight) + history * weight);
        }
      });
    }, ct);
  }

OIDN_NAMESPACE_END
