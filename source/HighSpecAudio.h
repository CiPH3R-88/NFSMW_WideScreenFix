#pragma once
#ifndef NFSMW_WIDESCREENFIX_HIGHSPECAUDIO_H
#define NFSMW_WIDESCREENFIX_HIGHSPECAUDIO_H

#include <cstdint>
#include "MemoryTools.h"

namespace HighSpecAudio {
    // Verified from IDA: sub_4C40C0 (Audio system initialization)
    // In vanilla speed.exe, Low/Medium PerformanceLevel checks fall through to 0x004C41EF:
    //   mov dword ptr [g_PC_SampleRate], 22050  (bytes: C7 05 50 E6 8E 00 22 56 00 00)
    // Overwriting the immediate operand at 0x004C41F5 from 22050 (0x00005622) to 44100 (0x0000AC44)
    // forces the audio mixer to run at full CD quality (44.1 kHz) by default.
    constexpr uintptr_t ADDR_SAMPLE_RATE_FALLBACK = 0x004C41F5;
    constexpr uint32_t HIGH_SPEC_SAMPLE_RATE      = 44100;

    inline void Init() {
        MemoryTools::Write<uint32_t>(ADDR_SAMPLE_RATE_FALLBACK, HIGH_SPEC_SAMPLE_RATE);
    }
}

#endif // NFSMW_WIDESCREENFIX_HIGHSPECAUDIO_H

