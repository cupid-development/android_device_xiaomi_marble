/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "SensorNotifier.h"

class LightNotifier : public SensorNotifier {
  public:
    LightNotifier(sp<ISensorManager> manager);
    ~LightNotifier();

  protected:
    void pollingFunction();
};
