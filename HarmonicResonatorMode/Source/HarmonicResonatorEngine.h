#pragma once

// HarmonicResonatorMode engine.
//
// This is not a distortion followed by a resonant filter. It is a loop:
//
//   input --> [ nonlinear stage ] --> [ resonator bank ] --> wet
//                  ^                        |
//                  |                        |
//                  +------ coupling --------+
//
// The routing mode chooses which edges of that loop are open:
//
//   DIST > RES    the distortion drives the resonators
//   RES  > DIST   the resonators drive the distortion
//   RES  < > DIST both, each one feeding the other
//
// INTERACTION is the amount of coupling on the return edge, FEEDBACK is the
// gain of that edge, and the source / destination pair says what travels on
// it and where it lands. With the coupling closed the resonators keep the
// nonlinear stage ringing and the nonlinear stage keeps the resonators fed:
// a state a crossfade between two parallel chains cannot reach.
//
// Everything below is real-time safe: fixed-size arrays, no allocation, no
// locks, no strings on the audio path. The per-sample work never depends on
// the block size, so with the modulation still the output is the same whatever
// the host asks for; only the modulation targets are refreshed once per block.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace harmonicresonator
{

constexpr int kNumResonators = 8;
constexpr double kPi = 3.14159265358979323846;

enum class Shape       { soft = 0, hard, fold, asym, cheby, crush, ring };
enum class Tuning      { harmonic = 0, odd, even, chord, metallic };
enum class Routing     { distToRes = 0, resToDist, bidirectional };
enum class Source      { res = 0, dist, clean, combined, band };
enum class Destination { both = 0, toDist, toRes };
enum class ModMode     { lfo = 0, random, sampleHold, envelope };
enum class ModDest     { tune = 0, resonance, decay, drive, interaction, pan };

//==============================================================================
struct Settings
{
    // output
    float inputGain = 1.0f;      // linear
    float outputGain = 0.8f;     // linear
    float mix = 0.75f;           // dry / wet
    float fundamental = 0.25f;   // 0 .. 1, reinforce the input fundamental

    // harmonics
    float drive = 0.35f;         // 0 .. 1
    Shape shape = Shape::soft;
    float evenOdd = -0.25f;      // -1 odd only, 0 both, +1 even only
    float symmetry = 0.15f;      // 0 .. 1, positive / negative imbalance
    float density = 0.55f;       // 0 .. 1, how far the harmonics reach
    float brightness = 0.5f;     // 0 .. 1, tilt of the shaped signal

    // resonators
    float tune = 220.0f;         // Hz, the fundamental of the bank
    Tuning tuning = Tuning::harmonic;
    float resonance = 0.55f;     // 0 .. 1
    float decay = 1.4f;          // seconds
    float spread = 0.5f;         // 0 .. 1, stereo spread of the bank
    float resGain = 0.70f;       // 0 .. 1, level of the bank
    float align = 0.0f;          // 0 .. 1, pull the bank onto the input pitch

    // interaction
    Routing routing = Routing::distToRes;
    float interaction = 0.35f;   // 0 .. 1, coupling on the return edge
    float feedback = 0.45f;      // 0 .. 1, gain of that edge
    float fbDrive = 0.30f;       // 0 .. 1, saturation in the return edge
    float lowCut = 90.0f;        // Hz, high pass inside the return edge
    float highCut = 11000.0f;    // Hz, low pass inside the return edge
    Source source = Source::res;
    Destination destination = Destination::both;

    // modulation
    ModMode modMode = ModMode::lfo;
    ModDest modDest = ModDest::tune;
    float modRate = 0.40f;       // Hz
    float modDepth = 0.0f;       // 0 .. 1
    float envDepth = 0.0f;       // 0 .. 1, envelope follower to the drive
};

//==============================================================================
struct Snapshot
{
    std::atomic<float> inLevel  { 0.0f };
    std::atomic<float> wetLevel { 0.0f };
    std::atomic<float> lfoValue { 0.0f };
    std::atomic<float> envValue { 0.0f };
    std::atomic<float> tracked  { 0.0f };   // Hz found by the pitch tracker
    std::atomic<float> confidence { 0.0f };
    std::atomic<int>   routingMode { 0 };
    std::atomic<float> partialEnergy[kNumResonators] {};
    std::atomic<float> partialFreq[kNumResonators] {};
    std::atomic<int>   activePartials { 0 };
};

//==============================================================================
// A one-pole smoother, used so a knob move never steps the signal.
struct Smooth
{
    float value = 0.0f;
    float coeff = 0.01f;

    void setup (double sampleRate, double milliseconds) noexcept
    {
        coeff = (float) (1.0 - std::exp (-1.0 / (0.001 * milliseconds * sampleRate)));
    }

    float next (float target) noexcept
    {
        value += (target - value) * coeff;
        return value;
    }

    void snap (float target) noexcept { value = target; }
};

//==============================================================================
// One-pole low / high pass pair, used for the tone controls and the filters
// inside the coupling path.
struct OnePole
{
    double state = 0.0;
    float coeff = 1.0f;

    void setCutoff (double hz, double sampleRate) noexcept
    {
        hz = std::clamp (hz, 5.0, sampleRate * 0.45);
        coeff = (float) (1.0 - std::exp (-2.0 * kPi * hz / sampleRate));
    }

    float lowpass (float x) noexcept
    {
        state += (double) coeff * ((double) x - state);
        return (float) state;
    }

    float highpass (float x) noexcept { return x - lowpass (x); }

    void reset() noexcept { state = 0.0; }
};

//==============================================================================
// Removes the DC that asymmetric shaping and any loop love to build up.
struct DCBlocker
{
    float lastIn = 0.0f;
    float lastOut = 0.0f;

    float process (float x) noexcept
    {
        const auto y = x - lastIn + 0.9975f * lastOut;
        lastIn = x;
        lastOut = y;
        return y;
    }

    void reset() noexcept { lastIn = lastOut = 0.0f; }
};

//==============================================================================
// A topology preserving state variable filter used as one resonator.
//
// The band pass output is normalised to unity at the peak, so a high Q makes
// the tail longer without making the resonator louder. The state is double:
// a bell can ring for seconds at 9 kHz and a float would run out of mantissa.
struct Resonator
{
    double ic1 = 0.0;
    double ic2 = 0.0;

    float g = 0.1f;      // tan (pi * f / sr)
    float k = 1.0f;      // 1 / Q
    float gPrev = 0.1f;
    float kPrev = 1.0f;
    float gStep = 0.0f;
    float kStep = 0.0f;

    // Coefficients travel from where they were to where they are, block by
    // block, so dragging the tune knob glides instead of stepping.
    void setTargets (float newG, float newK) noexcept
    {
        gPrev = g;
        kPrev = k;
        g = newG;
        k = newK;
    }

    void beginBlock (int numSamples) noexcept
    {
        const auto n = (float) std::max (1, numSamples);
        gStep = (g - gPrev) / n;
        kStep = (k - kPrev) / n;
        g = gPrev;
        k = kPrev;
    }

    // Start from the target instead of sliding to it: used on the first block
    // after a reset, so the very first block does not sweep the bank from a
    // default frequency and leave that sweep ringing.
    void snapToTargets() noexcept { gPrev = g; kPrev = k; }

    float process (float x) noexcept
    {
        g += gStep;
        k += kStep;

        const auto gg = (double) g;
        const auto kk = (double) std::max (k, 1.0e-4f);

        const auto a1 = 1.0 / (1.0 + gg * (gg + kk));
        const auto a2 = gg * a1;
        const auto a3 = gg * a2;

        const auto v3 = (double) x - ic2;
        const auto v1 = a1 * ic1 + a2 * v3;
        const auto v2 = ic2 + a2 * ic1 + a3 * v3;

        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;

        if (! std::isfinite (ic1) || ! std::isfinite (ic2))
            ic1 = ic2 = 0.0;

        return (float) (kk * v1);
    }

    void reset() noexcept { ic1 = ic2 = 0.0; }
};

//==============================================================================
// A small autocorrelation pitch tracker.
//
// The input is decimated by eight and low passed first, then every 256 input
// samples the last window is correlated against itself over the lag range of
// a musical fundamental. Running it on a fixed sample count (not once per
// block) is what keeps it independent of the host block size.
struct PitchTracker
{
    static constexpr int kRing = 512;
    static constexpr int kWindow = 192;
    static constexpr int kDecimate = 8;
    static constexpr int kPeriod = 256;

    double sampleRate = 48000.0;
    float lowpassState = 0.0f;
    float lowpassCoeff = 0.3f;
    float ring[kRing] = {};
    int writeIndex = 0;
    int decimateCount = 0;
    int periodCount = 0;

    float f0 = 0.0f;
    float confidence = 0.0f;

    void prepare (double sr) noexcept
    {
        sampleRate = std::max (8000.0, sr);
        lowpassCoeff = (float) (1.0 - std::exp (-2.0 * kPi * 1200.0 / sampleRate));
        reset();
    }

    void reset() noexcept
    {
        lowpassState = 0.0f;
        std::fill (ring, ring + kRing, 0.0f);
        writeIndex = 0;
        decimateCount = 0;
        periodCount = 0;
        f0 = 0.0f;
        confidence = 0.0f;
    }

    void push (float x) noexcept
    {
        lowpassState += (x - lowpassState) * lowpassCoeff;

        if (++decimateCount >= kDecimate)
        {
            decimateCount = 0;
            ring[writeIndex] = lowpassState;
            writeIndex = (writeIndex + 1) % kRing;
        }

        if (++periodCount >= kPeriod)
        {
            periodCount = 0;
            analyse();
        }
    }

    void analyse() noexcept
    {
        const auto decimatedRate = sampleRate / (double) kDecimate;

        // The window is read oldest first.
        float window[kWindow];

        for (int i = 0; i < kWindow; ++i)
        {
            const auto index = (writeIndex - kWindow + i + 2 * kRing) % kRing;
            window[i] = ring[index];
        }

        const auto minLag = std::max (2, (int) std::floor (decimatedRate / 1000.0));
        const auto maxLag = std::min (kWindow / 2 - 1, (int) std::ceil (decimatedRate / 80.0));

        if (maxLag <= minLag + 2)
            return;

        double totalEnergy = 0.0;

        for (int i = 0; i < kWindow; ++i)
            totalEnergy += (double) window[i] * (double) window[i];

        if (totalEnergy < 1.0e-7)
        {
            // Hold the last pitch for a while when the note stops: dropping
            // confidence in a few milliseconds would sweep the bank back to
            // the knob and take the ringing tail with it.
            confidence *= 0.997f;
            return;
        }

        float best = 0.0f;
        int bestLag = 0;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double dot = 0.0;
            double energy = 0.0;

            for (int i = 0; i + lag < kWindow; ++i)
            {
                dot += (double) window[i] * (double) window[i + lag];
                energy += (double) window[i + lag] * (double) window[i + lag];
            }

            if (energy < 1.0e-9)
                continue;

            const auto r = (float) (dot / std::sqrt (totalEnergy * energy));

            if (r > best)
            {
                best = r;
                bestLag = lag;
            }
        }

        if (bestLag == 0 || best < 0.45f)
        {
            confidence *= 0.99f;

            if (confidence < 0.01f)
                f0 = 0.0f;

            return;
        }

        // Parabolic refinement around the winning lag.
        auto refinedLag = (double) bestLag;

        if (bestLag > minLag && bestLag < maxLag)
        {
            const auto r0 = correlationAt (window, bestLag - 1, totalEnergy);
            const auto r1 = best;
            const auto r2 = correlationAt (window, bestLag + 1, totalEnergy);
            const auto denominator = r0 - 2.0f * r1 + r2;

            if (std::abs (denominator) > 1.0e-6f)
                refinedLag += 0.5 * (double) ((r0 - r2) / denominator);
        }

        if (refinedLag < 2.0)
            return;

        auto found = (float) (decimatedRate / refinedLag);
        found = std::clamp (found, 60.0f, 1200.0f);

        // An octave jump is allowed; a small wobble is smoothed.
        if (f0 > 0.0f && std::abs (found - f0) / f0 < 0.20f)
            f0 = f0 * 0.65f + found * 0.35f;
        else
            f0 = found;

        confidence = confidence * 0.6f + best * 0.4f;
    }

    float correlationAt (const float* window, int lag, double totalEnergy) const noexcept
    {
        double dot = 0.0;
        double energy = 0.0;

        for (int i = 0; i + lag < kWindow; ++i)
        {
            dot += (double) window[i] * (double) window[i + lag];
            energy += (double) window[i + lag] * (double) window[i + lag];
        }

        if (energy < 1.0e-9)
            return 0.0f;

        return (float) (dot / std::sqrt (totalEnergy * energy));
    }
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate);
    void setSettings (const Settings& s) noexcept { settings = s; }

    void process (const float* const* input, float* const* output,
                  int numChannels, int numSamples) noexcept;

    const Snapshot& getSnapshot() const noexcept { return snapshot; }

    void reset() noexcept;

