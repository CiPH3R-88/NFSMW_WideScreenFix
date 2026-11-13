#pragma once
#ifndef NFSMW_WIDESCREENFIX_MOVIEFIX_H
#define NFSMW_WIDESCREENFIX_MOVIEFIX_H

#include "MemoryTools.h"
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <string>
#include <cctype>

namespace MovieFix {
    // Verified from IDA: cFEngRender::RenderMovie (0x00599DF0) quad vertex push operands
    constexpr uintptr_t ADDR_MOVIE_QUAD_Y2 = 0x0059A02A; // Height (Bottom)
    constexpr uintptr_t ADDR_MOVIE_QUAD_X2 = 0x0059A02F; // Width (Right)
    constexpr uintptr_t ADDR_MOVIE_QUAD_Y1 = 0x0059A034; // Height (Top)
    constexpr uintptr_t ADDR_MOVIE_QUAD_X1 = 0x0059A039; // Width (Left)

    // Verified from IDA: sub_7F996A (0x007F996A) - YUV420 to RGB Lookup Tables
    // Tables consumed by MMX video rasterizer (sub_7F91BD).
    constexpr uintptr_t ADDR_Y_TABLE        = 0x00908648; // Y table (256 * 8 bytes)
    constexpr uintptr_t ADDR_U_TABLE        = 0x00908E48; // U table (256 * 8 bytes)
    constexpr uintptr_t ADDR_V_TABLE        = 0x00909648; // V table (256 * 8 bytes)
    constexpr uintptr_t ADDR_CALC_YUV_LUT   = 0x007F996A; // sub_7F996A entry point
    constexpr uintptr_t ADDR_FLAG_YUV_RESET = 0x009BFE18; // Reset flag at end of sub_7F996A

    // Verified from IDA: BuildRegion::IsPal (0x0064A110)
    // Controls movie filename suffix (_pal vs _ntsc) and boot flow sequences.
    constexpr uintptr_t ADDR_BUILD_REGION_IS_PAL = 0x0064A110;

    inline int s_MovieType = 0; // 0 = Auto-detect, 1 = NTSC, 2 = PAL
    inline bool s_IsPal = false;

    inline bool __cdecl Hooked_BuildRegion_IsPal() {
        if (s_MovieType == 1) return false;
        if (s_MovieType == 2) return true;
        return s_IsPal;
    }

