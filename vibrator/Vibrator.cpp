/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Vibrator"

#include "Vibrator.h"

#include <hardware/hardware.h>
#include <hardware/vibrator.h>
#include <log/log.h>

#include <cinttypes>
#include <cmath>
#include <fstream>
#include <iostream>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr uint32_t WAVEFORM_TICK_EFFECT_INDEX = 2;
static constexpr uint32_t WAVEFORM_TICK_EFFECT_MS = 9;

static constexpr uint32_t WAVEFORM_CLICK_EFFECT_INDEX = 3;
static constexpr uint32_t WAVEFORM_CLICK_EFFECT_MS = 9;

static constexpr uint32_t WAVEFORM_HEAVY_CLICK_EFFECT_INDEX = 4;
static constexpr uint32_t WAVEFORM_HEAVY_CLICK_EFFECT_MS = 9;
static constexpr uint32_t WAVEFORM_STRONG_HEAVY_CLICK_EFFECT_MS = 12;

static constexpr uint32_t WAVEFORM_DOUBLE_CLICK_EFFECT_INDEX = 7;
static constexpr uint32_t WAVEFORM_DOUBLE_CLICK_EFFECT_MS = 130;

static constexpr uint32_t WAVEFORM_RINGTONE_EFFECT_INDEX = 65534;
static constexpr uint32_t WAVEFORM_RINGTONE_EFFECT_MS = 30000;

// The_big_adventure - RINGTONE_1
static constexpr char WAVEFORM_RINGTONE1_EFFECT_QUEUE[] = "160, 11.100, 1600, 1!";

// Copycat - RINGTONE_2
static constexpr char WAVEFORM_RINGTONE2_EFFECT_QUEUE[] = "260, 12.100, 660, 2!";

// Crackle - RINGTONE_3
static constexpr char WAVEFORM_RINGTONE3_EFFECT_QUEUE[] = "404, 13.100, 1440, 5!";

// Flutterby - RINGTONE_4
static constexpr char WAVEFORM_RINGTONE4_EFFECT_QUEUE[] = "140, 14.100, 6!";

// Hotline - RINGTONE_5
static constexpr char WAVEFORM_RINGTONE5_EFFECT_QUEUE[] = "140, 15.100, 4!";

// Leaps_and_bounds - RINGTONE_6
static constexpr char WAVEFORM_RINGTONE6_EFFECT_QUEUE[] = "140, 16.100, 1!";

// Lollipop - RINGTONE_7
static constexpr char WAVEFORM_RINGTONE7_EFFECT_QUEUE[] = "140, 17.100, 624, 1!";

// Lost_and_found - RINGTONE_8
static constexpr char WAVEFORM_RINGTONE8_EFFECT_QUEUE[] = "140, 18.100, 1020,496, 1!";

// Mash_up - RINGTONE_9
static constexpr char WAVEFORM_RINGTONE9_EFFECT_QUEUE[] = "140, 19.100, 8, 3!";

// Monkey_around - RINGTONE_10
static constexpr char WAVEFORM_RINGTONE10_EFFECT_QUEUE[] = "20.100, 23.100, 23.80, 23.60, 892, 4!";

// Schools_out - RINGTONE_11
static constexpr char WAVEFORM_RINGTONE11_EFFECT_QUEUE[] = "21.60, 21.80, 21.100, 1020, 564, 6!";

// Zen_too - RINGTONE_12
static constexpr char WAVEFORM_RINGTONE12_EFFECT_QUEUE[] = "140, 22.100, 972, 1!";

static constexpr float AMP_ATTENUATE_STEP_SIZE = 0.125f;

static constexpr int8_t MAX_TRIGGER_LATENCY_MS = 6;

static uint8_t amplitudeToScale(float amplitude, float maximum) {
    return std::round((-20 * std::log10(amplitude / maximum)) / (AMP_ATTENUATE_STEP_SIZE));
}