private:
    static void ratiosFor (Tuning tuning, float* out) noexcept;

    float shapeSample (float x, float drive, float evenOdd, float symmetry,
                       float density) noexcept;
    void updateResonators (float tune, float qScale, float decay) noexcept;

    Settings settings;
    Snapshot snapshot;

    double sampleRate = 48000.0;
    bool prepared = false;

    // ---- smoothers ----
    Smooth inGainSm, outGainSm, mixSm, fundSm;
    Smooth driveSm, evenOddSm, symmetrySm, densitySm, brightSm;
    Smooth interactionSm, feedbackSm, fbDriveSm, resGainSm, alignSm;

    // ---- tone and coupling filters ----
    OnePole densityL, densityR;
    OnePole brightL, brightR;
    OnePole lowCutL, lowCutR, highCutL, highCutR;
    OnePole fundL, fundR;
    DCBlocker dcL, dcR, dcBackL, dcBackR;

    // ---- resonators ----
    Resonator resonators[kNumResonators];
    float partialGain[kNumResonators] = {};
    float panL[kNumResonators] = {};
    float panR[kNumResonators] = {};
    float partialEnergy[kNumResonators] = {};
    float partialFreq[kNumResonators] = {};

    // ---- loop state ----
    float backL = 0.0f;
    float backR = 0.0f;

    // ---- modulation ----
    double lfoPhase = 0.0;
    float lfoValue = 0.0f;
    float randomValue = 0.0f;
    float randomTarget = 0.0f;
    std::uint32_t randomState = 0x12345678u;
    float envValue = 0.0f;
    double ringPhase = 0.0;

    PitchTracker tracker;

    // ---- meters ----
    float inPeak = 0.0f;
    float wetPeak = 0.0f;

    // Set by reset(): the next block starts the resonators on their targets.
    bool snapResonators = true;
};

