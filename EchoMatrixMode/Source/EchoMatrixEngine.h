#pragma once

// EchoMatrixMode engine - Phase 1 usable core.
//
//   input ---> [ memory delay line ] ---> 16 step sequencer (one step per 16th)
//                      |                         |
//                      |                         +--> spawn an independent
//                      |                              EventVoice (own level,
//                      |                              own pan, own envelope,
//                      |                              own lifetime)
//                      |
//                      +--> feedback loop (HP -> LP -> drive -> feedback)
//                              |
//                              v
//   output = dry * (1 - mix) + wet * mix
//
// Every repeat is an independent voice: spawning a new repeat never touches the
// state of the repeats that are already ringing. The pool is bounded (64
// voices) and the oldest voice is stolen when it overflows, so the CPU cost is
// predictable no matter what the pattern does.
//
// No JUCE dependencies: framework-free for unit testing.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace echomatrix
{

//==============================================================================
// Stereo delay line with linear interpolation. The capacity is chosen at
// prepare() time from the maximum delay time and the sample rate.
class DelayLine
{
public:
    void setCapacity (int numSamples, int numChannels)
    {
        capacity = std::max (1, numSamples);
        channels = std::max (1, numChannels);
        buffer.assign ((std::size_t) channels * (std::size_t) capacity, 0.0f);
        writePos.assign ((std::size_t) channels, 0);
    }

    void write (int channel, float value)
    {
        if (channel < 0 || channel >= channels || capacity <= 0)
            return;

        buffer[(std::size_t) channel * (std::size_t) capacity
               + (std::size_t) writePos[(std::size_t) channel]] = value;

        writePos[(std::size_t) channel] = (writePos[(std::size_t) channel] + 1) % capacity;
    }

    // Read with linear interpolation `delaySamples` in the past.
    float read (int channel, double delaySamples) const
    {
        if (capacity <= 0 || channel < 0 || channel >= channels)
            return 0.0f;

        // The read position is always inside one lap of the buffer, so the
        // wrap is a single add or subtract: no modulo, no allocation.
        auto pos = (double) writePos[(std::size_t) channel] - delaySamples;

        if (pos < 0.0)
            pos += (double) capacity;
        else if (pos >= (double) capacity)
            pos -= (double) capacity;

        const auto i0 = (int) pos;                 // pos is never negative here
        const auto i1 = (i0 + 1) % capacity;
        const auto frac = (float) (pos - (double) i0);

        const auto s0 = buffer[(std::size_t) channel * (std::size_t) capacity + (std::size_t) i0];
        const auto s1 = buffer[(std::size_t) channel * (std::size_t) capacity + (std::size_t) i1];

        return s0 + frac * (s1 - s0);
    }

    void clear()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
    }

    int getCapacity() const noexcept { return capacity; }

private:
    int capacity = 0;
    int channels = 0;
    std::vector<float> buffer;
    std::vector<int> writePos;
};

//==============================================================================
// One repeat: an independently controllable audio event spawned by the
// sequencer. It reads the memory at its own offset, applies its own gain and
// pan, decays over its own lifetime and then dies. It never touches another
// event's state.
struct EventVoice
{
    bool active = false;

    double pending = 0.0;         // samples to wait before the grain opens
    double remainingSamples = 0.0;  // life left, in samples
    double lifetimeSamples = 1.0;   // total life of the grain, in samples
    double elapsed = 0.0;           // samples since the grain opened

    double tapOffset = 0.0;         // how far in the past this voice reads
    double readRate = 1.0;          // 1.0 = original pitch (Phase 2 uses this)

    float level = 0.7f;
    float pan = 0.0f;               // -1 = left, 0 = centre, 1 = right

    float feedbackSend = 0.0f;      // this voice's own share of the loop
};

//==============================================================================
// One sequencer step.
struct Step
{
    bool enabled = false;
    float level = 0.7f;
    float pan = 0.0f;
    float timing = 1.0f;   // multiplier on the delay time (Phase 2 exposes it)
};

