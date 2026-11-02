#pragma once
#include "common.h"

struct UsbDeviceInfo {
    std::wstring instanceId;    // например USB\VID_046D&PID_C52B\5&2a...
    std::wstring description;
    std::wstring manufacturer;
    std::wstring deviceClass;   // HIDClass, USB, DiskDrive и т.д.
    uint16_t vid = 0;
    uint16_t pid = 0;
};

std::vector<UsbDeviceInfo> EnumerateUsbDevices();