//==============================================================================
inline void Engine::prepare (double sr)
{
    sampleRate = std::max (8000.0, sr);
    prepared = true;

    inGainSm.setup (sampleRate, 12.0);
    outGainSm.setup (sampleRate, 12.0);
    mixSm.setup (sampleRate, 12.0);
    fundSm.setup (sampleRate, 25.0);
    driveSm.setup (sampleRate, 15.0);
    evenOddSm.setup (sampleRate, 15.0);
    symmetrySm.setup (sampleRate, 15.0);
    densitySm.setup (sampleRate, 20.0);
    brightSm.setup (sampleRate, 20.0);
    interactionSm.setup (sampleRate, 20.0);
    feedbackSm.setup (sampleRate, 20.0);
    fbDriveSm.setup (sampleRate, 20.0);
    resGainSm.setup (sampleRate, 20.0);
    alignSm.setup (sampleRate, 40.0);

    tracker.prepare (sampleRate);

    reset();
}

inline void Engine::reset() noexcept
{
    for (auto& r : resonators)
        r.reset();

    densityL.reset(); densityR.reset();
    brightL.reset();  brightR.reset();
    lowCutL.reset();  lowCutR.reset();
    highCutL.reset(); highCutR.reset();
    fundL.reset();    fundR.reset();
    dcL.reset(); dcR.reset(); dcBackL.reset(); dcBackR.reset();

    backL = backR = 0.0f;

    lfoPhase = 0.0;
    lfoValue = 0.0f;
    randomValue = 0.0f;
    randomTarget = 0.0f;
    envValue = 0.0f;
    ringPhase = 0.0;

    inPeak = 0.0f;
    wetPeak = 0.0f;

    tracker.reset();
    snapResonators = true;

    for (int i = 0; i < kNumResonators; ++i)
        partialEnergy[i] = 0.0f;

    inGainSm.snap (settings.inputGain);
    outGainSm.snap (settings.outputGain);
    mixSm.snap (settings.mix);
    fundSm.snap (settings.fundamental);
    driveSm.snap (settings.drive);
    evenOddSm.snap (settings.evenOdd);
    symmetrySm.snap (settings.symmetry);
    densitySm.snap (settings.density);
    brightSm.snap (settings.brightness);
    interactionSm.snap (settings.interaction);
    feedbackSm.snap (settings.feedback);
    fbDriveSm.snap (settings.fbDrive);
    resGainSm.snap (settings.resGain);
    alignSm.snap (settings.align);

    snapshot.inLevel.store (0.0f);
    snapshot.wetLevel.store (0.0f);
    snapshot.lfoValue.store (0.0f);
    snapshot.envValue.store (0.0f);
    snapshot.tracked.store (0.0f);
    snapshot.confidence.store (0.0f);
    snapshot.activePartials.store (0);
}

