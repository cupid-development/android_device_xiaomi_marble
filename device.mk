#
# Copyright (C) 2022-2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from xiaomi sm8450-common
$(call inherit-product, device/xiaomi/sm8450-common/common.mk)

# Inherit from the proprietary version
$(call inherit-product, vendor/xiaomi/marble/marble-vendor.mk)

# Audio
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/mixer_paths_overlay_static.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_ukee/mixer_paths_overlay_static.xml \
    $(LOCAL_PATH)/audio/resourcemanager_ukee_mtp.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_ukee/resourcemanager_ukee_mtp.xml

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
