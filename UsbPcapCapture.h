#pragma once
#include "common.h"
#include "PcapStreamParser.h"
#include <atomic>
#include <mutex>
#include <thread>

class UsbPcapCapture {
public:
    explicit UsbPcapCapture(HWND notifyWnd) : wnd_(notifyWnd) {}
    ~UsbPcapCapture() { Stop(); }

    // Номера найденных интерфейсов: 1 для \\.\USBPcap1 и т.д.
    static std::vector<int> FindInterfaces();

    bool Start(int interfaceIndex, const std::wstring& pcapPath, std::wstring& err);
    void Stop();
    bool IsRunning() const { return running_; }
    DWORD LastError() const { return lastError_; }

    // Забрать накопленные пакеты (вызывать из потока интерфейса)
    void Drain(std::vector<CapturedPacket>& out);

private:
    void ThreadMain();

    HWND   wnd_;
    HANDLE h_ = INVALID_HANDLE_VALUE;
    HANDLE stopEvent_ = nullptr;
    HANDLE pcapFile_ = INVALID_HANDLE_VALUE;
    std::thread thread_;
    std::atomic<bool> running_{ false };
    std::atomic<bool> notified_{ false };
    std::atomic<DWORD> lastError_{ 0 };
    std::mutex mtx_;
    std::vector<CapturedPacket> pending_;
};
