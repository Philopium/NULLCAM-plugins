#pragma once

// Real-time dynamic rhythm slicer, with no JUCE in it.
//
// Like a Boss Slicer, live steps chop the incoming signal in real time with zero
// buffering lag; ratchets, reverses, holds, and scatters pull seamlessly from a
// 2.7 s capture ring. You choose a time signature and a rhythmic style, and your
// playing dynamics rewrite the pattern density, slice length, and ratchets live.
//
//   input ──► live slice gate ──────────────────────► tone ──► mix ──► out
//      │            ▲                                   ▲
//      └──► capture ring (2.7 s) ──► repeat/rev/scatter │
//      │                                                │
//      └──► dynamics & onset ──► time-sig pattern gen ──┘
//
// Everything here is plain C++ so tests/dsp_test.cpp and the editor can share the
// exact same pattern rules.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mauri
{
constexpr int kCapturePow2 = 17;                       // 131072 samples
constexpr int kCaptureSize = 1 << kCapturePow2;        // ≈ 2.7 s at 48 kHz
constexpr int kCaptureMask = kCaptureSize - 1;
constexpr int kScopePoints = 512;
constexpr int kMaxSteps = 20;
constexpr int kTimeSigs = 8;
constexpr int kStyles = 8;

enum class Mode
{
    live = 0,     // zero-latency rhythmic gate on the live input
    scatter,      // jump to a captured transient or slice in the ring
    onset,        // re-trigger from the most recent detected onset
    hold          // freeze and ratchet the captured slice
};

struct TimeSigInfo
{
    const char* name;
    int steps;           // sixteenth-note steps in one bar (<= kMaxSteps)
    int beatGroup;       // primary beat grouping for UI markers (e.g. 4 for 4/4, 6 for 6/8)
    int secondaryGroup;  // optional secondary accent marker (e.g. 3+2 or 2+2+3)
};

inline constexpr std::array<TimeSigInfo, kTimeSigs> kTimeSigsInfo {{
    { "4/4", 16, 4, 4 },
    { "3/4", 12, 4, 4 },
    { "5/4", 20, 4, 4 },
    { "2/4",  8, 4, 4 },
    { "6/8", 12, 6, 6 },
    { "5/8", 10, 6, 4 },   // 3 + 2 eighth-note feel
    { "7/8", 14, 4, 6 },   // 2 + 2 + 3 eighth-note feel
    { "9/8", 18, 6, 6 }    // 3 + 3 + 3 eighth-note feel
}};

inline constexpr std::array<const char*, kStyles> kStyleNames {
    "GROOVE", "EUCLID", "SYNCOP", "ROLL", "GHOST", "BACK", "TREM", "INVERT"
};

// Beats per step for the eight subdivisions: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/32T, 1/64
inline constexpr std::array<double, 8> kDivisionBeats { 1.0, 0.5, 1.0 / 3.0, 0.25,
                                                        1.0 / 6.0, 0.125, 1.0 / 12.0, 0.0625 };

inline constexpr std::array<const char*, 8> kDivisionNames { "1/4", "1/8", "1/8T", "1/16",
                                                             "1/16T", "1/32", "1/32T", "1/64" };

struct Settings
{
    int clock = 0;         // 0 host, 1 input, 2 free
    int timeSig = 0;       // index into kTimeSigsInfo
    int rateIndex = 3;     // index into kDivisionBeats (default 1/16)
    int style = 0;         // index into kStyleNames
    float slice = 0.65f;   // base gate/slice length
    float repeat = 0.10f;  // base ratchets inside a step
    float reverse = 0.0f;  // reverse probability
    float scatter = 0.10f; // scatter probability
    float dynPat = 0.70f;  // how strongly dynamics morph the pattern
    float dynSlice = 0.50f;// how strongly dynamics tighten slice length
    float dynRate = 0.45f; // how strongly dynamics add ratchets
    float sens = 0.55f;    // input dynamics & transient sensitivity
    float tone = 0.80f;    // wet low pass, 0 dark .. 1 open
    float mix = 1.0f;
    float output = 1.0f;
};

struct Step
{
    bool on = false;
    bool accent = false;
    bool reverse = false;
    Mode mode = Mode::live;
    float len = 1.0f;
    int extraRepeats = 0;
};

inline int stepsForTimeSig (int timeSigIndex) noexcept
{
    const auto idx = (std::size_t) std::clamp (timeSigIndex, 0, kTimeSigs - 1);
    return kTimeSigsInfo[idx].steps;
}

// Metrical weight of step `i` inside `timeSigIndex` in [0.05, 1.0]:
// 1.0 = downbeat, 0.85 = main pulse, 0.68 = eighth offbeat, 0.42..0.15 = 16th syncopations.
inline float metricalWeight (int timeSigIndex, int stepIndex) noexcept
{
    const auto ts = std::clamp (timeSigIndex, 0, kTimeSigs - 1);
    const auto total = stepsForTimeSig (ts);
    const auto i = ((stepIndex % total) + total) % total;

    if (i == 0)
        return 1.0f;

    switch (ts)
    {
        case 0: // 4/4 (16)
            if (i == 8) return 0.90f;
            if (i == 4 || i == 12) return 0.80f;
            if ((i & 1) == 0) return 0.58f;
            return (i == 3 || i == 7 || i == 11 || i == 15) ? 0.34f : 0.20f;

        case 1: // 3/4 (12)
            if (i == 4 || i == 8) return 0.82f;
            if ((i & 1) == 0) return 0.56f;
            return (i == 3 || i == 7 || i == 11) ? 0.32f : 0.18f;

        case 2: // 5/4 (20: 3+2 feel)
            if (i == 12) return 0.88f;
            if (i == 4 || i == 8 || i == 16) return 0.78f;
            if ((i & 1) == 0) return 0.54f;
            return (i % 4 == 3) ? 0.30f : 0.16f;

        case 3: // 2/4 (8)
            if (i == 4) return 0.86f;
            if ((i & 1) == 0) return 0.60f;
            return 0.28f;

        case 4: // 6/8 (12: 3+3 eighth pulses)
            if (i == 6) return 0.90f;
            if (i == 2 || i == 4 || i == 8 || i == 10) return 0.66f;
            return (i == 5 || i == 11) ? 0.36f : 0.18f;

        case 5: // 5/8 (10: 3+2)
            if (i == 6) return 0.88f;
            if (i == 2 || i == 4 || i == 8) return 0.64f;
            return (i == 5 || i == 9) ? 0.34f : 0.16f;

        case 6: // 7/8 (14: 2+2+3)
            if (i == 4 || i == 8) return 0.86f;
            if (i == 2 || i == 6 || i == 10 || i == 12) return 0.64f;
            return (i == 3 || i == 7 || i == 13) ? 0.34f : 0.18f;

        case 7: // 9/8 (18: 3+3+3)
            if (i == 6 || i == 12) return 0.86f;
            if ((i & 1) == 0) return 0.62f;
            return (i == 5 || i == 11 || i == 17) ? 0.32f : 0.16f;
    }

    return 0.5f;
}

// Compute the effective dynamic energy [0..1] that drives pattern generation.
// Weak playing = low energy = simple pattern; hard playing = high energy = dense,
// syncopated, ratcheted pattern. DYN PAT decides how much of the playing is allowed
// to reshape the grid: at 0 the pattern stays a fixed medium groove, at 1 the whole
// range from a bare pulse to a dense shuffle is reachable.
inline float dynamicPatternEnergy (float velocity, float dynPat, int style) noexcept
{
    const auto v = std::clamp (velocity, 0.0f, 1.0f);
    const auto dp = std::clamp (dynPat, 0.0f, 1.0f);

    // INVERT flips soft <-> hard: gentle playing creates dense shimmer, hard accents
    // open into sparse spaces.
    const auto shaped = (style == 7) ? (1.0f - v) : v;
    const auto liveEnergy = std::clamp (0.04f + shaped * 1.04f, 0.0f, 1.0f);

    return 0.55f * (1.0f - dp) + liveEnergy * dp;
}

// Complexity tier unlocked by the energy: the pattern grows from a bare pulse to a
// fully loaded shuffle as the playing gets stronger.
enum class Tier
{
    pulse = 0,   // downbeat / strong beats only
    groove = 1,  // + offbeats
    syncop = 2,  // + sixteenths and syncopation
    shred = 3    // + ratchets, holds, scatters, reverses
};

inline Tier complexityTier (float energy) noexcept
{
    if (energy < 0.26f) return Tier::pulse;
    if (energy < 0.54f) return Tier::groove;
    if (energy < 0.78f) return Tier::syncop;
    return Tier::shred;
}

// Pure deterministic pattern generator used by BOTH the audio engine and the UI grid.
// Given a preselected time signature, style, and live dynamic energy, returns what
// happens at step `stepIndex`.
inline Step evaluateStep (int timeSigIndex, int styleIndex, int stepIndex, float energy) noexcept
{
    const auto ts = std::clamp (timeSigIndex, 0, kTimeSigs - 1);
    const auto st = std::clamp (styleIndex, 0, kStyles - 1);
    const auto total = stepsForTimeSig (ts);
    const auto i = ((stepIndex % total) + total) % total;
    const auto e = std::clamp (energy, 0.0f, 1.0f);
    const auto w = metricalWeight (ts, i);
    const auto tier = complexityTier (e);

    Step s;
    s.accent = (w >= 0.80f);

    // How many events are allowed at this energy: the density knob of the pattern.
    const auto threshold = 1.0f - e * 0.96f;

    switch (st)
    {
        case 0: // GROOVE: metrical ladder. Soft = pulse, hard = every sixteenth.
        {
            s.on = (w >= threshold);
            s.mode = Mode::live;
            s.len = s.accent ? 1.0f : (w >= 0.5f ? 0.84f : 0.64f);
            if (e > 0.80f && (i & 1) == 1)
                s.extraRepeats = 1;
            break;
        }

        case 1: // EUCLID: pulse count scales smoothly with the dynamics.
        {
            const auto minPulses = std::max (2, total / 8);
            const auto span = (float) (total - minPulses);
            const auto pulses = std::clamp (
                minPulses + (int) std::lround (std::pow (e, 1.30f) * span),
                1, total);

            const auto cur = (i * pulses) / total;
            const auto prev = (i == 0) ? -1 : (((i - 1) * pulses) / total);
            s.on = (cur != prev);
            s.mode = (e > 0.84f && ! s.accent && (i % 3 == 0)) ? Mode::scatter : Mode::live;
            s.len = s.accent ? 1.0f : 0.78f;
            if (e > 0.88f && ! s.accent)
                s.extraRepeats = 1;
            break;
        }

        case 2: // SYNCOP: downbeat anchors, anticipations wake up with the dynamics.
        {
            const auto offbeatScore = (i == 0) ? 1.0f
                                               : (((i & 1) == 1) ? (0.74f - 0.16f * (1.0f - w)) : w * 0.66f);
            s.on = (offbeatScore >= threshold);
            s.mode = (e > 0.74f && (i % 4 == 3)) ? Mode::onset : Mode::live;
            s.len = ((i & 1) == 1) ? 0.65f : 0.95f;
            break;
        }

        case 3: // ROLL: clean head of the bar, the tail turns into fills as dynamics climb.
        {
            const auto barPos = (float) i / (float) std::max (1, total - 1);
            const auto rollBoost = barPos * e * 0.85f;
            s.on = (w + rollBoost >= (0.98f - e * 0.86f));
            s.mode = (barPos > 0.6f && e > 0.70f) ? Mode::hold : Mode::live;
            s.len = barPos > 0.55f ? (1.0f - 0.5f * e) : 1.0f;
            if (barPos > 0.65f && e > 0.58f)
                s.extraRepeats = (e > 0.84f) ? 2 : 1;
            break;
        }

        case 4: // GHOST: sparse onset triggers when soft, frantic scatter when loud.
        {
            s.on = (w >= threshold);

            if (! s.on)
                break;

            if (e < 0.34f)
                s.mode = (i == 0) ? Mode::live : Mode::onset;
            else if (e < 0.70f)
                s.mode = (i % 4 == 2) ? Mode::scatter : ((i & 1) ? Mode::onset : Mode::live);
            else
            {
                s.mode = (i % 4 == 3) ? Mode::hold : ((i % 2 == 1) ? Mode::scatter : Mode::onset);
                s.reverse = (i % 6 == 4);
                s.extraRepeats = (i % 4 == 3) ? 1 : 0;
            }
            s.len = s.accent ? 0.95f : 0.68f;
            break;
        }

        case 5: // BACK: forward chops when soft, alternating reverse slices when loud.
        {
            s.on = (w >= threshold);
            s.mode = Mode::live;
            s.reverse = s.on && ! s.accent && (e > 0.45f) && (((i & 1) == 1) || (e > 0.78f && i % 4 == 2));
            s.len = s.reverse ? 0.85f : 1.0f;
            break;
        }

        case 6: // TREM: the chopper. Every step fires more as the dynamics rise.
        {
            s.on = (w >= threshold);
            s.mode = Mode::live;
            s.len = std::clamp (1.0f - e * 0.55f + (s.accent ? 0.2f : 0.0f), 0.22f, 1.0f);
            if (e > 0.78f && ! s.accent)
                s.extraRepeats = 1;
            break;
        }

        case 7: // INVERT: the groove grid driven by inverted energy.
        {
            s.on = (w >= threshold);
            s.mode = (e > 0.76f && (i % 4 == 2)) ? Mode::scatter : Mode::live;
            s.len = s.accent ? 1.0f : 0.75f;
            if (e > 0.84f && (i & 1) == 1)
                s.extraRepeats = 1;
            break;
        }
    }

    // ---- complexity gate: a weak hand only ever plays the simple version -----
    switch (tier)
    {
        case Tier::pulse:
            // Only the strong beats survive, everything else is silence.
            s.on = s.on && (w >= 0.74f);
            s.mode = Mode::live;
            s.reverse = false;
            s.extraRepeats = 0;
            s.len = 1.0f;
            break;

        case Tier::groove:
            s.mode = Mode::live;
            s.reverse = false;
            s.extraRepeats = 0;
            break;

        case Tier::syncop:
            if (s.mode == Mode::hold)
                s.mode = Mode::live;
            s.reverse = false;
            s.extraRepeats = 0;
            break;

        case Tier::shred:
            break;
    }

    if (! s.on)
    {
        s.mode = Mode::live;
        s.reverse = false;
        s.extraRepeats = 0;
    }

    return s;
}

// What the editor is allowed to look at. Plain atomics, written from the audio thread,
// read from the message thread: no locks, no allocation.
struct Snapshot
{
    std::atomic<int> step { 0 };
    std::atomic<int> gateOpen { 0 };
    std::atomic<float> envelope { 0.0f };
    std::atomic<float> velocity { 0.0f };
    std::atomic<float> patternEnergy { 0.5f };
    std::atomic<float> sliceFraction { 0.0f };
    std::atomic<int> onsets { 0 };
    std::atomic<int> clock { 0 };
    std::atomic<int> timeSig { 0 };
    std::atomic<int> style { 0 };
    std::atomic<int> patternLength { 16 };
    std::atomic<float> head { 0.0f };
    std::atomic<float> stepProgress { 0.0f };
};

class Slicer
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;

        left.assign ((std::size_t) kCaptureSize, 0.0f);
        right.assign ((std::size_t) kCaptureSize, 0.0f);

        attackCoef = 1.0f - std::exp (-1.0f / (float) (0.0008 * sampleRate));
        releaseCoef = 1.0f - std::exp (-1.0f / (float) (0.085 * sampleRate));
        velDecay = std::exp (-1.0f / (float) (0.240 * sampleRate));
        minStepSamples = 0.020 * sampleRate;
        maxStepSamples = 1.4 * sampleRate;

        reset();
    }

    void reset()
    {
        std::fill (left.begin(), left.end(), 0.0f);
        std::fill (right.begin(), right.end(), 0.0f);

        writeIndex = 0;
        lastOnsetPos = 0;
        holdPos = 0;
        envelope = 0.0f;
        previousEnvelope = 0.0f;
        velocity = 0.0f;
        refractory = 0;
        stepPhase = 0.0;
        stepLength = sampleRate * 0.125;
        stepCount = 0;
        patternIndex = 0;
        sliceRemaining = 0.0;
        sliceAge = 0.0;
        sliceFadeIn = 0.0;
        sliceFadeOut = 0.0;
        sliceLength = 0.0;
        readPos = 0.0;
        readInc = 1.0;
        repeatPos = 0.0;
        repeatsLeft = 0;
        passLiveDirect = false;
        currentStepStartWrite = 0;
        toneStateL = 0.0f;
        toneStateR = 0.0f;
        toneCoef = 0.5f;
        pendingReset = false;
        wasTransportPlaying = false;

        onsetCount = 0;
        lastSliceFraction = 0.0f;
        lastEnergy = 0.5f;
        scopeAccumulator = 0.0;
        lastBlockSamples = 0;
        rng = 0x9e3779b9u;

        snapshot.step.store (0);
        snapshot.gateOpen.store (0);
        snapshot.envelope.store (0.0f);
        snapshot.velocity.store (0.0f);
        snapshot.patternEnergy.store (0.5f);
        snapshot.sliceFraction.store (0.0f);
        snapshot.onsets.store (0);
        snapshot.clock.store (settings.clock);
        snapshot.timeSig.store (settings.timeSig);
        snapshot.style.store (settings.style);
        snapshot.patternLength.store (stepsForTimeSig (settings.timeSig));
        snapshot.head.store (0.0f);
        snapshot.stepProgress.store (0.0f);
    }

    void setSettings (const Settings& s) { settings = s; }

    void setTransport (double bpm, bool playing) noexcept
    {
        transportBpm = (bpm > 20.0 && bpm < 320.0) ? bpm : 120.0;

        // Re-align bar downbeat when host playback starts.
        if (playing && ! wasTransportPlaying && settings.clock == 0)
        {
            stepPhase = stepLength;
            stepCount = 0;
        }

        wasTransportPlaying = playing;
        transportPlaying = playing;
    }

    void process (float* leftChannel, float* rightChannel, int numSamples) noexcept
    {
        if (left.empty())
            return;

        if (pendingReset.exchange (false))
            reset();

        const auto div = kDivisionBeats[(std::size_t) std::clamp (settings.rateIndex, 0, 7)];
        const auto bpm = transportBpm;
        const auto hostStep = sampleRate * 60.0 / bpm * div;
        const auto freeStep = sampleRate * 60.0 / 120.0 * div;

        const auto sens = std::clamp (settings.sens, 0.0f, 1.0f);
        const auto sensGain = 0.8f + sens * 5.0f;
        const auto threshold = 0.005f + (1.0f - sens) * 0.14f;
        const auto length = stepsForTimeSig (settings.timeSig);

        const auto mix = std::clamp (settings.mix, 0.0f, 1.0f);
        const auto gain = std::clamp (settings.output, 0.0f, 2.0f);

        const auto toneCut = 180.0f * std::pow (140.0f, std::clamp (settings.tone, 0.0f, 1.0f));
        const auto toneTarget = (float) (1.0 - std::exp (-2.0 * 3.14159265 * toneCut / sampleRate));
        toneCoef += (toneTarget - toneCoef) * 0.05f;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto inL = leftChannel[i];
            const auto inR = rightChannel != nullptr ? rightChannel[i] : inL;

            // ---- envelope, velocity & onset ----------------------------------------
            const auto peak = std::max (std::abs (inL), std::abs (inR));
            const auto coef = peak > envelope ? attackCoef : releaseCoef;
            envelope += (peak - envelope) * coef;

            const auto isOnset = envelope > threshold && previousEnvelope <= threshold
                                 && refractory <= 0;
            previousEnvelope = envelope;

            if (refractory > 0)
                --refractory;

            // Perceptual velocity: quiet playing stays near zero, hard playing tops out
            // smoothly instead of clipping, so the pattern has room to breathe.
            const auto scaledEnv = 1.0f - std::exp (-envelope * sensGain);
            velocity = std::max (velocity * velDecay, scaledEnv);

            // ---- capture -----------------------------------------------------------
            const auto w = writeIndex & kCaptureMask;
            left[(std::size_t) w] = inL;
            right[(std::size_t) w] = inR;
            ++writeIndex;

            if (isOnset)
            {
                lastOnsetPos = writeIndex - 1;
                refractory = (int) (0.028 * sampleRate);
                ++onsetCount;
            }

            // ---- clock -------------------------------------------------------------
            if (settings.clock == 1)   // input transients advance the time-sig grid
            {
                if (isOnset && stepPhase > minStepSamples)
                {
                    stepLength = std::clamp (stepPhase, minStepSamples, maxStepSamples);
                    startStep (length, hostStep);
                    stepPhase = 0.0;
                }
                else
                {
                    stepPhase += 1.0;
                }
            }
            else
            {
                const auto target = settings.clock == 0 ? hostStep : freeStep;
                stepLength += (target - stepLength) * 0.0008;
                stepPhase += 1.0;

                if (stepPhase >= stepLength)
                {
                    stepPhase -= stepLength;
                    startStep (length, target);
                }
            }

            // ---- slice player (zero-latency live gate OR captured repeat/rev) ------
            float wetL = 0.0f;
            float wetR = 0.0f;

            if (sliceRemaining > 0.0)
            {
                if (passLiveDirect)
                {
                    // Direct real-time Boss Slicer chop: zero latency on live signal.
                    wetL = inL;
                    wetR = inR;
                }
                else
                {
                    const auto pos = (std::int64_t) std::llround (readPos);
                    const auto idx = pos & kCaptureMask;
                    wetL = left[(std::size_t) idx];
                    wetR = right[(std::size_t) idx];
                    readPos += readInc;
                }

                auto fade = 1.0f;

                if (sliceAge < sliceFadeIn)
                    fade = (float) (sliceAge / std::max (1.0, sliceFadeIn));

                const auto toEnd = sliceRemaining;

                if (toEnd < sliceFadeOut)
                    fade *= (float) (toEnd / std::max (1.0, sliceFadeOut));

                wetL *= fade;
                wetR *= fade;

                sliceAge += 1.0;
                --sliceRemaining;

                if (sliceRemaining <= 0.0 && repeatsLeft > 0 && sliceLength > 1.0)
                {
                    --repeatsLeft;
                    sliceRemaining = sliceLength;
                    sliceAge = 0.0;
                    passLiveDirect = false;   // ratchets replay the slice we just captured
                    readPos = repeatPos;
                }
            }

            // ---- tone, mix, output -------------------------------------------------
            toneStateL += (wetL - toneStateL) * toneCoef;
            toneStateR += (wetR - toneStateR) * toneCoef;

            const auto outL = (inL * (1.0f - mix) + toneStateL * mix) * gain;
            const auto outR = (inR * (1.0f - mix) + toneStateR * mix) * gain;

            leftChannel[i] = outL;

            if (rightChannel != nullptr)
                rightChannel[i] = outR;

            if ((writeIndex & 31) == 0)
            {
                snapshot.envelope.store (scaledEnv, std::memory_order_relaxed);
                snapshot.velocity.store (velocity, std::memory_order_relaxed);
                snapshot.patternEnergy.store (lastEnergy, std::memory_order_relaxed);
                snapshot.gateOpen.store (sliceRemaining > 0.0 ? 1 : 0, std::memory_order_relaxed);

                const auto window = (double) sampleRate * 0.7;
                const auto headPos = passLiveDirect ? writeIndex : (std::int64_t) readPos;
                const auto behind = (double) (writeIndex - headPos);
                snapshot.head.store ((float) std::clamp (1.0 - behind / window, 0.0, 1.0),
                                     std::memory_order_relaxed);
                snapshot.stepProgress.store ((float) std::clamp (stepPhase / std::max (1.0, stepLength),
                                                                 0.0, 1.0),
                                             std::memory_order_relaxed);
            }
        }

        lastBlockSamples = numSamples;

        snapshot.step.store (patternIndex, std::memory_order_relaxed);
        snapshot.onsets.store (onsetCount, std::memory_order_relaxed);
        snapshot.sliceFraction.store (lastSliceFraction, std::memory_order_relaxed);
        snapshot.clock.store (settings.clock, std::memory_order_relaxed);
        snapshot.timeSig.store (settings.timeSig, std::memory_order_relaxed);
        snapshot.style.store (settings.style, std::memory_order_relaxed);
        snapshot.patternLength.store (length, std::memory_order_relaxed);
    }

    const Snapshot& getSnapshot() const noexcept { return snapshot; }

    void updateScope (float* dest, int count) noexcept
    {
        scopeAccumulator += (double) lastBlockSamples;

        if (scopeAccumulator < sampleRate * 0.05)
            return;

        scopeAccumulator = 0.0;
        fillScope (dest, count);
    }

    void fillScope (float* dest, int count) const noexcept
    {
        if (count <= 0 || left.empty())
            return;

        const auto window = std::min ((std::int64_t) (sampleRate * 0.7), writeIndex);
        const auto start = writeIndex - window;

        for (int i = 0; i < count; ++i)
        {
            const auto a = start + window * i / count;
            const auto b = start + window * (i + 1) / count;

            float peak = 0.0f;

            for (auto s = a; s < b; ++s)
                peak = std::max (peak, std::abs (left[(std::size_t) (s & kCaptureMask)]));

            dest[i] = std::min (1.0f, peak * 1.4f);
        }
    }

    void requestReset() noexcept { pendingReset = true; }

    double getSampleRate() const noexcept { return sampleRate; }

