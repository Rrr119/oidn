// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "core/temporal_accum.h"
#include "cpu_engine.h"

OIDN_NAMESPACE_BEGIN

  class CPUTemporalAccum final : public TemporalAccum
  {
  public:
    explicit CPUTemporalAccum(CPUEngine* engine);

    Engine* getEngine() const override { return engine; }
    void submitKernels(const Ref<CancellationToken>& ct) override;

  private:
    CPUEngine* engine;
  };

OIDN_NAMESPACE_END
