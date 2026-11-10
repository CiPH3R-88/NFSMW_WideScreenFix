#pragma once
#ifndef NFSMW_WIDESCREENFIX_MEMORYTOOLS_H
#define NFSMW_WIDESCREENFIX_MEMORYTOOLS_H

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cstdlib>

namespace MemoryTools {
    inline uintptr_t PatternScan(const char* patternStr, size_t matchIndex = 0, uintptr_t base = 0x00400000, size_t size = 0x500000) {
        std::vector<uint8_t> bytes;
        std::vector<bool> mask;

        const char* p = patternStr;
        while (*p) {
            while (*p == ' ') p++;
            if (!*p) break;
            if (*p == '?') {
                bytes.push_back(0);
                mask.push_back(false);
                p++;
                if (*p == '?') p++;
            } else {
                char byteStr[3] = { p[0], p[1], 0 };
                bytes.push_back(static_cast<uint8_t>(strtoul(byteStr, nullptr, 16)));
                mask.push_back(true);
                p += 2;
            }
        }

        if (bytes.empty()) return 0;

        uint8_t* pStart = reinterpret_cast<uint8_t*>(base);
        size_t currentMatch = 0;
        for (size_t i = 0; i <= size - bytes.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < bytes.size(); ++j) {
                if (mask[j] && pStart[i + j] != bytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                if (currentMatch == matchIndex) {
                    return reinterpret_cast<uintptr_t>(pStart + i);
                }
                currentMatch++;
            }
        }
        return 0;
    }
    inline bool Unprotect(uintptr_t address, size_t size, DWORD& oldProtect) {
        return VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect) != FALSE;
    }

    inline void RestoreProtect(uintptr_t address, size_t size, DWORD oldProtect) {
        DWORD temp;
        VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &temp);
    }

    template <typename T>
    inline void Write(uintptr_t address, const T& value) {
        DWORD oldProtect;
        Unprotect(address, sizeof(T), oldProtect);
        *reinterpret_cast<T*>(address) = value;
        RestoreProtect(address, sizeof(T), oldProtect);
    }

    template <typename T>
    inline T Read(uintptr_t address) {
        return *reinterpret_cast<T*>(address);
    }

    inline void WriteBytes(uintptr_t address, const void* data, size_t size) {
        DWORD oldProtect;
        Unprotect(address, size, oldProtect);
        std::memcpy(reinterpret_cast<void*>(address), data, size);
        RestoreProtect(address, size, oldProtect);
    }

    inline void MakeNOP(uintptr_t address, size_t count = 1) {
        DWORD oldProtect;
        Unprotect(address, count, oldProtect);
        std::memset(reinterpret_cast<void*>(address), 0x90, count);
        RestoreProtect(address, count, oldProtect);
    }

    inline void MakeCALL(uintptr_t from, uintptr_t to, size_t nops = 0) {
        DWORD oldProtect;
        size_t total = 5 + nops;
        Unprotect(from, total, oldProtect);
        *reinterpret_cast<uint8_t*>(from) = 0xE8; // CALL rel32
        *reinterpret_cast<int32_t*>(from + 1) = static_cast<int32_t>(to - (from + 5));
        if (nops > 0) {
            std::memset(reinterpret_cast<void*>(from + 5), 0x90, nops);
        }
        RestoreProtect(from, total, oldProtect);
    }

    inline void MakeJMP(uintptr_t from, uintptr_t to, size_t nops = 0) {
        DWORD oldProtect;
        size_t total = 5 + nops;
        Unprotect(from, total, oldProtect);
        *reinterpret_cast<uint8_t*>(from) = 0xE9; // JMP rel32
        *reinterpret_cast<int32_t*>(from + 1) = static_cast<int32_t>(to - (from + 5));
        if (nops > 0) {
            std::memset(reinterpret_cast<void*>(from + 5), 0x90, nops);
        }
        RestoreProtect(from, total, oldProtect);
    }

    inline uintptr_t GetCallTarget(uintptr_t callInstructionAddress) {
        int32_t offset = Read<int32_t>(callInstructionAddress + 1);
        return callInstructionAddress + 5 + offset;
    }
}

#endif // NFSMW_WIDESCREENFIX_MEMORYTOOLS_H

