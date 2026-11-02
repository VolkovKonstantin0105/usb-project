#include "PcapStreamParser.h"
#include "UsbPcapProtocol.h"
#include <cstring>

bool PcapStreamParser::Feed(const uint8_t* p, size_t n, const Callback& cb) {
    if (failed_) return false;
    buf_.insert(buf_.end(), p, p + n);

    size_t pos = 0;
    if (!gotGlobal_) {
        if (buf_.size() < sizeof(PcapGlobalHeader)) return true;
        PcapGlobalHeader gh;
        std::memcpy(&gh, buf_.data(), sizeof(gh));
        if (gh.magic != kPcapMagic || gh.network != kLinkTypeUsbPcap) {
            failed_ = true;
            return false;
        }
        gotGlobal_ = true;
        pos = sizeof(gh);
    }

    while (buf_.size() - pos >= sizeof(PcapRecordHeader)) {
        PcapRecordHeader rh;
        std::memcpy(&rh, &buf_[pos], sizeof(rh));
        size_t total = sizeof(rh) + rh.inclLen;
        if (buf_.size() - pos < total) break;        // запись пришла не целиком

        const uint8_t* payload = &buf_[pos + sizeof(rh)];
        if (rh.inclLen >= sizeof(UsbPcapPacketHeader)) {
            UsbPcapPacketHeader h;
            std::memcpy(&h, payload, sizeof(h));
            if (h.headerLen >= sizeof(h) && h.headerLen <= rh.inclLen) {
                CapturedPacket pkt;
                pkt.tsUs = static_cast<uint64_t>(rh.tsSec) * 1000000ull + rh.tsUsec;
                pkt.irpId = h.irpId;
                pkt.status = h.status;
                pkt.dataLength = h.dataLength;
                pkt.function = h.function;
                pkt.bus = h.bus;
                pkt.device = h.device;
                pkt.info = h.info;
                pkt.endpoint = h.endpoint;
                pkt.transfer = h.transfer;
                if (h.transfer == 2 && h.headerLen > sizeof(h))
                    pkt.stage = payload[sizeof(h)];
                pkt.data.assign(payload + h.headerLen, payload + rh.inclLen);
                cb(std::move(pkt));
            }
        }
        pos += total;
    }
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<ptrdiff_t>(pos));
    return true;
}
