/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "LightNotifier"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <display/drm/mi_disp.h>
#include <dlfcn.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "LightNotifier.h"

#define DISP_FEATURE_PATH "/dev/mi_display/disp_feature"

#define SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW 33171111

enum notify_t {
    BRIGHTNESS = 17,
    DC_STATE = 18,
    DISPLAY_FREQUENCY = 20,
    REPORT_VALUE = 201,
    POWER_STATE = 202,
};

struct _oem_msg {
    uint32_t sensor_type;
    notify_t notify_type;
    float unknown1;
    float unknown2;
    float notify_type_float;
    float value;
    float unused0;
    float unused1;
    float unused2;
};

typedef void (*process_msg_t)(_oem_msg* msg);
typedef void (*init_current_sensors_t)(bool debug);

using android::hardware::Return;
using android::hardware::Void;
using android::hardware::sensors::V1_0::Event;

namespace {

static disp_event_resp* parseDispEvent(int fd) {
    disp_event header;
    ssize_t headerSize = read(fd, &header, sizeof(header));
    if (headerSize < sizeof(header)) {
        LOG(ERROR) << "unexpected display event header size: " << headerSize;
        return nullptr;
    }

    struct disp_event_resp* response =
            reinterpret_cast<struct disp_event_resp*>(malloc(header.length));
    response->base = header;

    int dataLength = response->base.length - sizeof(response->base);
    if (dataLength < 0) {
        LOG(ERROR) << "invalid data length: " << response->base.length;
        return nullptr;
    }

    ssize_t dataSize = read(fd, &response->data, dataLength);
    if (dataSize < dataLength) {
        LOG(ERROR) << "unexpected display event data size: " << dataSize;
        return nullptr;
    }

    return response;
}

class LightSensorCallback : public IEventQueueCallback {
  public:
    LightSensorCallback() {
        disp_fd_ = android::base::unique_fd(open(DISP_FEATURE_PATH, O_RDWR));
        if (disp_fd_.get() == -1) {
            LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH;
        }

        ssccalapiHandle = dlopen("libssccalapi@2.0.so", RTLD_NOW);
        if (ssccalapiHandle) {
            init_current_sensors =
                    (init_current_sensors_t)dlsym(ssccalapiHandle, "_Z20init_current_sensorsb");
            process_msg = (process_msg_t)dlsym(ssccalapiHandle, "_Z11process_msgP8_oem_msg");

            if (init_current_sensors != NULL) {
                init_current_sensors(false);
            }
        } else {
            LOG(ERROR) << "could not dlopen libssccalapi@2.0.so";
        }
    }

    ~LightSensorCallback() { dlclose(ssccalapiHandle); }

    Return<void> onEvent(const Event& e) {
        _oem_msg* msg = new _oem_msg;
        msg->notify_type = REPORT_VALUE;
        msg->value = e.u.scalar;
        msg->notify_type_float = msg->notify_type;
        msg->unknown1 = 2;
        msg->unknown2 = 5;
        msg->sensor_type = SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW;
        if (process_msg == NULL) {
            process_msg(msg);
        }

        return Void();
    }

  private:
    android::base::unique_fd disp_fd_;

    void* ssccalapiHandle;
    init_current_sensors_t init_current_sensors;
    process_msg_t process_msg;
};

}  // namespace

LightNotifier::LightNotifier(sp<ISensorManager> manager) : SensorNotifier(manager) {
    initializeSensorQueue("xiaomi.sensor.ambientlight.factory", false, new LightSensorCallback());
}

LightNotifier::~LightNotifier() {
    deactivate();
}

void LightNotifier::pollingFunction() {
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
            .revents = 0,
    };

    while (true) {
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