//==============================================================================
// One-pole filter, used for the high-pass and the low-pass of the feedback path.
class OnePole
{
public:
    void setCutoff (double hz, double sampleRate) noexcept
    {
        const auto clamped = std::clamp (hz, 10.0, sampleRate * 0.45);
        coeff = (float) (1.0 - std::exp (-2.0 * 3.14159265358979 * clamped / sampleRate));
    }

    float lowpass (float x) noexcept
    {
        state += (double) coeff * ((double) x - state);
        return (float) state;
    }

    // The same pole used as a high-pass: x - lowpass(x).
    float highpass (float x) noexcept
    {
        const auto l = lowpass (x);
        return x - l;
    }

    void reset() noexcept { state = 0.0; }

private:
    double state = 0.0;
    float coeff = 1.0f;
};

//==============================================================================
struct Settings
{
    float delayTime = 0.5f;      // seconds (manual mode)
    bool  syncEnabled = false;   // follow the host tempo
    float feedback = 0.45f;      // 0 .. 0.99
    float mix = 0.5f;            // dry/wet
    float inputGain = 1.0f;
    float outputGain = 1.0f;
    float hpfCutoff = 20.0f;     // Hz, feedback path
    float lpfCutoff = 12000.0f;  // Hz, feedback path
    float drive = 0.0f;          // 0 .. 1, saturation in the feedback path

    Step steps[16];
};

//==============================================================================
struct Snapshot
{
    std::atomic<int>   activeEvents { 0 };   // voices currently sounding
    std::atomic<float> wetLevel { 0.0f };    // wet peak, 0..1
    std::atomic<int>   currentStep { 0 };    // the step under the playhead
    std::atomic<float> stepPhase { 0.0f };   // 0..1 inside the current step
    std::atomic<float> bpm { 0.0f };         // the tempo reported by the host
};

//==============================================================================
class Engine
{
public:
    static constexpr int kMaxEvents = 64;
    static constexpr int kNumSteps = 16;

    // The pattern runs in sixteenths: four steps make one delay period.
    static constexpr int kStepsPerPeriod = 4;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const auto maxDelaySamples = (int) (2.2 * sampleRate); // 2 s max + headroom
        lines[0].setCapacity (maxDelaySamples, 2);
        lines[1].setCapacity (maxDelaySamples, 2);
        lines[0].clear();
        lines[1].clear();

        events.assign ((std::size_t) kMaxEvents, EventVoice{});
        nextEventIndex = 0;

        hpfL.reset(); hpfR.reset();
        lpfL.reset(); lpfR.reset();
        hpfL.setCutoff (settings.hpfCutoff, sampleRate);
        lpfR.setCutoff (settings.hpfCutoff, sampleRate);
        lpfL.setCutoff (settings.lpfCutoff, sampleRate);
        lpfR.setCutoff (settings.lpfCutoff, sampleRate);

        samplesSinceTick = 0.0;
        stepIndex = 0;
        started = false;
        activeCountSmoothed = 1.0;
        wetPeak = 0.0f;

