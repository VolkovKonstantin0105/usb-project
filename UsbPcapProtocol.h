#pragma once
#include "common.h"
#include <winioctl.h>

// Коды управления драйвером USBPcap (СВЕРИТЬ с USBPcap.h!)
#define IOCTL_USBPCAP_SETUP_BUFFER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USBPCAP_START_FILTERING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USBPCAP_STOP_FILTERING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_USBPCAP_SET_SNAPLEN_SIZE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)

#pragma pack(push, 1)

// Фильтр адресов для START_FILTERING
struct UsbPcapAddressFilter {
    uint32_t addresses[4];   // битовая маска адресов 0..127
    uint8_t  filterAll;      // 1 = пропускать все устройства
};

// Заголовок пакета USBPcap (идет сразу после заголовка записи pcap)
struct UsbPcapPacketHeader {
    uint16_t headerLen;      // длина заголовка (27, а для CONTROL 28)
    uint64_t irpId;          // идентификатор запроса
    uint32_t status;         // код USBD_STATUS
    uint16_t function;       // код функции URB
    uint8_t  info;           // бит 0: 1 = ответ (устройство -> хост)
    uint16_t bus;            // номер шины
    uint16_t device;         // адрес устройства
    uint8_t  endpoint;       // бит 7 = IN, остальное номер точки
    uint8_t  transfer;       // 0 isoch, 1 interrupt, 2 control, 3 bulk, 254 irp info
    uint32_t dataLength;     // длина данных после заголовка
};

struct PcapGlobalHeader {
    uint32_t magic;          // 0xA1B2C3D4
    uint16_t versionMajor;
    uint16_t versionMinor;
    int32_t  thisZone;
    uint32_t sigFigs;
    uint32_t snapLen;
    uint32_t network;        // 249 для USBPcap
};

struct PcapRecordHeader {
    uint32_t tsSec;
    uint32_t tsUsec;
    uint32_t inclLen;        // сколько байт записи в файле
    uint32_t origLen;
};

#pragma pack(pop)

static_assert(sizeof(UsbPcapPacketHeader) == 27, "UsbPcapPacketHeader size");
static_assert(sizeof(PcapGlobalHeader) == 24, "PcapGlobalHeader size");
static_assert(sizeof(PcapRecordHeader) == 16, "PcapRecordHeader size");

constexpr uint32_t kPcapMagic = 0xA1B2C3D4;
constexpr uint32_t kLinkTypeUsbPcap = 249;

