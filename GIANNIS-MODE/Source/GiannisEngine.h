#pragma once

// GIANNIS-MODE engine.
//
// No MIDI, no samples: the input audio drives an analog style octave divider and
// a couple of sine sub voices, then everything is glued with a fuzz stage.
//
//   input --> low pass --> Schmitt trigger --> divide by 2 --> /2 sub
//                                          --> divide by 4 --> /4 sub ("doom")
//         --> full wave rectifier --------------------------> +1 octave
//         --> dry -----------------------------------------> voice
//   voice --> fuzz --> tone --> + sub + doom --> limiter --> out
//
// The engine is deliberately framework free so the DSP can be measured in a
// plain console executable.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string>

namespace giannis
{

//==============================================================================
// A one pole smoother, used for envelopes and gentle tone shaping.
struct OnePole
{
    void setCutoff (double hz, double sampleRate) noexcept
    {
        const auto clamped = std::clamp (hz, 0.1, sampleRate * 0.45);
        coeff = float (1.0 - std::exp (-2.0 * 3.14159265358979 * clamped / sampleRate));
    }

    float process (float x) noexcept
    {
        state += double (coeff) * (double (x) - state);
        return float (state);
    }

    void reset() noexcept { state = 0.0; }

    double state = 0.0;
    float coeff = 1.0f;
};

//==============================================================================
// DC blocker: a one pole high pass at ~14 Hz. The divider and the rectifier both
// make DC, and DC in a bass plugin is a speaker travelling without a reason.
struct DcBlocker
{
    void prepare (double sampleRate) noexcept
    {
        r = float (std::exp (-2.0 * 3.14159265358979 * 14.0 / sampleRate));
    }

    float process (float x) noexcept
    {
        const auto y = r * (previousOut + x - previousIn);
        previousIn = x;
        previousOut = y;
        return y;
    }

    void reset() noexcept { previousIn = previousOut = 0.0f; }

    float r = 0.998f;
    float previousIn = 0.0f;
    float previousOut = 0.0f;
};

//==============================================================================
// Topology preserving state variable filter (Andy Simper's form). Stable when
// the cutoff moves, which matters because the sub filter tracks the note.
struct Svf
{
    void set (double hz, double q, double sampleRate) noexcept
    {
        const auto clamped = std::clamp (hz, 12.0, sampleRate * 0.45);
        g = float (std::tan (3.14159265358979 * clamped / sampleRate));
        k = float (1.0 / std::clamp (q, 0.20, 8.0));

        const auto first = 1.0f / (1.0f + g * (g + k));
        a1 = first;
        a2 = g * first;
        a3 = g * a2;
    }

    float lowpass (float x) noexcept
    {
        const auto v3 = x - ic2;
        const auto v1 = a1 * ic1 + a2 * v3;
        const auto v2 = ic2 + a2 * ic1 + a3 * v3;

        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        return v2;
    }

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    float g = 0.1f, k = 1.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

//==============================================================================
// The analog divider: the input is low passed so the harmonics stop crossing
// zero, a Schmitt trigger turns it into a square, and two flip flops give the
// octave down and the two octaves down. That is exactly what an OC-2 does, and
// it starts on the first cycle instead of waiting for an analysis window.
class Divider
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        inputFilter.setCutoff (320.0, sampleRate);
        peakDecay = float (std::exp (-1.0 / (0.12 * sampleRate)));
        reset();
    }

    void reset() noexcept
    {
        inputFilter.reset();
        peak = 0.0f;
        high = false;
        wasHigh = false;
        half = false;
        quarter = false;
        sinceEdge = 0;
        period = 0.0;
        frequency = 0.0f;
    }

    // Feed one sample.
    void push (float x) noexcept
    {
        peak = std::max (std::abs (x), peak * peakDecay);

        const auto conditioned = inputFilter.process (x);
        const auto hysteresis = std::clamp (0.10f * peak, 0.0004f, 0.05f);

        if (conditioned > hysteresis)
            high = true;
        else if (conditioned < -hysteresis)
            high = false;

        ++sinceEdge;

        if (high && ! wasHigh)
        {
            // A rising edge: every one flips the octave, every second one flips
            // the two octaves down.
            half = ! half;

            if (half)
                quarter = ! quarter;

            updatePeriod();
        }

        wasHigh = high;
    }

    float squareHalf() const noexcept { return half ? 1.0f : -1.0f; }
    float squareQuarter() const noexcept { return quarter ? 1.0f : -1.0f; }