        snapshot.activeEvents.store (0);
        snapshot.wetLevel.store (0.0f);
        snapshot.currentStep.store (0);
        snapshot.stepPhase.store (0.0f);
    }

    void reset()
    {
        lines[0].clear();
        lines[1].clear();

        for (auto& e : events)
            e = EventVoice{};

        nextEventIndex = 0;
        hpfL.reset(); hpfR.reset();
        lpfL.reset(); lpfR.reset();
        samplesSinceTick = 0.0;
        stepIndex = 0;
        started = false;
        activeCountSmoothed = 1.0;
        wetPeak = 0.0f;

        snapshot.activeEvents.store (0);
        snapshot.wetLevel.store (0.0f);
    }

    void setSettings (const Settings& s) noexcept { settings = s; }
    const Settings& getSettings() const noexcept { return settings; }
    const Snapshot& getSnapshot() const noexcept { return snapshot; }

    // How many voices are alive right now (used by the tests).
    int getActiveVoiceCount() const noexcept
    {
        int count = 0;

        for (const auto& ev : events)
            if (ev.active)
                ++count;

        return count;
    }

    // Called by the processor when the transport BPM is available.
    void setBpm (double bpm) noexcept
    {
        hostBpm = bpm;
        snapshot.bpm.store ((float) bpm, std::memory_order_relaxed);
    }

    void process (const float* const* input, float* const* output,
                  int numChannels, int numSamples) noexcept
    {
        if (numChannels <= 0 || numSamples <= 0 || input == nullptr || output == nullptr)
            return;

        const auto& s = settings;
        const auto stereo = numChannels > 1;

        // Effective delay time: manual, or locked to the host tempo.
        double delaySeconds = (double) s.delayTime;

        if (s.syncEnabled && hostBpm > 0.0)
            delaySeconds = 60.0 / hostBpm;

        delaySeconds = std::clamp (delaySeconds, 0.005, 2.0);

        const auto delaySamples = delaySeconds * sr;
        const auto tickSamples = delaySamples / (double) kStepsPerPeriod;

        const auto dry = std::clamp (s.inputGain, 0.0f, 2.0f);
        const auto wetMix = std::clamp (s.mix, 0.0f, 1.0f);
        const auto outGain = std::clamp (s.outputGain, 0.0f, 2.0f);
        const auto fb = std::clamp (s.feedback, 0.0f, 0.98f);
        const auto drive = std::clamp (s.drive, 0.0f, 1.0f);

        hpfL.setCutoff ((double) s.hpfCutoff, sr);
        hpfR.setCutoff ((double) s.hpfCutoff, sr);
        lpfL.setCutoff ((double) s.lpfCutoff, sr);
        lpfR.setCutoff ((double) s.lpfCutoff, sr);

        const auto maxRead = (double) (lines[0].getCapacity() - 2);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto inL = input[0][i] * dry;
            const auto inR = stereo ? input[1][i] * dry : inL;

            // ---- the very first step fires straight away ----
            if (! started)
            {
                started = true;
                spawnEventFromCurrentStep (delaySamples, tickSamples);
                stepIndex = 1 % kNumSteps;
            }

            // ---- sequencer: one step every sixteenth of the delay period ----
            samplesSinceTick += 1.0;

            if (samplesSinceTick >= tickSamples)
            {
                samplesSinceTick -= tickSamples;
                spawnEventFromCurrentStep (delaySamples, tickSamples);
                stepIndex = (stepIndex + 1) % kNumSteps;
            }

            snapshot.currentStep.store (stepIndex, std::memory_order_relaxed);
            snapshot.stepPhase.store ((float) std::clamp (samplesSinceTick / tickSamples, 0.0, 1.0),
                                      std::memory_order_relaxed);

            // ---- every independent voice plays its own grain ----
            float wetL = 0.0f;
            float wetR = 0.0f;
            int alive = 0;

            for (auto& ev : events)
            {
                if (! ev.active)
                    continue;

                ++alive;

                // The grain is still travelling: it opens once the delay time
                // has passed, and only then does it become audible.
                if (ev.pending > 0.0)
                {
                    ev.pending -= 1.0;
                    continue;
                }

                const auto life = std::clamp (ev.remainingSamples / ev.lifetimeSamples, 0.0, 1.0);
                const auto env = (float) life * (0.55f + 0.45f * (float) life);
                const auto gain = ev.level * env;

                const auto panRad = ((double) ev.pan + 1.0) * 3.14159265358979 / 4.0;
                const auto leftGain = gain * (float) std::cos (panRad);
                const auto rightGain = gain * (float) std::sin (panRad);

                auto readOffset = ev.tapOffset - (ev.readRate - 1.0) * ev.elapsed;
                readOffset = std::clamp (readOffset, 1.0, maxRead);

                const auto eL = lines[0].read (0, readOffset);
                const auto eR = lines[1].read (1, readOffset);

                wetL += eL * leftGain;
                wetR += eR * rightGain;

                ev.elapsed += 1.0;
                ev.remainingSamples -= 1.0;

                if (ev.remainingSamples <= 0.0)
                    ev.active = false;
            }

            // A smooth count of the sounding voices, for the panel.
            activeCountSmoothed += ((double) alive - activeCountSmoothed) * 0.002;

            // ---- feedback loop: one global ring that keeps the memory alive ----
            const auto srcL = lines[0].read (0, delaySamples);
            const auto srcR = lines[1].read (1, delaySamples);

            auto fbL = lpfL.lowpass (hpfL.highpass (srcL)) * fb;
            auto fbR = lpfR.lowpass (hpfR.highpass (srcR)) * fb;

            auto writeL = inL + fbL;
            auto writeR = inR + fbR;

            if (drive > 0.0f)
            {
                const auto d = 1.0f + drive * 12.0f;
                writeL = (float) std::tanh (writeL * d) / d;
                writeR = (float) std::tanh (writeR * d) / d;
            }

            lines[0].write (0, writeL);
            lines[1].write (1, writeR);

            // ---- mix ----
            output[0][i] = (inL * (1.0f - wetMix) + wetL * wetMix) * outGain;
            output[1][i] = (inR * (1.0f - wetMix) + wetR * wetMix) * outGain;

            wetPeak = std::max (wetPeak, std::abs (wetL));
            wetPeak = std::max (wetPeak, std::abs (wetR));
        }

        snapshot.activeEvents.store (activeCountSmoothed >= 0.5
                                        ? (int) (activeCountSmoothed + 0.5) : 0,
                                    std::memory_order_relaxed);
        snapshot.wetLevel.store (std::min (wetPeak, 1.0f), std::memory_order_relaxed);
    }

