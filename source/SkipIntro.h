#pragma once
#ifndef NFSMW_WIDESCREENFIX_SKIPINTRO_H
#define NFSMW_WIDESCREENFIX_SKIPINTRO_H

#include "MemoryTools.h"
#include <windows.h>

namespace SkipIntro {
    // Verified from IDA: FEngMovieStarter::Callback (0x00559680) and SkipMovies flag (0x00926144)

    constexpr uintptr_t HOOK_SKIP_MOVIES_CHECK = 0x0055969B;
    constexpr uintptr_t RET_SKIP_MOVIES_CHECK  = 0x005596A4;
    constexpr uintptr_t ADDR_SKIP_MOVIES_FLAG  = 0x00926144;

    inline int s_SkipCounter = 0;
    inline uint32_t s_OriginalValue = 0;

    inline void __declspec(naked) FEngMovieStarter_Callback_Hook() {
        __asm {
            mov eax, [ADDR_SKIP_MOVIES_FLAG]
            cmp s_SkipCounter, 3
            jge restore_flag

            cmp s_SkipCounter, 0
            jne set_flag
            mov edx, [eax]
            mov s_OriginalValue, edx

        set_flag:
            mov dword ptr [eax], 1
            inc s_SkipCounter
            jmp finish

        restore_flag:
            mov edx, s_OriginalValue
            mov [eax], edx

        finish:
            mov eax, [eax]
            test eax, eax
            jmp RET_SKIP_MOVIES_CHECK
        }
    }

    inline void Init() {
        // Hook 9 bytes at 0x0055969B (5 bytes JMP + 4 NOPs)
        MemoryTools::MakeJMP(HOOK_SKIP_MOVIES_CHECK, reinterpret_cast<uintptr_t>(FEngMovieStarter_Callback_Hook), 4);
    }
}

#endif // NFSMW_WIDESCREENFIX_SKIPINTRO_H
