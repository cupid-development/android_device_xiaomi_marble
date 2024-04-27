#include <dlfcn.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>

#define LOG_TAG "libssccalapi_shim"

#include <android-base/logging.h>

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

extern "C" void _Z11process_msgP8_oem_msg(void* msg) {
    static auto process_msg_orig = reinterpret_cast<typeof(_Z11process_msgP8_oem_msg)*>(
            dlsym(RTLD_NEXT, "_Z11process_msgP8_oem_msg"));

    struct _oem_msg* receivedMsg = static_cast<struct _oem_msg*>(msg);
    LOG(ERROR) << "sending oem_msg for sensor " << receivedMsg->sensor_type
               << " with notify_type: " << receivedMsg->notify_type << " and value: " << receivedMsg->value;
    LOG(ERROR) << "sending oem_msg unknown1: " << receivedMsg->unknown1
               << ", unknown2: " << receivedMsg->unknown2
               << ", notify_type_float: " << receivedMsg->notify_type_float
               << ", unused0: " << receivedMsg->unused0
               << ", unused2: " << receivedMsg->unused1
               << ", unused2: " << receivedMsg->unused2;

    receivedMsg->unknown1 = 1;

    process_msg_orig(msg);
}

extern "C" void _Z20init_current_sensorsb(bool init) {
    static auto init_current_sensors_orig = reinterpret_cast<typeof(_Z20init_current_sensorsb)*>(
        dlsym(RTLD_NEXT, "_Z20init_current_sensorsb"));

    LOG(ERROR) << "init_current_sensors, init: " << init;

    init_current_sensors_orig(init);
}