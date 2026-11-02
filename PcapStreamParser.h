#pragma once
#include "common.h"
#include <functional>

struct CapturedPacket {
    uint64_t tsUs = 0;          // время в микросекундах с 1970 года
    uint64_t irpId = 0;
    uint32_t status = 0;
    uint32_t dataLength = 0;
    uint16_t function = 0;
    uint16_t bus = 0;
    uint16_t device = 0;
    uint8_t  info = 0;
    uint8_t  endpoint = 0;
    uint8_t  transfer = 0;
    int      stage = -1;        // CONTROL: 0 setup, 1 data, 2 status, 3 complete
    std::vector<uint8_t> data;  // полезные данные
};

class PcapStreamParser {
public:
    using Callback = std::function<void(CapturedPacket&&)>;
    // Скармливаем кусочки потока. false = неверный формат потока.
    bool Feed(const uint8_t* p, size_t n, const Callback& cb);
private:
    std::vector<uint8_t> buf_;
    bool gotGlobal_ = false;
    bool failed_ = false;
};
