#pragma once
#ifndef NFSMW_WIDESCREENFIX_NOSTRAILFIX_H
#define NFSMW_WIDESCREENFIX_NOSTRAILFIX_H

#include <windows.h>
#include <cstdint>
#include <cstring>
#include "MemoryTools.h"

namespace NOSTrailFix {
    // Verified from IDA: VehicleRenderConn::RenderFlares (0x00744E20)
    constexpr uintptr_t ADDR_NOS_TRAIL_TEST = 0x00744FB7; // 85 C0 (test eax, eax)
    constexpr uintptr_t ADDR_LOOP_COUNT = 0x00744FC3; // Immediate value '3' in: mov [esp+10h], 3
    constexpr uintptr_t ADDR_FLARE_DIV = 0x00903694; // Global FlareDiv (default 8)
    constexpr uintptr_t ADDR_NOS_TRAIL_LENGTH = 0x00745038; // 8 bytes: EB 06 8D 9B 00 00 00 00
    constexpr uintptr_t ADDR_NOS_TRAIL_CALL = 0x007450E8; // call CarRenderInfo::RenderFlaresOnCar

    // Pause Fix addresses (VehicleRenderConn::RenderFlares & FEManager::mPauseRequest)
    inline const uintptr_t ADDR_PAUSE_REQUEST = 0x0091CAE4; // FEManager::mPauseRequest
    inline const uintptr_t ADDR_HISTORY_HOOK = 0x00744F0C; // cmp [edi+1280h], ecx (6 bytes: 39 8F 80 12 00 00)
    inline const uintptr_t RET_HISTORY_NORMAL = 0x00744F12; // jge loc_744F1A
    inline const uintptr_t JMP_HISTORY_SKIP = 0x00744F79; // loc_744F79

    inline const uintptr_t ADDR_SPEED_CHECK_HOOK = 0x00744FA7; // jnz loc_74510E (6 bytes: 0F 85 61 01 00 00)
    inline const uintptr_t RET_SPEED_CONTINUE = 0x00744FAD; // mov edx, [esp+10h]
    inline const uintptr_t JMP_SPEED_SKIP = 0x0074510E; // loc_74510E

    struct bVector3 {
        float x, y, z;
    };

    struct bVector4 {
        float x, y, z, w;
    };

    struct bMatrix4 {
        bVector4 v0;
        bVector4 v1;
        bVector4 v2;
        bVector4 v3;
    };

#pragma pack(push, 1)
    struct eLightFlare {
        eLightFlare* Next;
        eLightFlare* Prev;
        uint32_t     NameHash;
        uint32_t     ColourTint;
        float        PositionX;
        float        PositionY;
        float        PositionZ;
        float        ReflectPosZ;
        float        DirectionX;
        float        DirectionY;
        float        DirectionZ;
        char         Type;
        char         Flags;
        int16_t      ScenerySectionNumber;
    };
#pragma pack(pop)

    inline float s_NOSTrailLength = 0.0f;
    inline float s_NOSTrailPositionScalar = 0.0f;
    inline bool  s_NOSTrailFixForHighFPS = true;
    inline float s_ActiveHz = 60.0f;

    // Static buffers for velocity tracking
    inline bVector3 s_ActiveVelocity = { 0.0f, 0.0f, 0.0f };
    inline bVector3 s_PrevActiveVelocity = { 0.0f, 0.0f, 0.0f };
    inline bVector3 s_PointA_Rear = { 0.0f, 0.0f, 0.0f };
    inline void* s_LastCar = nullptr;
    inline uint32_t s_LastTick = 0;