    inline bool DetectPalMovies(const std::filesystem::path& gamePath, bool& outIsPal) {
        std::filesystem::path moviesDir = gamePath / "MOVIES";
        std::error_code ec;
        if (!std::filesystem::exists(moviesDir, ec) || !std::filesystem::is_directory(moviesDir, ec)) {
            moviesDir = "MOVIES";
            if (!std::filesystem::exists(moviesDir, ec) || !std::filesystem::is_directory(moviesDir, ec)) {
                return false;
            }
        }

        bool hasPal = false;
        bool hasNtsc = false;

        for (const auto& entry : std::filesystem::directory_iterator(moviesDir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;

            std::string filename = entry.path().filename().string();
            for (char& c : filename) {
                c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
            }

            if (filename.find("_pal.vp6") != std::string::npos) {
                hasPal = true;
            } else if (filename.find("_ntsc.vp6") != std::string::npos) {
                hasNtsc = true;
            }

            if (hasPal && hasNtsc) break;
        }

        if (hasPal && !hasNtsc) {
            outIsPal = true;
            return true;
        }
        if (hasNtsc && !hasPal) {
            outIsPal = false;
            return true;
        }
        if (hasPal && hasNtsc) {
            outIsPal = true;
            return true;
        }
        return false;
    }

    // Full color range (0-255) LUT calculation from Toru
    struct RgbMap {
        uint16_t R;
        uint16_t G;
        uint16_t B;
        uint16_t A;
    };

    inline void MoviePlayerCalcYuvLut(uintptr_t Y, uintptr_t U, uintptr_t V) {
        auto* y_table = reinterpret_cast<RgbMap*>(Y);
        auto* u_table = reinterpret_cast<RgbMap*>(U);
        auto* v_table = reinterpret_cast<RgbMap*>(V);

        for (int i = 0; i < 256; i++) {
            y_table[i].R = (std::min)(static_cast<uint16_t>((std::max)(i - 16, 0) * (255.0 / 219.0)), static_cast<uint16_t>(255));
            y_table[i].G = (std::min)(static_cast<uint16_t>((std::max)(i - 16, 0) * (255.0 / 219.0)), static_cast<uint16_t>(255));
            y_table[i].B = (std::min)(static_cast<uint16_t>((std::max)(i - 16, 0) * (255.0 / 219.0)), static_cast<uint16_t>(255));
            y_table[i].A = 0xFF;

            u_table[i].R = static_cast<uint16_t>(((i / 255.0) - 0.5) * (+1.7720 * (255.0 / 224.0)) * 255.0);
            u_table[i].G = static_cast<uint16_t>(((i / 255.0) - 0.5) * (-0.3441 * (255.0 / 224.0)) * 255.0);
            u_table[i].B = 0;
            u_table[i].A = 0;

            v_table[i].R = 0;
            v_table[i].G = static_cast<uint16_t>(((i / 255.0) - 0.5) * (-0.7141 * (255.0 / 224.0)) * 255.0);
            v_table[i].B = static_cast<uint16_t>(((i / 255.0) - 0.5) * (+1.4020 * (255.0 / 224.0)) * 255.0);
            v_table[i].A = 0;
        }
    }

    inline void __cdecl Hooked_CalcYuvLut() {
        MoviePlayerCalcYuvLut(ADDR_Y_TABLE, ADDR_U_TABLE, ADDR_V_TABLE);
        *reinterpret_cast<uint32_t*>(ADDR_FLAG_YUV_RESET) = 0;
    }

    inline void Init(int mode, int movieType, const std::filesystem::path& gamePath, bool x360StuffInstalled = false) {
        // Hook YUV LUT generator (sub_7F996A) to generate full-color range tables (0-255)
        // instead of vanilla limited range (16-235).
        MemoryTools::MakeJMP(ADDR_CALC_YUV_LUT, reinterpret_cast<uintptr_t>(Hooked_CalcYuvLut));

        // Pre-calculate tables right away
        Hooked_CalcYuvLut();

        // --------------------------------------------------------------------
        // Movie Region Fix: NTSC (_ntsc) vs PAL (_pal)
        // --------------------------------------------------------------------
        s_MovieType = movieType;
        if (s_MovieType == 1) {
            s_IsPal = false; // Force NTSC
        } else if (s_MovieType == 2) {
            s_IsPal = true;  // Force PAL
        } else {
            // Auto-detect based on MOVIES folder contents
            bool detectedPal = false;
            if (DetectPalMovies(gamePath, detectedPal)) {
                s_IsPal = detectedPal;
            } else {
                // If MOVIES directory has no recognizable region files, preserve original game region
                auto origIsPal = reinterpret_cast<bool(__cdecl*)()>(ADDR_BUILD_REGION_IS_PAL);
                s_IsPal = origIsPal ? origIsPal() : false;
            }
        }

        // Hook BuildRegion::IsPal (0x0064A110) so CalculateMovieFilename and BootFlowManager use the correct region
        MemoryTools::MakeJMP(ADDR_BUILD_REGION_IS_PAL, reinterpret_cast<uintptr_t>(Hooked_BuildRegion_IsPal));

        if (mode <= 0 || x360StuffInstalled) return;

        constexpr float kAspectFactor = (4.0f / 3.0f) / (16.0f / 9.0f); // 0.75f
        constexpr float kScaled = 0.5f / kAspectFactor;                 // ~0.6666667f

        if (mode == 1) {
            // Mode 1: Fill 16:9 screen (zoom/crop letterbox)
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_Y2,  kScaled);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_X2,  kScaled);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_Y1, -kScaled);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_X1, -kScaled);
        } else if (mode == 2) {
            // Mode 2: Aspect ratio correct (preserve height, scale width to prevent stretch)
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_Y2,  0.5f);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_X2,  kScaled);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_Y1, -0.5f);
            MemoryTools::Write<float>(ADDR_MOVIE_QUAD_X1, -kScaled);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_MOVIEFIX_H
