/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SensorNotifierExt.h"

#include "RawLightNotifier.h"

SensorNotifierExt::SensorNotifierExt(sp<ISensorManager> manager) {
    mNotifiers.push_back(std::make_unique<RawLightNotifier>(manager));
}