Vibrator::Vibrator(std::ofstream &&activate, std::ofstream &&duration, std::ofstream &&effect,
                   std::ofstream &&queue, std::ofstream &&scale)
    : mActivate(std::move(activate)),
      mDuration(std::move(duration)),
      mEffectIndex(std::move(effect)),
      mEffectQueue(std::move(queue)),
      mScale(std::move(scale)) {}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t *_aidl_return) {
    int32_t ret = 0;
    if (mScale) {
        ret |= IVibrator::CAP_AMPLITUDE_CONTROL;
    }
    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::enable(uint32_t timeoutMs, uint32_t effectIndex) {
    mEffectIndex << effectIndex << std::endl;
    mDuration << (timeoutMs + MAX_TRIGGER_LATENCY_MS) << std::endl;
    mActivate << 1 << std::endl;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback> &callback) {
    if (callback) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    return enable(timeoutMs, 0);
}

ndk::ScopedAStatus Vibrator::off() {
    mActivate << 0 << std::endl;
    if (!mActivate) {
        ALOGE("Failed to turn vibrator off (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int32_t scale = amplitudeToScale(amplitude, 1.0);

    mScale << scale << std::endl;
    if (!mScale) {
        ALOGE("Failed to set amplitude (%d): %s", errno, strerror(errno));
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool /*enabled*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect> *_aidl_return) {
    *_aidl_return = {
        Effect::TICK,       Effect::CLICK,       Effect::HEAVY_CLICK, Effect::DOUBLE_CLICK,
        Effect::RINGTONE_1, Effect::RINGTONE_2,  Effect::RINGTONE_3,  Effect::RINGTONE_4,
        Effect::RINGTONE_5, Effect::RINGTONE_6,  Effect::RINGTONE_7,  Effect::RINGTONE_8,
        Effect::RINGTONE_9, Effect::RINGTONE_10, Effect::RINGTONE_11, Effect::RINGTONE_12};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback> &callback,
                                     int32_t *_aidl_return) {
    ndk::ScopedAStatus status;

    if (callback) {
        status = ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else {
        status = performEffect(effect, strength, _aidl_return);
    }

    return status;
}

ndk::ScopedAStatus Vibrator::performEffect(Effect effect, EffectStrength strength,
                                           int32_t *outTimeMs) {
    ndk::ScopedAStatus status;
    uint32_t timeMs = 0;
    uint32_t effectIndex;

    switch (effect) {
        case Effect::TICK:
            effectIndex = WAVEFORM_TICK_EFFECT_INDEX;
            timeMs = WAVEFORM_TICK_EFFECT_MS;
            break;
        case Effect::CLICK:
            effectIndex = WAVEFORM_CLICK_EFFECT_INDEX;
            timeMs = WAVEFORM_CLICK_EFFECT_MS;
            break;
        case Effect::HEAVY_CLICK:
            effectIndex = WAVEFORM_HEAVY_CLICK_EFFECT_INDEX;
            if (strength == EffectStrength::STRONG) {
                timeMs = WAVEFORM_STRONG_HEAVY_CLICK_EFFECT_MS;
            } else {
                timeMs = WAVEFORM_HEAVY_CLICK_EFFECT_MS;
            }
            break;
        case Effect::DOUBLE_CLICK:
            effectIndex = WAVEFORM_DOUBLE_CLICK_EFFECT_INDEX;
            timeMs = WAVEFORM_DOUBLE_CLICK_EFFECT_MS;
            break;
        case Effect::RINGTONE_1:
            mEffectQueue << WAVEFORM_RINGTONE1_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_2:
            mEffectQueue << WAVEFORM_RINGTONE2_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_3:
            mEffectQueue << WAVEFORM_RINGTONE3_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_4:
            mEffectQueue << WAVEFORM_RINGTONE4_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_5:
            mEffectQueue << WAVEFORM_RINGTONE5_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_6:
            mEffectQueue << WAVEFORM_RINGTONE6_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_7:
            mEffectQueue << WAVEFORM_RINGTONE7_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_8:
            mEffectQueue << WAVEFORM_RINGTONE8_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_9:
            mEffectQueue << WAVEFORM_RINGTONE9_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_10:
            mEffectQueue << WAVEFORM_RINGTONE10_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_11:
            mEffectQueue << WAVEFORM_RINGTONE11_EFFECT_QUEUE << std::endl;
            break;
        case Effect::RINGTONE_12:
            mEffectQueue << WAVEFORM_RINGTONE12_EFFECT_QUEUE << std::endl;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    // EffectStrength needs to be handled differently for ringtone effects
    if (effect >= Effect::RINGTONE_1 && effect <= Effect::RINGTONE_15) {
        effectIndex = WAVEFORM_RINGTONE_EFFECT_INDEX;
        timeMs = WAVEFORM_RINGTONE_EFFECT_MS;
        switch (strength) {
            case EffectStrength::LIGHT:
                setAmplitude(0.3);
                break;
            case EffectStrength::MEDIUM:
                setAmplitude(0.5);
                break;
            case EffectStrength::STRONG:
                setAmplitude(1.0);
                break;
            default:
                return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
    } else {
        switch (strength) {
            case EffectStrength::LIGHT:
                effectIndex -= 1;
                break;
            case EffectStrength::MEDIUM:
                break;
            case EffectStrength::STRONG:
                effectIndex += 1;
                break;
            default:
                return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
        timeMs += MAX_TRIGGER_LATENCY_MS;  // Add expected cold-start latency
        setAmplitude(1.0);                 // Always set full-scale for non-ringtone constants
    }

    status = enable(timeMs, effectIndex);
    if (!status.isOk()) {
        return status;
    }

    *outTimeMs = timeMs;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect> * /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t /*id*/, Effect /*effect*/,
                                            EffectStrength /*strength*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t /*id*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t * /*maxDelayMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t * /*maxSize*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive> * /*supported*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive /*primitive*/,
                                                  int32_t * /*durationMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect> & /*composite*/,
                                     const std::shared_ptr<IVibratorCallback> & /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float * /*resonantFreqHz*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getQFactor(float * /*qFactor*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float * /*freqResolutionHz*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float * /*freqMinimumHz*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> * /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t * /*durationMs*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t * /*maxSize*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> * /*supported*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> & /*composite*/,
                                         const std::shared_ptr<IVibratorCallback> & /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