private:
    void startStep (int length, double activeStepSamples) noexcept
    {
        const auto s = settings;

        ++stepCount;
        patternIndex = (int) ((stepCount - 1) % length);

        const auto v = std::clamp ((float) velocity, 0.0f, 1.0f);
        lastEnergy = dynamicPatternEnergy (v, s.dynPat, s.style);

        const auto step = evaluateStep (s.timeSig, s.style, patternIndex, lastEnergy);

        sliceRemaining = 0.0;
        repeatsLeft = 0;
        passLiveDirect = false;

        if (! step.on)
        {
            holdPos = writeIndex;
            return;
        }

        // Dynamics tighten slice length and add ratchets on top of the generated pattern.
        const auto dynS = std::clamp (s.dynSlice, 0.0f, 1.0f);
        const auto dynR = std::clamp (s.dynRate, 0.0f, 1.0f);

        auto fraction = std::clamp (s.slice, 0.03f, 1.0f) * step.len;
        fraction *= 1.0f - 0.68f * v * dynS;
        fraction = std::clamp (fraction, 0.02f, 1.0f);

        const auto baseStep = s.clock == 1 ? stepLength : activeStepSamples;
        auto piece = baseStep * fraction;
        piece = std::clamp (piece, 24.0, (double) kCaptureSize * 0.9);

        auto repeats = 1 + step.extraRepeats
                       + (int) std::lround (std::clamp (s.repeat, 0.0f, 1.0f) * 3.0f
                                            + v * dynR * 3.0f);
        repeats = std::clamp (repeats, 1, 8);

        const auto playBackwards = step.reverse
                                   || (s.reverse > 0.001f && random01() < s.reverse);

        auto mode = step.mode;

        if (mode != Mode::hold && mode != Mode::onset && s.scatter > 0.001f
            && random01() < s.scatter)
            mode = Mode::scatter;

        sliceLength = piece / (double) repeats;
        sliceAge = 0.0;
        sliceFadeIn = std::min (sampleRate * 0.003, sliceLength * 0.35);
        sliceFadeOut = std::min (sampleRate * 0.005, sliceLength * 0.45);
        sliceRemaining = sliceLength;
        repeatsLeft = repeats - 1;

        const auto captureWindow = std::min ((std::int64_t) (sampleRate * 1.2), writeIndex);

        if (mode == Mode::live && ! playBackwards)
        {
            // Zero-latency live slice: pass 0 gates live audio while recording at
            // `writeIndex - 1`, so any ratchet repeats replay this exact slice!
            passLiveDirect = true;
            currentStepStartWrite = std::max<std::int64_t> (0, writeIndex - 1);
            holdPos = currentStepStartWrite;
            readPos = (double) currentStepStartWrite;
            readInc = 1.0;
            repeatPos = (double) currentStepStartWrite;
        }
        else
        {
            passLiveDirect = false;
            std::int64_t startPos = std::max<std::int64_t> (0, writeIndex - (std::int64_t) sliceLength);

            switch (mode)
            {
                case Mode::live:
                    startPos = std::max<std::int64_t> (0, writeIndex - (std::int64_t) sliceLength);
                    break;

                case Mode::scatter:
                    startPos = captureWindow > (std::int64_t) sliceLength
                                   ? writeIndex - (std::int64_t) sliceLength
                                     - (std::int64_t) (random01() * (float) (captureWindow - (std::int64_t) sliceLength))
                                   : 0;
                    break;

                case Mode::onset:
                    startPos = lastOnsetPos;
                    break;

                case Mode::hold:
                    startPos = holdPos > 0 ? holdPos
                                           : std::max<std::int64_t> (0, writeIndex - (std::int64_t) sliceLength);
                    break;
            }

            if (mode != Mode::hold)
                holdPos = startPos;

            startPos = std::max<std::int64_t> (0, startPos);

            if (playBackwards)
            {
                readPos = (double) (startPos + (std::int64_t) sliceLength);
                readInc = -1.0;
            }
            else
            {
                readPos = (double) startPos;
                readInc = 1.0;
            }

            repeatPos = readPos;
        }

        lastSliceFraction = (float) fraction;
        lastVelocity = v;
    }

    float random01() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) ((rng >> 8) & 0xffffff) / (float) 0xffffff;
    }

    Settings settings;
    Snapshot snapshot;

    std::vector<float> left, right;

    double sampleRate = 48000.0;
    double transportBpm = 120.0;
    bool transportPlaying = false;
    bool wasTransportPlaying = false;

    std::int64_t writeIndex = 0;
    std::int64_t lastOnsetPos = 0;
    std::int64_t holdPos = 0;
    std::int64_t currentStepStartWrite = 0;

    float envelope = 0.0f;
    float previousEnvelope = 0.0f;
    float velocity = 0.0f;
    float attackCoef = 0.01f;
    float releaseCoef = 0.001f;
    float velDecay = 0.999f;
    int refractory = 0;

    double stepPhase = 0.0;
    double stepLength = 6000.0;
    double minStepSamples = 960.0;
    double maxStepSamples = 67200.0;

    int stepCount = 0;
    int patternIndex = 0;
    int onsetCount = 0;

    double sliceRemaining = 0.0;
    double sliceAge = 0.0;
    double sliceLength = 0.0;
    double sliceFadeIn = 0.0;
    double sliceFadeOut = 0.0;
    double readPos = 0.0;
    double readInc = 1.0;
    double repeatPos = 0.0;
    int repeatsLeft = 0;
    bool passLiveDirect = false;

    float toneStateL = 0.0f;
    float toneStateR = 0.0f;
    float toneCoef = 0.5f;

    float lastSliceFraction = 0.0f;
    float lastVelocity = 0.0f;
    float lastEnergy = 0.5f;

    std::uint32_t rng = 0x9e3779b9u;
    std::atomic<bool> pendingReset { false };

    double scopeAccumulator = 0.0;
    int lastBlockSamples = 0;
};
} // namespace mauri