    // The note being tracked, or 0 while there is nothing periodic to follow.
    float getFrequency() const noexcept { return frequency; }

    bool isRunning() const noexcept { return frequency > 0.0f; }

private:
    void updatePeriod() noexcept
    {
        if (sinceEdge > 2)
        {
            const auto measured = double (sinceEdge);

            if (period <= 0.0)
                period = measured;
            else
                period = period * 0.70 + measured * 0.30;

            sinceEdge = 0;
        }
        else
        {
            sinceEdge = 0;
            return;
        }

        const auto hz = float (sr / period);

        // Only believable notes steer the subs; a missed crossing must not send
        // them to 8 Hz or to 900 Hz.
        if (hz < 22.0f || hz > 420.0f)
        {
            frequency = 0.0f;
            return;
        }

        frequency = frequency <= 0.0f ? hz : frequency * 0.55f + hz * 0.45f;
    }

    double sr = 48000.0;
    OnePole inputFilter;
    float peakDecay = 0.9998f;
    float peak = 0.0f;
    bool high = false, wasHigh = false;
    bool half = false, quarter = false;
    int sinceEdge = 0;
    double period = 0.0;
    float frequency = 0.0f;
};

//==============================================================================
struct Settings
{
    float sub = 0.75f;     // -1 octave level
    float doom = 0.15f;    // -2 octaves level
    float oct = 0.30f;     // +1 octave level
    float fuzz = 0.25f;    // drive on the voice
    float tone = 0.50f;    // low pass on the fuzzed voice
    float level = 0.90f;   // output

    // The dry signal is always there at unity: this is a pedal, not a synth.
    // Tests set it to zero to look at the generated voices on their own.
    float dry = 1.0f;
};

//==============================================================================
struct Snapshot
{
    std::atomic<float> frequency { 0.0f };   // tracked note, Hz
    std::atomic<float> subLevel { 0.0f };    // how much sub is coming out, 0..1
    std::atomic<float> voiceLevel { 0.0f };  // how much voice is coming out, 0..1
    std::atomic<float> inputLevel { 0.0f };  // input envelope, 0..1
    std::atomic<int> note { -1 };            // MIDI note
    std::atomic<int> active { 0 };           // 1 while a note is tracked
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        divider.prepare (sampleRate);
        divider.reset();

        subShape[0].set (120.0, 0.54, sampleRate);
        subShape[1].set (120.0, 1.30, sampleRate);
        doomShape[0].set (70.0, 0.54, sampleRate);
        doomShape[1].set (70.0, 1.30, sampleRate);

        condense.setCutoff (18.0, sampleRate);
        attackCoeff = float (1.0 - std::exp (-1.0 / (0.004 * sampleRate)));
        releaseCoeff = float (1.0 - std::exp (-1.0 / (0.140 * sampleRate)));
        doomReleaseCoeff = float (1.0 - std::exp (-1.0 / (0.260 * sampleRate)));

        for (auto& channel : channels)
        {
            channel.octDc.prepare (sampleRate);
            channel.octSmooth.setCutoff (7000.0, sampleRate);
            channel.toneFilter.set (4000.0, 0.70, sampleRate);

            channel.octDc.reset();
            channel.octSmooth.reset();
            channel.toneFilter.reset();
        }

        subDc.prepare (sampleRate);
        subDc.reset();

        envelope = 0.0f;
        doomEnvelope = 0.0f;
        smoothedSubHz = 0.0f;
        reportCounter = 0;
        reportHz = 0.0f;
        subMeter = 0.0f;
        voiceMeter = 0.0f;

