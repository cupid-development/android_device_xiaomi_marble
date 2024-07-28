/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "RawLightNotifier"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <display/drm/mi_disp.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "RawLightNotifier.h"
#include "SensorNotifierUtils.h"

#define DISP_FEATURE_PATH "/dev/mi_display/disp_feature"

#define SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW 33171111

using android::hardware::Return;
using android::hardware::Void;
using android::hardware::sensors::V1_0::Event;

namespace {

class RawLightSensorCallback : public IEventQueueCallback {
  public:
    RawLightSensorCallback(process_msg_t processMsg) : mProcessMsg(processMsg) {}

    Return<void> onEvent(const Event& e) {
        _oem_msg* msg = new _oem_msg;
        msg->notify_type = REPORT_VALUE;
        msg->value = e.u.scalar;
        msg->notify_type_float = msg->notify_type;
        msg->unknown1 = 2;
        msg->unknown2 = 5;
        msg->sensor_type = SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW;
        if (mProcessMsg == NULL) {
            mProcessMsg(msg);
        }

        return Void();
    }

  private:
    process_msg_t mProcessMsg;
};

}  // namespace

RawLightNotifier::RawLightNotifier(sp<ISensorManager> manager, process_msg_t processMsg)
    : SensorNotifier(manager, processMsg) {
    initializeSensorQueue("xiaomi.sensor.ambientlight.factory", false,
                          new RawLightSensorCallback(processMsg));
}

RawLightNotifier::~RawLightNotifier() {
    deactivate();
}

void RawLightNotifier::pollingFunction() {
    Result res;

    android::base::unique_fd disp_fd_ = android::base::unique_fd(open(DISP_FEATURE_PATH, O_RDWR));
    if (disp_fd_.get() == -1) {
        LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH;
    }

    // Enable the sensor initially
    res = mQueue->enableSensor(mSensorHandle, 20000 /* sample period */, 0 /* latency */);
    if (res != Result::OK) {
        LOG(ERROR) << "failed to enable sensor";
    }

    // Register for power events
    disp_event_req req;
    req.base.flag = 0;
    req.base.disp_id = MI_DISP_PRIMARY;
    req.type = MI_DISP_EVENT_POWER;
    ioctl(disp_fd_.get(), MI_DISP_IOCTL_REGISTER_EVENT, &req);

    struct pollfd dispEventPoll = {
            .fd = disp_fd_.get(),
            .events = POLLIN,
    };

    while (mActive) {
        int rc = poll(&dispEventPoll, 1, -1);
        if (rc < 0) {
            LOG(ERROR) << "failed to poll " << DISP_FEATURE_PATH << ", err: " << rc;
            continue;
        }

        struct disp_event_resp* response = parseDispEvent(disp_fd_.get());
        if (response == nullptr) {
            continue;
        }

        if (response->base.type != MI_DISP_EVENT_POWER) {
            LOG(ERROR) << "unexpected display event: " << response->base.type;
            continue;
        }

        int value = response->data[0];
        LOG(VERBOSE) << "received data: " << std::bitset<8>(value);

        switch (response->data[0]) {
            case MI_DISP_POWER_ON:
                res = mQueue->enableSensor(mSensorHandle, 20000 /* sample period */,
                                           0 /* latency */);
                if (res != Result::OK) {
                    LOG(ERROR) << "failed to enable sensor";
                }
                break;
            default:
                res = mQueue->disableSensor(mSensorHandle);
                if (res != Result::OK) {
                    LOG(ERROR) << "failed to disable sensor";
                }
                break;
        }
    }
}