//==============================================================================
inline void Engine::ratiosFor (Tuning tuning, float* out) noexcept
{
    static const float harmonic[kNumResonators] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    static const float odd[kNumResonators]      = { 1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 11.0f, 13.0f, 15.0f };
    static const float even[kNumResonators]     = { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f };
    static const float chord[kNumResonators]    = { 1.0f, 1.25f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 6.0f };
    static const float metallic[kNumResonators] = { 1.0f, 1.47f, 2.09f, 2.56f, 3.31f, 4.13f, 5.07f, 6.21f };

    const float* source = harmonic;

    switch (tuning)
    {
        case Tuning::odd:      source = odd;      break;
        case Tuning::even:     source = even;     break;
        case Tuning::chord:    source = chord;    break;
        case Tuning::metallic: source = metallic; break;
        case Tuning::harmonic:
        default:               source = harmonic; break;
    }

    for (int i = 0; i < kNumResonators; ++i)
        out[i] = source[i];
}

//==============================================================================
inline void Engine::updateResonators (float tune, float qScale, float decay) noexcept
{
    float ratios[kNumResonators];
    ratiosFor (settings.tuning, ratios);

    const auto nyquistLimit = (float) (sampleRate * 0.45);
    const auto baseDecay = std::clamp (decay, 0.03f, 8.0f);
    const auto resonanceScale = 0.6f + 2.4f * std::clamp (settings.resonance, 0.0f, 1.0f);

    for (int i = 0; i < kNumResonators; ++i)
    {
        const auto frequency = std::clamp (tune * ratios[i], 20.0f, nyquistLimit);

        // The top of the series damps first, the way a real material behaves.
        const auto partialDecay = baseDecay * std::pow (ratios[i], -0.18f);

        // A resonator ringing for T60 seconds needs a bandwidth of
        // 6.9078 / (pi * T60) Hz, and Q = f / bandwidth.
        const auto bandwidth = 6.9078 / (kPi * (double) partialDecay);
        const auto q = std::clamp ((double) frequency / bandwidth * (double) resonanceScale
                                       * (double) qScale,
                                   0.7, 4000.0);

        const auto g = (float) std::tan (kPi * (double) frequency / sampleRate);
        const auto k = (float) (1.0 / q);

        resonators[i].setTargets (g, k);

        // The bank sums to roughly one: the partials are scaled by their
        // position so the top of the series is quieter.
        partialGain[i] = 1.0f / (float) kNumResonators
                         / (0.55f + 0.45f * std::sqrt (ratios[i]));

        const auto side = (i % 2 == 0) ? -1.0f : 1.0f;
        const auto width = 0.30f + 0.70f * ((float) i / (float) (kNumResonators - 1));
        const auto pan = std::clamp (side * settings.spread * width, -1.0f, 1.0f);
        const auto angle = (pan + 1.0f) * 0.25f * (float) kPi;

        panL[i] = std::cos (angle);
        panR[i] = std::sin (angle);

        partialFreq[i] = frequency;
    }
}

