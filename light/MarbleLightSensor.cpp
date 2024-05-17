/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "MarbleLightSensor"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/looper.h>
#include <android/sensor.h>
#include <utils/Errors.h>

#include <fcntl.h>
#include <poll.h>
#include <string>

// #define SENSOR_TYPE_LIGHT 5
#define SENSOR_TYPE_GS1602_CCT_STREAM 33171089
#define SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW 33171111
#define SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_FACTORY 33171007

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

extern void process_msg(_oem_msg* msg);
extern void init_current_sensors(bool debug);

using ::android::NO_ERROR;

namespace {

struct SensorContext {
    ASensorEventQueue *queue;
};

int RawSensorCallback(__attribute__((unused)) int fd, __attribute__((unused)) int events,
                    void *data) {
    ASensorEvent event;
    int event_count = 0;
    SensorContext *context = reinterpret_cast<SensorContext *>(data);
    event_count = ASensorEventQueue_getEvents(context->queue, &event, 1);

    LOG(INFO) << "sensor reported: 0:" << event.data[0] << ", 1: " << event.data[1] << ", 2: " << event.data[2] << ", 3: " << event.data[3];

    _oem_msg* msg = new _oem_msg;
    msg->notify_type = REPORT_VALUE;
    msg->value = event.data[1];
    msg->notify_type_float = msg->notify_type;
    msg->unknown1 = 2;
    msg->unknown2 = 5;
    msg->sensor_type = SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_RAW;
    process_msg(msg);

    return 1;
}

int32_t PollRawSensor() {
    int err = NO_ERROR, counter = 0;
    ASensorManager *sensorManager = nullptr;
    ASensorRef RawAlsSensor;
    ALooper *looper;
    struct SensorContext context = {nullptr};

    sensorManager = ASensorManager_getInstanceForPackage("");
    if (!sensorManager) {
        LOG(ERROR) << "Failed to get ASensorManager instance";
        return 0;
    }

    RawAlsSensor = ASensorManager_getDefaultSensor(sensorManager, SENSOR_TYPE_XIAOMI_SENSOR_AMBIENTLIGHT_FACTORY);
    if (RawAlsSensor == nullptr) {
        LOG(ERROR) << "Failed to get raw als sensor";
    } else {
        looper = ALooper_forThread();
        if (looper == nullptr) {
            looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        }
        context.queue =
            ASensorManager_createEventQueue(sensorManager, looper, 0, RawSensorCallback, &context);

        err = ASensorEventQueue_registerSensor(context.queue, RawAlsSensor, 0, 0);
        if (err != NO_ERROR) {
            LOG(ERROR) << "Failed to register sensor with event queue, error: " << err;
            return 0;
        } else {
            while (true) {
                ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
            }
        }
    }

    if (sensorManager != nullptr && context.queue != nullptr) {
        ASensorEventQueue_disableSensor(context.queue, RawAlsSensor);
        ASensorManager_destroyEventQueue(sensorManager, context.queue);
    }

    return 0;
}

}  // namespace

int main() {
    bool debug_enable = android::base::GetBoolProperty("persist.vendor.debug.ssccalapi", false);
    init_current_sensors(debug_enable);

    PollRawSensor();

    // Should never reach this
    return EXIT_SUCCESS;
}
