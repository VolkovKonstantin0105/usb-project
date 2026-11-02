#include "UsbDeviceList.h"
#include <setupapi.h>
#include <algorithm>
#include <cstdlib>
#include <cwchar>

#pragma comment(lib, "setupapi.lib")

static std::wstring GetProp(HDEVINFO set, SP_DEVINFO_DATA& d, DWORD prop) {
    DWORD type = 0, size = 0;
    SetupDiGetDeviceRegistryPropertyW(set, &d, prop, &type, nullptr, 0, &size);
    if (size == 0) return L"";
    std::vector<BYTE> buf(size + sizeof(wchar_t), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(set, &d, prop, &type,
        buf.data(), size, nullptr))
        return L"";
    return std::wstring(reinterpret_cast<wchar_t*>(buf.data()));
}

// Достает число после метки, например VID_ -> 046D
static uint16_t ParseHexAfter(const std::wstring& s, const wchar_t* tag) {
    size_t pos = s.find(tag);
    if (pos == std::wstring::npos)
        return 0;
    return static_cast<uint16_t>(
        wcstoul(s.c_str() + pos + wcslen(tag), nullptr, 16));
}

std::vector<UsbDeviceInfo> EnumerateUsbDevices() {
    std::vector<UsbDeviceInfo> result;

    HDEVINFO set = SetupDiGetClassDevsW(nullptr, L"USB", nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (set == INVALID_HANDLE_VALUE) return result;

    SP_DEVINFO_DATA d{};
    d.cbSize = sizeof(d);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &d); ++i) {
        wchar_t id[512] = {};
        if (!SetupDiGetDeviceInstanceIdW(set, &d, id, 512, nullptr))
            continue;

        UsbDeviceInfo info;
        info.instanceId = id;
        info.description = GetProp(set, d, SPDRP_DEVICEDESC);
        info.manufacturer = GetProp(set, d, SPDRP_MFG);
        info.deviceClass = GetProp(set, d, SPDRP_CLASS);
        info.vid = ParseHexAfter(info.instanceId, L"VID_");
        info.pid = ParseHexAfter(info.instanceId, L"PID_");
        result.push_back(std::move(info));
    }
    SetupDiDestroyDeviceInfoList(set);

    std::sort(result.begin(), result.end(),
        [](const UsbDeviceInfo& a, const UsbDeviceInfo& b) {
            return a.instanceId < b.instanceId;
        });
    return result;
}
