/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SensorNotifierExt.h"
#include "RawLightNotifier.h"

SensorNotifierExt::SensorNotifierExt(sp<ISensorManager> manager, process_msg_t processMsg) {
    std::unique_ptr<RawLightNotifier> lightNotifier =
            std::make_unique<RawLightNotifier>(manager, processMsg);
    mNotifiers.push_back(std::move(lightNotifier));
}
