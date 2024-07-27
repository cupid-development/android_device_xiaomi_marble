/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LightNotifier.h"

void activateAdditionalNotifiers(sp<ISensorManager> manager) {
    std::unique_ptr<LightNotifier> lightNotifier = std::make_unique<LightNotifier>(manager);
    lightNotifier->activate();
}