        snapshot.frequency.store (0.0f);
        snapshot.subLevel.store (0.0f);
        snapshot.voiceLevel.store (0.0f);
        snapshot.inputLevel.store (0.0f);
        snapshot.note.store (-1);
        snapshot.active.store (0);
    }

    void setSettings (const Settings& s) noexcept { settings = s; }

    const Settings& getSettings() const noexcept { return settings; }

    const Snapshot& getSnapshot() const noexcept { return snapshot; }

    float getTrackedFrequency() const noexcept { return divider.getFrequency(); }

    static float midiFromFrequency (float hz) noexcept
    {
        return hz > 0.0f ? 69.0f + 12.0f * std::log2 (hz / 440.0f) : -1.0f;
    }

    static float frequencyFromMidi (float midi) noexcept
    {
        return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
    }

    void process (const float* const* input, float* const* output,
                  int numChannels, int numSamples) noexcept
    {
        if (numChannels <= 0 || numSamples <= 0 || input == nullptr || output == nullptr)
            return;

        const auto& s = settings;

        // The knobs are read once per block, like any host parameter.
        const auto fuzzAmount = std::clamp (s.fuzz, 0.0f, 1.0f);
        const auto drive = 1.0f + fuzzAmount * 40.0f;
        const auto driveComp = 1.0f / (1.0f + fuzzAmount * 0.55f);
        const auto dryAmount = std::clamp (s.dry, 0.0f, 1.0f);
        const auto subAmount = std::clamp (s.sub, 0.0f, 1.0f);
        const auto doomAmount = std::clamp (s.doom, 0.0f, 1.0f);
        const auto octAmount = std::clamp (s.oct, 0.0f, 1.0f);
        const auto toneHz = 150.0f * std::pow (46.0f, std::clamp (s.tone, 0.0f, 1.0f));
        const auto level = std::clamp (s.level, 0.0f, 1.5f);
        const auto stereo = numChannels > 1;

        for (auto& channel : channels)
            channel.toneFilter.set (toneHz, 0.70, sr);

        // The sub filters follow the note. One update per block is plenty: the
        // note moves at the speed of a bass player, not of a sample.
        if (smoothedSubHz > 0.0f)
        {
            const auto subCut = std::clamp (smoothedSubHz * 0.5f * 2.4f, 28.0f, 700.0f);
            const auto doomCut = std::clamp (smoothedSubHz * 0.25f * 2.6f, 22.0f, 420.0f);

            subShape[0].set (subCut, 0.54, sr);
            subShape[1].set (subCut, 1.30, sr);
            doomShape[0].set (doomCut, 0.54, sr);
            doomShape[1].set (doomCut, 1.30, sr);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const auto inL = input[0][i];
            const auto inR = stereo ? input[1][i] : inL;
            const auto mono = 0.5f * (inL + inR);

            // ---- tracking ------------------------------------------------------
            divider.push (mono);

            const auto tracked = divider.getFrequency();

            if (tracked > 0.0f)
            {
                if (smoothedSubHz <= 0.0f)
                    smoothedSubHz = tracked;
                else
                    smoothedSubHz += (tracked - smoothedSubHz) * 0.0025f;

                smoothedSubHz = std::clamp (smoothedSubHz, 18.0f, 420.0f);
            }
            else
            {
                // Nothing periodic: let the subs fade instead of freezing.
                smoothedSubHz *= 0.9995f;

                if (smoothedSubHz < 18.0f)
                    smoothedSubHz = 0.0f;
            }

            // ---- envelopes -----------------------------------------------------
            const auto rectified = std::abs (mono);
            envelope += (rectified - envelope)
                        * (rectified > envelope ? attackCoeff : releaseCoeff);
            doomEnvelope += (rectified - doomEnvelope)
                            * (rectified > doomEnvelope ? attackCoeff : doomReleaseCoeff);

            const auto cond = condense.process (envelope);
            const auto body = std::max (cond, envelope * 0.55f);

            // ---- the two sub voices -------------------------------------------
            float subVoice = 0.0f;
            float doomVoice = 0.0f;

            if (smoothedSubHz > 0.0f)
            {
                auto shapedSub = divider.squareHalf();
                shapedSub = subShape[0].lowpass (shapedSub);
                shapedSub = subShape[1].lowpass (shapedSub);
                shapedSub = std::tanh (shapedSub * 1.35f) * 0.80f;

                auto shapedDoom = divider.squareQuarter();
                shapedDoom = doomShape[0].lowpass (shapedDoom);
                shapedDoom = doomShape[1].lowpass (shapedDoom);
                shapedDoom = std::tanh (shapedDoom * 1.20f) * 0.85f;

                // A little compression on the way in: the sub stays huge even
                // when the bass is played softly, which is what a dubstep sub
                // wants.
                const auto subAmp = std::min (1.0f, body * 4.5f);
                const auto doomAmp = std::min (1.0f, doomEnvelope * 4.5f);

                subVoice = shapedSub * subAmp * subAmount;
                doomVoice = shapedDoom * doomAmp * doomAmount;
            }

            // Both subs share one DC blocker: a divider that stops on a held
            // note would otherwise leave a constant offset on the output.
            const auto subSum = subDc.process (subVoice + doomVoice);

            // ---- dry + octave up ----------------------------------------------
            auto voiceL = voiceOf (inL, channels[0], dryAmount, octAmount);
            auto voiceR = stereo ? voiceOf (inR, channels[1], dryAmount, octAmount) : voiceL;

            // ---- fuzz ----------------------------------------------------------
            voiceL = fuzzChannel (voiceL, channels[0], drive, driveComp, fuzzAmount);
            voiceR = stereo ? fuzzChannel (voiceR, channels[1], drive, driveComp, fuzzAmount)
                            : voiceL;

            // ---- glue ----------------------------------------------------------
            const auto outL = softClip ((voiceL + subSum) * level);
            const auto outR = stereo ? softClip ((voiceR + subSum) * level) : outL;

            output[0][i] = outL;

            if (stereo)
                output[1][i] = outR;

            // ---- meters ---------------------------------------------------------
            subMeter = std::max (subMeter * 0.9994f, std::abs (subSum));
            voiceMeter = std::max (voiceMeter * 0.9994f, std::abs (voiceL));
        }

        // ---- publish the state for the panel ------------------------------------
        reportCounter += numSamples;

        if (reportCounter >= 256)
        {
            reportCounter = 0;
            reportHz = smoothedSubHz;

            snapshot.frequency.store (reportHz, std::memory_order_relaxed);
            snapshot.subLevel.store (std::clamp (subMeter * 2.0f, 0.0f, 1.0f), std::memory_order_relaxed);
            snapshot.voiceLevel.store (std::clamp (voiceMeter * 1.6f, 0.0f, 1.0f), std::memory_order_relaxed);
            snapshot.inputLevel.store (std::clamp (envelope * 4.0f, 0.0f, 1.0f), std::memory_order_relaxed);

            const auto note = reportHz > 0.0f
                ? int (std::lround (midiFromFrequency (reportHz)))
                : -1;

            snapshot.note.store (std::clamp (note, -1, 127), std::memory_order_relaxed);
            snapshot.active.store (reportHz > 0.0f ? 1 : 0, std::memory_order_relaxed);
        }
    }

