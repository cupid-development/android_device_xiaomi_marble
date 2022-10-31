#
# Copyright (C) 2022-2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from xiaomi sm8450-common
$(call inherit-product, device/xiaomi/sm8450-common/common.mk)

# Inherit from the proprietary version
$(call inherit-product, vendor/xiaomi/marble/marble-vendor.mk)

# Overlay
PRODUCT_PACKAGES += \
    ApertureResMarble \
    FrameworksResMarble \
    SettingsProviderResMarble \
    SettingsResMarble \
    SystemUIResMarble \
    WifiResMarble

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)
