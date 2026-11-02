#include "UsbPcapCapture.h"
#include "UsbPcapProtocol.h"

// Синхронный DeviceIoControl для дескриптора, открытого с FILE_FLAG_OVERLAPPED
static bool Ioctl(HANDLE h, DWORD code, void* in, DWORD inLen) {
    OVERLAPPED ov{};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(h, code, in, inLen, nullptr, 0, &ret, &ov);
    if (!ok && GetLastError() == ERROR_IO_PENDING)
        ok = GetOverlappedResult(h, &ov, &ret, TRUE);
    CloseHandle(ov.hEvent);
    return ok != FALSE;
}

std::vector<int> UsbPcapCapture::FindInterfaces() {
    std::vector<int> res;
    for (int i = 1; i <= 32; ++i) {
        std::wstring path = L"\\\\.\\USBPcap" + std::to_wstring(i);
        HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            res.push_back(i);
        }
        else {
            DWORD e = GetLastError();
            // "не найдено" = такого интерфейса нет; остальное (нет прав,
            // занят другой программой) = интерфейс есть
            if (e != ERROR_FILE_NOT_FOUND && e != ERROR_PATH_NOT_FOUND)
                res.push_back(i);
        }
    }
    return res;
}

bool UsbPcapCapture::Start(int idx, const std::wstring& pcapPath, std::wstring& err) {
    if (running_) return true;

    auto fail = [&](const wchar_t* what) {
        err = std::wstring(what) + L" (код ошибки " +
            std::to_wstring(GetLastError()) + L")";
        Stop();
        return false;
        };

    std::wstring path = L"\\\\.\\USBPcap" + std::to_wstring(idx);
    h_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h_ == INVALID_HANDLE_VALUE)
        return fail(L"Не удалось открыть устройство USBPcap. Нужны права "
            L"администратора и установленный драйвер");

    uint32_t bufLen = 1024 * 1024;
    if (!Ioctl(h_, IOCTL_USBPCAP_SETUP_BUFFER, &bufLen, sizeof(bufLen)))
        return fail(L"SETUP_BUFFER");

    uint32_t snap = 65535;
    if (!Ioctl(h_, IOCTL_USBPCAP_SET_SNAPLEN_SIZE, &snap, sizeof(snap)))
        return fail(L"SET_SNAPLEN_SIZE");

    UsbPcapAddressFilter f{};
    f.filterAll = 1;
    if (!Ioctl(h_, IOCTL_USBPCAP_START_FILTERING, &f, sizeof(f)))
        return fail(L"START_FILTERING");

    pcapFile_ = CreateFileW(pcapPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pcapFile_ == INVALID_HANDLE_VALUE)
        return fail(L"Не удалось создать .pcap файл");

    stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    lastError_ = 0;
    running_ = true;
    thread_ = std::thread(&UsbPcapCapture::ThreadMain, this);
    return true;
}

void UsbPcapCapture::ThreadMain() {
    std::vector<uint8_t> buf(256 * 1024);
    PcapStreamParser parser;
    OVERLAPPED ov{};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    auto onPacket = [this](CapturedPacket&& p) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (pending_.size() < 1000000) pending_.push_back(std::move(p));
        }
        // будим интерфейс, только если он еще не получил уведомление
        if (!notified_.exchange(true))
            PostMessageW(wnd_, WM_APP_PACKETS, 0, 0);
        };

    for (;;) {
        ResetEvent(ov.hEvent);
        DWORD got = 0;
        BOOL ok = ReadFile(h_, buf.data(), static_cast<DWORD>(buf.size()), &got, &ov);
        if (!ok) {
            DWORD e = GetLastError();
            if (e != ERROR_IO_PENDING) { lastError_ = e; break; }

            HANDLE waits[2] = { ov.hEvent, stopEvent_ };
            DWORD w = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (w != WAIT_OBJECT_0) {              // просили остановиться
                CancelIoEx(h_, &ov);
                GetOverlappedResult(h_, &ov, &got, TRUE);
                break;
            }
            if (!GetOverlappedResult(h_, &ov, &got, FALSE)) {
                lastError_ = GetLastError();
                break;
            }
        }
        if (got == 0) continue;

        DWORD written = 0;
        WriteFile(pcapFile_, buf.data(), got, &written, nullptr);  // сырой pcap на диск

        if (!parser.Feed(buf.data(), got, onPacket)) {
            lastError_ = ERROR_INVALID_DATA;       // поток не похож на pcap USBPcap
            break;
        }
    }

    CloseHandle(ov.hEvent);
    running_ = false;
    PostMessageW(wnd_, WM_APP_CAPSTATE, 0, static_cast<LPARAM>(lastError_.load()));
}

void UsbPcapCapture::Stop() {
    if (stopEvent_) SetEvent(stopEvent_);
    if (thread_.joinable()) thread_.join();

    if (h_ != INVALID_HANDLE_VALUE) {
        Ioctl(h_, IOCTL_USBPCAP_STOP_FILTERING, nullptr, 0);
        CloseHandle(h_);
        h_ = INVALID_HANDLE_VALUE;
    }
    if (stopEvent_) { CloseHandle(stopEvent_); stopEvent_ = nullptr; }
    if (pcapFile_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pcapFile_);
        pcapFile_ = INVALID_HANDLE_VALUE;
    }
    running_ = false;
}

void UsbPcapCapture::Drain(std::vector<CapturedPacket>& out) {
    std::lock_guard<std::mutex> lk(mtx_);
    out.swap(pending_);
    pending_.clear();
    notified_ = false;       // следующий пакет снова разбудит интерфейс
}