private:
    struct Channel
    {
        DcBlocker octDc;
        OnePole octSmooth;
        Svf toneFilter;
    };

    float voiceOf (float dry, Channel& channel, float dryAmount, float octAmount) noexcept
    {
        // A full wave rectifier makes the octave up instantly, with the same
        // attack as the note: this is the part that bites.
        const auto rectified = std::abs (dry);
        const auto oct = channel.octSmooth.process (channel.octDc.process (rectified * 1.35f));

        return dry * dryAmount + oct * octAmount * 0.80f;
    }

    float fuzzChannel (float x, Channel& channel, float drive, float comp, float amount) noexcept
    {
        const auto dirty = std::tanh (x * drive) * comp;
        const auto toned = channel.toneFilter.lowpass (dirty);

        return x * (1.0f - amount) + toned * amount;
    }

    // Transparent below 0.80, then a smooth knee: the dry signal is untouched,
    // but a stack of sub voices cannot leave the rails.
    static float softClip (float x) noexcept
    {
        constexpr float knee = 0.80f;
        const auto magnitude = std::abs (x);

        if (magnitude <= knee)
            return x;

        const auto compressed = knee + (1.0f - knee) * std::tanh ((magnitude - knee) / (1.0f - knee));
        return x > 0.0f ? compressed : -compressed;
    }

    double sr = 48000.0;

    Divider divider;

    Svf subShape[2];
    Svf doomShape[2];

    OnePole condense;

    DcBlocker subDc;

    float attackCoeff = 0.01f;
    float releaseCoeff = 0.0002f;
    float doomReleaseCoeff = 0.0001f;

    float smoothedSubHz = 0.0f;

    float envelope = 0.0f, doomEnvelope = 0.0f;

    float subMeter = 0.0f, voiceMeter = 0.0f;

    int reportCounter = 0;
    float reportHz = 0.0f;

    std::array<Channel, 2> channels;

    Settings settings;
    Snapshot snapshot;
};

//==============================================================================
// Note names, used by the panel and by the tests.
inline const char* noteName (int midi) noexcept
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (midi < 0)
        return "--";

    return names[((midi % 12) + 12) % 12];
}

inline std::string noteLabel (int midi) noexcept
{
    if (midi < 0)
        return "--";

    return std::string (noteName (midi)) + std::to_string ((midi / 12) - 1);
}

} // namespace giannis