    // Velocity-Driven & Car-Anchored Trail Handler:
    // Root cause of length flicker: At high FPS (120+ Hz), rendering runs faster than physics ticks.
    // Sampling frame-to-frame position history (mHistoryPos) alternated between full movement
    // and zero movement on skipped physics ticks, causing length to violently fluctuate.
    // By deriving trail displacement directly from the car's continuous physics velocity (v5[14]),
    // the trail length is 100% steady, smooth at all speeds, and completely flicker-free.
    //
    // Root cause of pause offset: Anchoring to previous history slots placed the trail where the
    // car was 1-2 frames ago. Anchoring directly to v37 (the car's current world position on the
    // stack) ensures 0.0 mm gap between the taillights and the trail while driving AND while paused.
    extern "C" inline bVector3* __cdecl ProcessNOSTrailPass(
        void* carRenderInfo,
        int passIndex,
        float* delta,
        void* callerEsp,
        int* pMatrixIndex
    ) {
        if (!carRenderInfo || !delta || !callerEsp || !pMatrixIndex) {
            if (pMatrixIndex) *pMatrixIndex = 0;
            return reinterpret_cast<bVector3*>(delta);
        }

        uintptr_t carBase = reinterpret_cast<uintptr_t>(carRenderInfo);
        uint32_t head = *reinterpret_cast<uint32_t*>(carBase + 0x1280);

        // 1. Car's exact current 3D world position from caller's stack frame (v37 at [caller_esp + 0x20])
        bVector3* pCarPos = reinterpret_cast<bVector3*>(reinterpret_cast<char*>(callerEsp) + 0x20);

        // 2. Car's linear velocity vector from VehicleRenderConn (v4 at [caller_esp + 0x14], v5[14] at +0x38)
        uintptr_t pVehEntry = *reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(callerEsp) + 0x14);
        uintptr_t vConn = pVehEntry ? *reinterpret_cast<uintptr_t*>(pVehEntry) : 0;
        bVector3* pVelocity = vConn ? *reinterpret_cast<bVector3**>(vConn + 0x38) : nullptr;

        uint32_t currentTick = GetTickCount();
        if (currentTick - s_LastTick > 100 || carRenderInfo != s_LastCar) {
            s_LastCar = carRenderInfo;
            s_ActiveVelocity = { 0.0f, 0.0f, 0.0f };
            s_PrevActiveVelocity = { 0.0f, 0.0f, 0.0f };
        }
        s_LastTick = currentTick;

        // Check if simulation is paused
        bool isPaused = (*reinterpret_cast<uint32_t*>(ADDR_PAUSE_REQUEST) != 0);

        // Update active velocities from physics when car is moving and not paused
        if (pVelocity && !isPaused) {
            float vx = pVelocity->x;
            float vy = pVelocity->y;
            float vz = pVelocity->z;
            float speedSq = vx * vx + vy * vy + vz * vz;

            // Physical speed threshold: 0.2 m/s (speedSq > 0.04f)
            if (speedSq > 0.04f) {
                if (s_ActiveVelocity.x != 0.0f || s_ActiveVelocity.y != 0.0f || s_ActiveVelocity.z != 0.0f) {
                    s_PrevActiveVelocity = s_ActiveVelocity;
                }
                else {
                    s_PrevActiveVelocity = *pVelocity;
                }
                s_ActiveVelocity = *pVelocity;
            }
        }

        // Use active velocity (persists while paused for frozen pause trail)
        bVector3 vel0 = s_ActiveVelocity;
        bVector3 vel1 = s_PrevActiveVelocity;
        if (vel0.x == 0.0f && vel0.y == 0.0f && vel0.z == 0.0f) {
            if (pVelocity) {
                vel0 = *pVelocity;
                vel1 = *pVelocity;
            }
        }

        // Time step calculation: normalize to vanilla 60 FPS physical distance
        float baseHz = (s_NOSTrailFixForHighFPS || s_ActiveHz <= 0.0f) ? 60.0f : s_ActiveHz;
        float timeStep = 1.0f / baseHz;
        float lengthMult = (s_NOSTrailLength > 0.0f) ? s_NOSTrailLength : 1.0f;
        float scale = timeStep * lengthMult;

        // Backward displacement vectors along car trajectory
        bVector3 S1 = { -vel0.x * scale, -vel0.y * scale, -vel0.z * scale };
        bVector3 S2 = { -vel1.x * scale, -vel1.y * scale, -vel1.z * scale };

        // Pass 2 (i = 2) is executed first: Rear Segment (midpoint -> tail tip)
        if (passIndex == 2) {
            s_PointA_Rear.x = pCarPos->x + S1.x;
            s_PointA_Rear.y = pCarPos->y + S1.y;
            s_PointA_Rear.z = pCarPos->z + S1.z;

            delta[0] = S2.x;
            delta[1] = S2.y;
            delta[2] = S2.z;

            *pMatrixIndex = (head + 2) % 3;
            return &s_PointA_Rear;
        }
        // Pass 1 (i = 1) is executed second: Front Segment (car taillights -> midpoint)
        else if (passIndex == 1) {
            delta[0] = S1.x;
            delta[1] = S1.y;
            delta[2] = S1.z;

            *pMatrixIndex = head % 3;
            return pCarPos;
        }