//==============================================================================
inline float Engine::shapeSample (float x, float drive, float evenOdd,
                                  float symmetry, float density) noexcept
{
    const auto gain = 1.0f + drive * drive * 40.0f;
    const auto sym = std::clamp (symmetry, 0.0f, 1.0f);

    // Positive and negative halves get different weights: that is what makes
    // the shape asymmetric, and the asymmetry is audible on its own.
    const auto y = x * gain;
    const auto biased = y * (y >= 0.0f ? (1.0f + 0.65f * sym)
                                       : (1.0f - 0.55f * sym));

    // One shaper, evaluated twice below. Everything the ODD / EVEN knob needs
    // comes from these two calls: a symmetric shaper only ever makes odd
    // harmonics, so the even side is the same shaper fed with |x|, which is
    // even in x by construction. The old version biased the input instead,
    // and at high drive that turned into a square wave: all odd, whatever the
    // EVEN knob said.
    auto shapeOf = [this, drive, density] (float v) noexcept
    {
        switch (settings.shape)
        {
            case Shape::soft:
                return std::tanh (v);

            case Shape::hard:
                return std::clamp (v, -1.0f, 1.0f);

            case Shape::fold:
            {
                // Triangle foldback around +/-1, closed form so there is no loop.
                auto f = std::fmod (v, 4.0f);

                if (f > 2.0f)  f -= 4.0f;
                if (f > 1.0f)  f = 2.0f - f;
                if (f < -1.0f) f = -2.0f - f;

                return f;
            }

            case Shape::asym:
                // The two halves are treated differently even before the
                // rectifier: this is the shape that already sounds lopsided.
                return v > 0.0f ? std::tanh (v * 1.15f)
                                : -0.55f * std::tanh (v * 0.60f);

            case Shape::cheby:
            {
                // Chebyshev generation, cheapest form: T(n+1) = 2x T(n) - T(n-1).
                const auto in = std::clamp (v, -1.0f, 1.0f);
                const auto order = 3 + (int) std::round (std::clamp (density, 0.0f, 1.0f) * 13.0f);

                auto t0 = 1.0f;
                auto t1 = in;
                auto sum = 0.0f;

                for (int n = 2; n <= order; ++n)
                {
                    const auto t2 = 2.0f * in * t1 - t0;
                    t0 = t1;
                    t1 = t2;
                    sum += t2 / (float) n;
                }

                return sum;
            }

            case Shape::crush:
            {
                const auto bits = 12.0f - 7.0f * drive;          // 12 bit .. 5 bit
                const auto levels = std::pow (2.0f, std::max (2.0f, bits));

                return std::round (std::clamp (v, -1.0f, 1.0f) * levels) / levels;
            }

            case Shape::ring:
            default:
                return std::tanh (v);
        }
    };

    const auto odd = shapeOf (biased);
    const auto even = shapeOf (std::abs (biased));

    if (settings.shape == Shape::ring)
    {
        // Ring modulation against the tuned frequency: sidebands, not a
        // waveshape. The phase is advanced per sample in process().
        const auto carrier = (float) std::sin (ringPhase);

        return (odd * 0.5f + even * 0.5f) * carrier / (1.0f + drive * 2.2f);
    }

    // The rectified side carries a large DC term; the blockers in the audio
    // path and in the return edge take it out a few milliseconds later.
    const auto balance = std::clamp (evenOdd, -1.0f, 1.0f);
    const auto evenWeight = (balance + 1.0f) * 0.5f;
    const auto oddWeight = 1.0f - evenWeight;

    const auto blended = odd * oddWeight + even * evenWeight;

    // Level compensation: more drive must not mean more volume.
    return blended / (1.0f + drive * 2.2f);
}

