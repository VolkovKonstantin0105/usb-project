#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

// Пользовательские сообщения главного окна
#define WM_APP_PACKETS  (WM_APP + 1)  // в очереди появились новые пакеты
#define WM_APP_CAPSTATE (WM_APP + 2)  // поток захвата завершился, lParam = код ошибки

// Преобразование ASCII-строки в wide (только латиница и цифры)
inline std::wstring Widen(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}
