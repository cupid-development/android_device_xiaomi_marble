#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

function vendor_imports() {
    cat << EOF >> "$1"
		"device/xiaomi/sm8450-common",
		"hardware/qcom-caf/sm8450",
		"hardware/xiaomi",
		"vendor/qcom/opensource/commonsys-intf/display",
EOF
}

function lib_to_package_fixup_vendor_variants() {
    if [ "$2" != "vendor" ]; then
        return 1
    fi

    case "$1" in
        libagmclient | \
        libagmmixer | \
        vendor.qti.hardware.pal@1.0-impl)
            ;;
        *)
            return 1
    esac
}

function lib_to_package_fixup() {
    lib_to_package_fixup_clang_rt_ubsan_standalone "$1" || \
    lib_to_package_fixup_proto_3_9_1 "$1" || \
    lib_to_package_fixup_vendor_variants "$@"
}

# If we're being sourced by the common script that we called,
# stop right here. No need to go down the rabbit hole.
if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    return
fi

set -e

export DEVICE=marble
export DEVICE_COMMON=sm8450-common
export VENDOR=xiaomi

"./../../${VENDOR}/${DEVICE_COMMON}/setup-makefiles.sh" "$@"