//==============================================================================
inline void Engine::process (const float* const* input, float* const* output,
                             int numChannels, int numSamples) noexcept
{
    if (! prepared || numChannels <= 0 || numSamples <= 0)
        return;

    const auto s = settings;

    // ---- the fixed targets of this block --------------------------------
    const auto density = std::clamp (s.density, 0.0f, 1.0f);

    densityL.setCutoff (1200.0 + 18800.0 * (double) (density * density), sampleRate);
    densityR.setCutoff (1200.0 + 18800.0 * (double) (density * density), sampleRate);
    brightL.setCutoff (2400.0, sampleRate);
    brightR.setCutoff (2400.0, sampleRate);
    lowCutL.setCutoff ((double) std::clamp (s.lowCut, 20.0f, 900.0f), sampleRate);
    lowCutR.setCutoff ((double) std::clamp (s.lowCut, 20.0f, 900.0f), sampleRate);
    highCutL.setCutoff ((double) std::clamp (s.highCut, 500.0f, 18000.0f), sampleRate);
    highCutR.setCutoff ((double) std::clamp (s.highCut, 500.0f, 18000.0f), sampleRate);

    // ---- modulation -----------------------------------------------------
    const auto lfoDepth = std::clamp (s.modDepth, 0.0f, 1.0f);
    const auto envDepth = std::clamp (s.envDepth, 0.0f, 1.0f);

    const auto modSignal = (s.modMode == ModMode::envelope)
                               ? envValue * lfoDepth
                               : (lfoValue * lfoDepth + envValue * envDepth);

    auto tune = std::clamp (s.tune, 20.0f, 4000.0f);

    // The pitch tracker pulls the bank onto the input fundamental. It only
    // moves when it is confident and when the ALIGN knob asks for it.
    if (s.align > 0.001f && tracker.confidence > 0.5f && tracker.f0 > 30.0f)
    {
        const auto blend = std::clamp (s.align * std::clamp ((tracker.confidence - 0.5f) * 2.5f, 0.0f, 1.0f),
                                       0.0f, 1.0f);
        const auto ratio = (float) (std::log ((double) tracker.f0) / std::log (2.0)
                                    - std::log ((double) tune) / std::log (2.0));
        tune *= std::pow (2.0f, ratio * blend);
    }

    auto qScale = 1.0f;
    auto decayScale = 1.0f;

    switch (s.modDest)
    {
        case ModDest::tune:
            tune *= std::pow (2.0f, modSignal * 1.5f);
            break;

        case ModDest::resonance:
            qScale = std::pow (2.0f, modSignal * 2.0f);
            break;

        case ModDest::decay:
            decayScale = std::pow (2.0f, modSignal * 1.6f);
            break;

        case ModDest::drive:
        case ModDest::interaction:
        case ModDest::pan:
        default:
            break;
    }


    updateResonators (tune, qScale, s.decay * decayScale);

    for (auto& r : resonators)
    {
        if (snapResonators)
            r.snapToTargets();

        r.beginBlock (numSamples);
    }

    snapResonators = false;

    // ---- which edges of the loop are open -------------------------------
    const auto k = std::clamp (std::clamp (s.interaction, 0.0f, 1.0f)
                               * std::clamp (s.feedback, 0.0f, 1.0f), 0.0f, 0.98f);

    float forward = 1.0f;     // distortion -> resonators
    float directIn = 0.0f;    // clean input -> resonators
    float wetDist = 0.30f;
    float wetRes = 1.00f;

    switch (s.routing)
    {
        case Routing::distToRes:
            forward = 1.0f; directIn = 0.0f;
            wetDist = 0.30f; wetRes = 1.00f;
            break;

        case Routing::resToDist:
            forward = k;   directIn = 1.0f;
            wetDist = 1.00f; wetRes = 0.22f;
            break;

        case Routing::bidirectional:
        default:
            forward = k;   directIn = 1.0f;
            wetDist = 0.55f; wetRes = 0.85f;
            break;
    }

    // Where the return edge lands.
    float returnToDist = 0.0f;
    float returnToRes = 0.0f;

    switch (s.destination)
    {
        case Destination::toDist: returnToDist = 1.0f; break;
        case Destination::toRes:  returnToRes  = 1.0f; break;
        case Destination::both:
        default:                  returnToDist = 0.7f; returnToRes = 0.3f; break;
    }

    const auto left = input[0];
    const auto right = numChannels > 1 ? input[1] : input[0];

    const auto outL = output[0];
    const auto outR = numChannels > 1 ? output[1] : output[0];

    const auto fbDrive = std::clamp (s.fbDrive, 0.0f, 1.0f);
    const auto fbGain = 1.0f + 7.0f * fbDrive;

    const auto driveMod = (s.modDest == ModDest::drive) ? (1.0f + modSignal) : 1.0f;
    const auto interMod = (s.modDest == ModDest::interaction) ? (1.0f + modSignal) : 1.0f;
    const auto panMod = (s.modDest == ModDest::pan) ? modSignal : 0.0f;

    const auto ringRate = (double) std::clamp (s.tune, 20.0f, 8000.0f) / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- parameters, smoothed per sample ----------------------------
        const auto inGain  = inGainSm.next (s.inputGain);
        const auto outGain = outGainSm.next (s.outputGain);
        const auto mix     = mixSm.next (std::clamp (s.mix, 0.0f, 1.0f));
        const auto fund    = fundSm.next (std::clamp (s.fundamental, 0.0f, 1.0f));
        const auto drv     = driveSm.next (std::clamp (s.drive, 0.0f, 1.0f) * driveMod);
        const auto eo      = evenOddSm.next (std::clamp (s.evenOdd, -1.0f, 1.0f));
        const auto sym     = symmetrySm.next (std::clamp (s.symmetry, 0.0f, 1.0f));
        const auto dens    = densitySm.next (density);
        const auto bright  = brightSm.next (std::clamp (s.brightness, 0.0f, 1.0f));
        const auto resG    = resGainSm.next (std::clamp (s.resGain, 0.0f, 1.0f));

        const auto xL = left[i] * inGain;
        const auto xR = right[i] * inGain;

        // ---- nonlinear stage --------------------------------------------
        ringPhase += 2.0 * kPi * ringRate;

        if (ringPhase > 2.0 * kPi)
            ringPhase -= 2.0 * kPi;

        const auto dInL = xL + backL * returnToDist;
        const auto dInR = xR + backR * returnToDist;

        auto dL = shapeSample (dInL, drv, eo, sym, dens);
        auto dR = shapeSample (dInR, drv, eo, sym, dens);

        // ---- tone of the shaped signal ----------------------------------
        dL = densityL.lowpass (dL);
        dR = densityR.lowpass (dR);

        {
            const auto lowL = brightL.lowpass (dL);
            const auto lowR = brightR.lowpass (dR);
            const auto tilt = (bright - 0.5f) * 1.8f;   // -0.9 .. +0.9

            dL = lowL + (dL - lowL) * (1.0f + tilt);
            dR = lowR + (dR - lowR) * (1.0f + tilt);
        }

        // ---- what feeds the bank ----------------------------------------
        const auto monoD = (dL + dR) * 0.5f;
        const auto monoIn = (xL + xR) * 0.5f;

        auto resIn = monoD * forward + monoIn * directIn + (backL + backR) * 0.5f * returnToRes;

        // ---- the bank ----------------------------------------------------
        auto rL = 0.0f;
        auto rR = 0.0f;

        for (int r = 0; r < kNumResonators; ++r)
        {
            const auto y = resonators[r].process (resIn) * partialGain[r];

            rL += y * panL[r];
            rR += y * panR[r];

            partialEnergy[r] += (std::abs (y) - partialEnergy[r]) * 0.0025f;
        }

        rL *= resG * 2.0f;
        rR *= resG * 2.0f;

        // Modulation of the stereo position: one cheap width shift of the
        // whole bank, so the partial loop stays free of trigonometry.
        if (panMod != 0.0f)
        {
            const auto shift = std::clamp (panMod * 0.8f, -1.0f, 1.0f);
            const auto leftScale = 1.0f - shift * 0.5f;
            const auto rightScale = 1.0f + shift * 0.5f;

            rL *= leftScale;
            rR *= rightScale;
        }

        // ---- the return edge ---------------------------------------------
        // The source selects what is reinjected; the filters and the
        // saturation keep the loop musical instead of a blow up.
        //
        // The bank is a filter bank, not a gain: a single partial arrives at
        // roughly a fifth of the level that went in, so the return edge needs
        // its own send. Without it the coupling knob moved numbers and not
        // much else.
        constexpr float kLoopSend = 4.0f;

        auto sourceL = 0.0f;
        auto sourceR = 0.0f;

        switch (s.source)
        {
            case Source::clean:    sourceL = xL;     sourceR = xR;     break;
            case Source::dist:     sourceL = dL;     sourceR = dR;     break;

            case Source::combined:
                sourceL = (dL + rL * kLoopSend) * 0.5f;
                sourceR = (dR + rR * kLoopSend) * 0.5f;
                break;

            case Source::band:
            case Source::res:
            default:
                sourceL = rL * kLoopSend;
                sourceR = rR * kLoopSend;
                break;
        }

        const auto kb = std::clamp (k * interMod, 0.0f, 0.98f);

        auto toDistL = sourceL * kb;
        auto toDistR = sourceR * kb;

        toDistL = lowCutL.highpass (toDistL);
        toDistR = lowCutR.highpass (toDistR);
        toDistL = highCutL.lowpass (toDistL);
        toDistR = highCutR.lowpass (toDistR);

        toDistL = std::tanh (toDistL * fbGain) / fbGain;
        toDistR = std::tanh (toDistR * fbGain) / fbGain;

        toDistL = dcBackL.process (toDistL);
        toDistR = dcBackR.process (toDistR);

        backL = toDistL;
        backR = toDistR;

        // ---- wet ----------------------------------------------------------
        auto wetL = dL * wetDist + rL * wetRes;
        auto wetR = dR * wetDist + rR * wetRes;

        wetL = dcL.process (wetL);
        wetR = dcR.process (wetR);

        if (fund > 0.0001f)
        {
            wetL += fundL.lowpass (xL) * fund;
            wetR += fundR.lowpass (xR) * fund;
        }

        // ---- mix, limiter, output ----------------------------------------
        auto yL = (xL * (1.0f - mix) + wetL * mix) * outGain;
        auto yR = (xR * (1.0f - mix) + wetR * mix) * outGain;

        yL = std::tanh (yL * 0.9f) / 0.9f;
        yR = std::tanh (yR * 0.9f) / 0.9f;

        if (! std::isfinite (yL) || ! std::isfinite (yR))
        {
            reset();
            outL[i] = 0.0f;
            outR[i] = 0.0f;
            continue;
        }

        outL[i] = yL;
        outR[i] = yR;

        // ---- LFO, random source and envelope, per sample ------------------
        lfoPhase += (double) std::clamp (s.modRate, 0.02f, 24.0f) / sampleRate;

        if (lfoPhase >= 1.0)
        {
            lfoPhase -= 1.0;

            // A fresh random target on every LFO cycle.
            randomState = randomState * 1664525u + 1013904223u;
            randomTarget = ((float) ((randomState >> 9) & 0x7fffff) / 8388607.0f) * 2.0f - 1.0f;
        }

        if (s.modMode == ModMode::lfo || s.modMode == ModMode::envelope)
        {
            lfoValue = (float) std::sin (2.0 * kPi * lfoPhase);
        }
        else if (s.modMode == ModMode::random)
        {
            randomValue += (randomTarget - randomValue) * 0.002f;
            lfoValue = randomValue;
        }
        else // sampleHold
        {
            lfoValue = randomTarget;
        }

        {
            const auto level = std::max (std::abs (xL), std::abs (xR));
            const auto coeff = level > envValue ? 0.004f : 0.00035f;
            envValue += (level - envValue) * coeff;
        }

        tracker.push (monoIn);

        inPeak = std::max (std::abs (xL), std::max (std::abs (xR), inPeak * 0.9997f));
        wetPeak = std::max (std::abs (wetL), std::max (std::abs (wetR), wetPeak * 0.9997f));
    }

    // ---- publish the meters ---------------------------------------------
    snapshot.inLevel.store (std::min (inPeak, 2.0f), std::memory_order_relaxed);
    snapshot.wetLevel.store (std::min (wetPeak, 2.0f), std::memory_order_relaxed);
    snapshot.lfoValue.store (lfoValue, std::memory_order_relaxed);
    snapshot.envValue.store (envValue, std::memory_order_relaxed);
    snapshot.tracked.store (tracker.f0, std::memory_order_relaxed);
    snapshot.confidence.store (tracker.confidence, std::memory_order_relaxed);
    snapshot.routingMode.store ((int) s.routing, std::memory_order_relaxed);

    auto active = 0;

    for (int i = 0; i < kNumResonators; ++i)
    {
        if (partialEnergy[i] > 0.002f)
            ++active;

        snapshot.partialEnergy[i].store (std::min (partialEnergy[i] * 4.0f, 1.0f),
                                         std::memory_order_relaxed);
        snapshot.partialFreq[i].store (partialFreq[i], std::memory_order_relaxed);
    }

    snapshot.activePartials.store (active, std::memory_order_relaxed);
}

} // namespace harmonicresonator