#include "MainWindow.h"
#include <dbt.h>

HDEVNOTIFY g_hNotify = nullptr; //здесь будет информация об устроствах подключенных

static const GUID kUsbDeviceGuid =
{
    0xA5DCBF10, 0x6530, 0x11D2,
    {
        0x90,
        0x1F,
        0x00,
        0xC0,
        0x4F,
        0xB9,
        0x51,
        0xED
    }
};

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m)
    {
    case WM_DESTROY:
        if (g_hNotify) {
            UnregisterDeviceNotification(g_hNotify);
            g_hNotify = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    case WM_DEVICECHANGE:
        if (w == DBT_DEVICEARRIVAL || w == DBT_DEVICEREMOVECOMPLETE) {
            auto* hdr = reinterpret_cast<PDEV_BROADCAST_HDR>(l);
            if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                // Устройство подключено или отключено
                SetTimer(h, 1, 300, nullptr); // запускаем таймер на 300 мс
            }
        }
        return TRUE;

    case WM_TIMER:
        if (w == 1) {
            KillTimer(h, 1);
            // Здесь позже будет вызов RefreshDevices();
            MessageBoxW(h, L"Устройство изменилось!", L"USB", MB_OK); // временно
        }
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int RunMainWindow(HINSTANCE hInst, int nShow) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"UsbMonitorWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"USB-Monitor",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 750,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    DEV_BROADCAST_DEVICEINTERFACE_W filter{};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = kUsbDeviceGuid;

    // Сохраняем хэндл уведомления в глобальную переменную
    g_hNotify = RegisterDeviceNotificationW(
        hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}