private:
    void spawnEventFromCurrentStep (double delaySamples, double tickSamples)
    {
        const auto& step = settings.steps[stepIndex];

        if (! step.enabled)
            return;

        auto* slot = findFreeEvent();

        if (slot == nullptr)
            slot = findOldestEvent();

        if (slot == nullptr)
            return;

        const auto tap = std::clamp (delaySamples * (double) step.timing, 1.0,
                                     (double) (lines[0].getCapacity() - 2));

        slot->active = true;
        slot->lifetimeSamples = std::max (1.0, tickSamples * 2.0);
        slot->remainingSamples = slot->lifetimeSamples;
        slot->elapsed = 0.0;
        slot->readRate = 1.0;
        slot->tapOffset = tap;
        // The grain waits for the delay time, then opens on the material that
        // was captured when the step fired.
        slot->pending = tap;
        slot->level = std::clamp (step.level, 0.0f, 2.0f);
        slot->pan = std::clamp (step.pan, -1.0f, 1.0f);
        slot->feedbackSend = 0.0f;
    }

    EventVoice* findFreeEvent() noexcept
    {
        for (auto& ev : events)
            if (! ev.active)
                return &ev;

        return nullptr;
    }

    EventVoice* findOldestEvent() noexcept
    {
        // Round-robin stealing: the pool never grows, so the CPU is bounded.
        auto* ev = &events[(std::size_t) nextEventIndex];
        nextEventIndex = (nextEventIndex + 1) % kMaxEvents;
        ev->active = false;
        return ev;
    }

    double sr = 48000.0;
    double hostBpm = 0.0;

    DelayLine lines[2];
    std::vector<EventVoice> events;
    int nextEventIndex = 0;

    int stepIndex = 0;
    double samplesSinceTick = 0.0;
    bool started = false;

    Settings settings;
    Snapshot snapshot;

    OnePole hpfL, hpfR, lpfL, lpfR;

    double activeCountSmoothed = 1.0;
    float wetPeak = 0.0f;
};

} // namespace echomatrix