        *pMatrixIndex = head % 3;
        return pCarPos;
    }

    // Hook at 0x00745038: Intercepts each pass of the trail loop, passing car render info,
    // loop index i, caller's delta vector, caller's esp (for v37 & velocity), and updating Point A.
    __declspec(naked) inline void Hook_NOSTrailLength() {
        __asm {
            push ebx                        // reserve stack space for matrixIndex out-param
            lea eax, [esp]                  // eax = &matrixIndex
            push eax                        // arg 5: int* pMatrixIndex
            lea eax, [esp + 4 + 8]          // eax = caller_esp (offset 4 from entry + 8 bytes of 2 pushes)
            push eax                        // arg 4: void* callerEsp
            lea eax, [esp + 0x34 + 12]      // arg 3: float* delta (caller's [esp+0x34] shifted by 3 pushes)
            push eax
            mov eax, [esp + 0x14 + 16]      // arg 2: int passIndex (caller's [esp+0x14] shifted by 4 pushes)
            push eax
            push edi                        // arg 1: void* carRenderInfo (edi)
            call ProcessNOSTrailPass
            add esp, 20                     // pop 5 arguments (5 * 4 = 20)

            pop ebx                         // update ebx to chosen matrixIndex
            mov esi, eax                    // update esi to Point A pointer returned in eax
            xor eax, eax                    // MUST ZERO EAX SO THE ENGINE LOOP RUNS!
            ret
        }
    }

    // Original RenderFlaresOnCar function pointer
    using PFN_RenderFlaresOnCar = void(__thiscall*)(void* thisCarRenderInfo, void* pView, bVector3* pPosition, bMatrix4* pBodyMatrix, int flag1, int flag2, int flag3);
    inline PFN_RenderFlaresOnCar s_OrigRenderFlaresOnCar = nullptr;

    inline void __fastcall Hook_RenderFlaresOnCar(
        void* thisCarRenderInfo,
        void* dummyEdx,
        void* pView,
        bVector3* pPosition,
        bMatrix4* pBodyMatrix,
        int flag1,
        int flag2,
        int flag3
    ) {
        if (!thisCarRenderInfo || !s_OrigRenderFlaresOnCar) {
            return;
        }

        // Omnidirectional 360° Visibility:
        // Brakelight flares (Type == 1) have bit 2 (value 4) set in Flare->Flags by default.
        // In eRenderLightFlare (0x00505380), bit 2 causes the flare intensity to be multiplied by
        // pow(dot_product, exponent) where dot_product = FlareNormal . CameraViewVector.
        // When viewed from front or side angles (e.g. debug camera / photo mode), dot_product <= 0,
        // which culls the flares completely. Clearing bit 2 (Flags &= ~4) during NOS trail passes
        // disables directional attenuation, rendering the trail sprites at full glow from all 360° angles!
        uintptr_t carBase = reinterpret_cast<uintptr_t>(thisCarRenderInfo);
        eLightFlare* sentinel = reinterpret_cast<eLightFlare*>(carBase + 0x104);
        eLightFlare* flareHead = *reinterpret_cast<eLightFlare**>(carBase + 0x104);

        constexpr int MAX_FLARES = 32;
        eLightFlare* modifiedFlares[MAX_FLARES];
        char origFlags[MAX_FLARES];
        int count = 0;

        for (eLightFlare* f = flareHead; f && f != sentinel && count < MAX_FLARES; f = f->Next) {
            if (f->Type == 1) { // ELF_CAR_BRAKELIGHT
                modifiedFlares[count] = f;
                origFlags[count] = f->Flags;
                f->Flags &= ~4; // Disable directional falloff for 360° billboard visibility
                count++;
            }
        }

        // Custom Position Scalar (if configured)
        bMatrix4 adjustedMatrix;
        bMatrix4* pMatrixToUse = pBodyMatrix;
        if (s_NOSTrailPositionScalar > 0.0f && pBodyMatrix) {
            adjustedMatrix = *pBodyMatrix;
            float posScale = 1.0f + s_NOSTrailPositionScalar;
            adjustedMatrix.v0.x *= posScale;
            adjustedMatrix.v0.y *= posScale;
            adjustedMatrix.v0.z *= posScale;
            adjustedMatrix.v2.x *= posScale;
            adjustedMatrix.v2.y *= posScale;
            pMatrixToUse = &adjustedMatrix;
        }

        s_OrigRenderFlaresOnCar(thisCarRenderInfo, pView, pPosition, pMatrixToUse, flag1, flag2, flag3);

        // Restore original flare flags so normal taillights remain directional
        for (int i = 0; i < count; ++i) {
            modifiedFlares[i]->Flags = origFlags[i];
        }
    }

    // Hook at 0x00744F0C: Freezes the 3-frame history ring buffer while paused so previous positions are preserved
    __declspec(naked) inline void Hook_HistoryBufferPause() {
        __asm {
            cmp dword ptr ds : [0x0091CAE4] , 0
            jne skip_history

            // Original instruction: cmp [edi+1280h], ecx
            cmp dword ptr[edi + 0x1280], ecx
            jmp dword ptr[RET_HISTORY_NORMAL]

            skip_history :
            jmp dword ptr[JMP_HISTORY_SKIP]
        }
    }

    // Hook at 0x00744FA7: Bypasses velocity check when paused so trail loop still renders frozen flare quads
    __declspec(naked) inline void Hook_SpeedCheckPause() {
        __asm {
            // Evaluate flags from original (test ah, 41h at 0x00744FA4)
            jnz speed_below_threshold
            // Speed > 0.2f: proceed to trail rendering
            jmp dword ptr[RET_SPEED_CONTINUE]

            speed_below_threshold:
            // Speed <= 0.2f. If paused, keep rendering trail!
            cmp dword ptr ds : [0x0091CAE4] , 0
                jne continue_paused

                // Not paused: skip trail rendering
                jmp dword ptr[JMP_SPEED_SKIP]

                continue_paused :
                jmp dword ptr[RET_SPEED_CONTINUE]
        }
    }

    inline void Init(bool nosTrailEnabled, float trailLength = 0.0f, float positionScalar = 0.0f, bool pauseFix = false, bool highFpsFix = true, float activeHz = 60.0f) {
        // Toggle: 0x00744FB7 (31 C0 = xor eax, eax skips trail; 85 C0 = test eax, eax) - Verified from IDA
        if (!nosTrailEnabled) {
            MemoryTools::Write<uint16_t>(ADDR_NOS_TRAIL_TEST, 0xC031);
            return;
        }

        MemoryTools::Write<uint16_t>(ADDR_NOS_TRAIL_TEST, 0xC085);

        s_NOSTrailLength = trailLength;
        s_NOSTrailPositionScalar = positionScalar;
        s_NOSTrailFixForHighFPS = highFpsFix;
        s_ActiveHz = activeHz;

        // If high-FPS fix is active OR custom trail length is set:
        // 1. Change outer loop count from 3 passes to 2 sequential passes (eliminates the triangle chord!)
        // 2. Double FlareDiv from 8 to 16 so both segments have 16 sprites (full-length intense glow)
        // 3. Hook at 0x00745038 to calculate the seamless backward segments
        if (s_NOSTrailFixForHighFPS || s_NOSTrailLength > 0.0f) {
            MemoryTools::Write<uint8_t>(ADDR_LOOP_COUNT, 2);
            MemoryTools::Write<uint32_t>(ADDR_FLARE_DIV, 16);
            MemoryTools::MakeCALL(ADDR_NOS_TRAIL_LENGTH, reinterpret_cast<uintptr_t>(Hook_NOSTrailLength), 3);
        }

        // Hook RenderFlaresOnCar at 0x007450E8:
        // 1. Omnidirectional 360° visibility: Taillights by default cull when viewed from front/side angles.
        //    Clearing bit 2 of Flare->Flags (Flags &= ~4) during trail rendering disables directional falloff,
        //    keeping the NOS flare trail visible from all camera angles (rear, sides, front, and debug cam).
        // 2. Custom Position scalar: If NOSTrailPositionScalar > 0, scales the flare body matrix offset.
        if (!s_OrigRenderFlaresOnCar) {
            s_OrigRenderFlaresOnCar = reinterpret_cast<PFN_RenderFlaresOnCar>(MemoryTools::GetCallTarget(ADDR_NOS_TRAIL_CALL));
            MemoryTools::MakeCALL(ADDR_NOS_TRAIL_CALL, reinterpret_cast<uintptr_t>(Hook_RenderFlaresOnCar));
        }

        // Pause Fix: Freeze history ring buffer and bypass speed check during pause
        if (pauseFix) {
            MemoryTools::MakeJMP(ADDR_HISTORY_HOOK, reinterpret_cast<uintptr_t>(Hook_HistoryBufferPause), 1);
            MemoryTools::MakeJMP(ADDR_SPEED_CHECK_HOOK, reinterpret_cast<uintptr_t>(Hook_SpeedCheckPause), 1);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_NOSTRAILFIX_H
