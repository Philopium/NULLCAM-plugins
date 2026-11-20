// Tests for the HarmonicResonatorMode engine, without JUCE and without a host.
//
// The engine is the part that has to be right: the shaper has to make the
// harmonics the knobs promise, the bank has to ring at the frequencies the
// tuning says, the loop has to be a loop (not a crossfade) and nothing may
// ever blow up, at any sample rate, at any block size.
//
//   harmonicresonator_dsp_test

#include "HarmonicResonatorEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, const char* what)
{
    std::printf ("  %s  %s\n", condition ? "ok  " : "FAIL", what);

    if (! condition)
        ++failures;
}

constexpr double kSampleRate = 48000.0;
constexpr double kTwoPi = 6.283185307179586;

struct Result
{
    std::vector<float> left;
    std::vector<float> right;
};

// Runs the engine over a mono input and returns both outputs.
Result run (harmonicresonator::Engine& engine, const std::vector<float>& input,
            double sampleRate = kSampleRate, int blockSize = 64)
{
    Result result;
    result.left.assign (input.size(), 0.0f);
    result.right.assign (input.size(), 0.0f);

    std::vector<float> inL ((std::size_t) blockSize, 0.0f);
    std::vector<float> inR ((std::size_t) blockSize, 0.0f);
    std::vector<float> outL ((std::size_t) blockSize, 0.0f);
    std::vector<float> outR ((std::size_t) blockSize, 0.0f);

    for (std::size_t pos = 0; pos < input.size(); pos += (std::size_t) blockSize)
    {
        const auto count = (int) std::min ((std::size_t) blockSize, input.size() - pos);

        for (int i = 0; i < count; ++i)
        {
            inL[(std::size_t) i] = input[pos + (std::size_t) i];
            inR[(std::size_t) i] = input[pos + (std::size_t) i];
        }

        for (int i = count; i < blockSize; ++i)
        {
            inL[(std::size_t) i] = 0.0f;
            inR[(std::size_t) i] = 0.0f;
        }

        const float* inputs[2] = { inL.data(), inR.data() };
        float* outputs[2] = { outL.data(), outR.data() };

        engine.process (inputs, outputs, 2, blockSize);

        for (int i = 0; i < count; ++i)
        {
            result.left[pos + (std::size_t) i] = outL[(std::size_t) i];
            result.right[pos + (std::size_t) i] = outR[(std::size_t) i];
        }
    }

    return result;
}

std::vector<float> silence (double seconds, double sampleRate = kSampleRate)
{
    return std::vector<float> ((std::size_t) (seconds * sampleRate), 0.0f);
}

std::vector<float> sine (double frequency, double seconds, float amplitude = 0.35f,
                         double sampleRate = kSampleRate)
{
    std::vector<float> signal ((std::size_t) (seconds * sampleRate), 0.0f);

    for (std::size_t i = 0; i < signal.size(); ++i)
        signal[i] = amplitude * (float) std::sin (kTwoPi * frequency * (double) i / sampleRate);

    return signal;
}

// A broadband strike, then silence: what is left afterwards was made by the
// plugin. An 8-sample click would be a fair impulse, but a band pass this
// narrow only passes a sliver of it; a short noise burst is what a struck
// object actually gets, and it leaves a measurable ring behind.
std::vector<float> strike (double seconds, double burstSeconds = 0.15,
                           float amplitude = 0.6f, double sampleRate = kSampleRate)
{
    std::vector<float> signal ((std::size_t) (seconds * sampleRate), 0.0f);
    std::uint32_t state = 0x9e3779b9u;

    const auto burst = (std::size_t) (burstSeconds * sampleRate);

    for (std::size_t i = 0; i < burst && i < signal.size(); ++i)
    {
        state = state * 1664525u + 1013904223u;
        signal[i] = amplitude * ((float) ((state >> 9) & 0x7fffff) / 4194304.0f - 1.0f);
    }

    return signal;
}

// A tone that stops: what rings afterwards is the bank.
std::vector<float> burst (double frequency, double toneSeconds, double seconds,
                          float amplitude = 0.4f, double sampleRate = kSampleRate)
{
    auto signal = silence (seconds, sampleRate);

    const auto end = (std::size_t) (toneSeconds * sampleRate);

    for (std::size_t i = 0; i < end && i < signal.size(); ++i)
        signal[i] = amplitude * (float) std::sin (kTwoPi * frequency * (double) i / sampleRate);

    return signal;
}

// Amplitude of one frequency, by the Goertzel transform.
double tone (const std::vector<float>& signal, double frequency, double sampleRate,
             std::size_t from, std::size_t to)
{
    if (to > signal.size())
        to = signal.size();

    if (to <= from)
        return 0.0;

    const auto w = kTwoPi * frequency / sampleRate;
    const auto c = 2.0 * std::cos (w);

    double s1 = 0.0;
    double s2 = 0.0;

    for (auto i = from; i < to; ++i)
    {
        const auto s0 = (double) signal[i] + c * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    const auto power = s1 * s1 + s2 * s2 - c * s1 * s2;

    return std::sqrt (std::max (0.0, power)) / (double) (to - from);
}

double peakOf (const std::vector<float>& signal)
{
    auto peak = 0.0;

    for (auto value : signal)
        peak = std::max (peak, (double) std::abs (value));

    return peak;
}

double energyOf (const std::vector<float>& signal, std::size_t from, std::size_t to)
{
    if (to > signal.size())
        to = signal.size();

    auto energy = 0.0;

    for (auto i = from; i < to; ++i)
        energy += (double) signal[i] * (double) signal[i];

    return energy;
}

bool allFinite (const std::vector<float>& signal)
{
    for (auto value : signal)
        if (! std::isfinite (value))
            return false;

    return true;
}

harmonicresonator::Settings baseSettings()
{
    harmonicresonator::Settings s;
    s.inputGain = 1.0f;
    s.outputGain = 1.0f;
    s.mix = 1.0f;
    s.fundamental = 0.0f;
    s.interaction = 0.0f;
    s.feedback = 0.0f;
    s.resGain = 0.0f;
    return s;
}

double difference (const std::vector<float>& a, const std::vector<float>& b)
{
    auto sum = 0.0;

    for (std::size_t i = 0; i < std::min (a.size(), b.size()); ++i)
    {
        const auto d = (double) a[i] - (double) b[i];
        sum += d * d;
    }

    return std::sqrt (sum / (double) std::max<std::size_t> (1, a.size()));
}
} // namespace

int main()
{
    using namespace harmonicresonator;

    std::printf ("HarmonicResonatorMode engine\n\n");

    //--------------------------------------------------------------------------
    std::printf ("silence and first principles:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);

        auto s = baseSettings();
        s.mix = 0.8f;
        s.resGain = 0.7f;
        s.interaction = 0.6f;
        s.feedback = 0.6f;
        engine.setSettings (s);
        engine.reset();

        const auto out = run (engine, silence (0.25));

        check (peakOf (out.left) < 1.0e-6 && peakOf (out.right) < 1.0e-6,
               "silence in stays silence out, even with the loop wide open");
        check (allFinite (out.left), "the output is always a real number");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nthe nonlinear stage:\n");
    {
        // Only the shaper: no bank, no loop, full wet.
        auto distortionOnly = [&] (Shape shape, float evenOdd, float drive)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.shape = shape;
            s.evenOdd = evenOdd;
            s.drive = drive;
            s.density = 1.0f;
            s.brightness = 0.5f;
            engine.setSettings (s);
            engine.reset();

            return run (engine, sine (220.0, 0.60));
        };

        const auto hard = distortionOnly (Shape::hard, -1.0f, 0.80f);
        const auto fund = tone (hard.left, 220.0, kSampleRate, 12000, 28000);
        const auto h2 = tone (hard.left, 440.0, kSampleRate, 12000, 28000);
        const auto h3 = tone (hard.left, 660.0, kSampleRate, 12000, 28000);
        const auto h5 = tone (hard.left, 1100.0, kSampleRate, 12000, 28000);

        std::printf ("    odd only:  220 %.4f, 440 %.4f, 660 %.4f, 1100 %.4f\n",
                     fund, h2, h3, h5);

        check (h3 > 0.002 || h5 > 0.002, "drive generates harmonics above the fundamental");
        check (h3 > h2 * 3.0, "the odd setting keeps the second harmonic down");
        check (h5 > h2 * 0.5, "the fifth harmonic is there too");

        const auto even = distortionOnly (Shape::hard, 1.0f, 0.80f);
        const auto e2 = tone (even.left, 440.0, kSampleRate, 12000, 28000);
        const auto e3 = tone (even.left, 660.0, kSampleRate, 12000, 28000);

        std::printf ("    even only: 440 %.4f, 660 %.4f\n", e2, e3);

        check (e2 > e3 * 2.0, "the even setting makes the second harmonic dominate");

        const auto soft = distortionOnly (Shape::soft, 0.0f, 0.15f);
        const auto hardDrive = distortionOnly (Shape::soft, 0.0f, 0.90f);

        const auto softThird = tone (soft.left, 660.0, kSampleRate, 12000, 28000);
        const auto hardThird = tone (hardDrive.left, 660.0, kSampleRate, 12000, 28000);

        std::printf ("    soft: 660 at drive 0.15 = %.5f, at drive 0.90 = %.5f\n",
                     softThird, hardThird);

        check (hardThird > softThird * 4.0, "more drive means more harmonic energy");

        const auto fold = distortionOnly (Shape::fold, 0.0f, 0.85f);
        check (difference (fold.left, hardDrive.left) > 0.01,
               "the folding shape does not sound like the soft one");

        bool shapesBounded = true;

        for (int s = 0; s < 7; ++s)
        {
            const auto out = distortionOnly ((Shape) s, 0.0f, 0.95f);

            if (! allFinite (out.left) || peakOf (out.left) > 1.2)
                shapesBounded = false;
        }

        check (shapesBounded, "all seven shapers stay inside the rails at full drive");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nthe resonator bank:\n");
    {
        // A broadband strike excites every partial at once, so the tuning
        // itself can be read back from the ring.
        auto ring = [&] (Tuning tuning, float tune, float decay, float seconds)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 1.0f;
            s.resonance = 0.8f;
            s.spread = 0.0f;
            s.decay = decay;
            s.tune = tune;
            s.tuning = tuning;
            s.drive = 0.5f;
            s.density = 0.9f;
            engine.setSettings (s);
            engine.reset();

            return run (engine, strike (seconds));
        };

        const auto harmonic = ring (Tuning::harmonic, 220.0f, 1.5f, 1.0f);

        const auto at220 = tone (harmonic.left, 220.0, kSampleRate, 19200, 46080);
        const auto at330 = tone (harmonic.left, 330.0, kSampleRate, 19200, 46080);
        const auto at440 = tone (harmonic.left, 440.0, kSampleRate, 19200, 46080);

        std::printf ("    harmonic 220: 220 %.6f, 330 %.6f, 440 %.6f\n",
                     at220, at330, at440);

        check (at220 > at330 * 10.0, "the bank rings on its tuned fundamental");
        check (at440 > at330 * 5.0, "and on its own partials, not between them");

        const auto high = ring (Tuning::harmonic, 440.0f, 1.5f, 1.0f);
        const auto high440 = tone (high.left, 440.0, kSampleRate, 19200, 46080);
        const auto high220 = tone (high.left, 220.0, kSampleRate, 19200, 46080);

        std::printf ("    harmonic 440: 220 %.6f, 440 %.6f\n", high220, high440);

        check (high440 > high220 * 10.0, "turning TUNE up moves the whole bank up");

        const auto odd = ring (Tuning::odd, 220.0f, 1.5f, 1.0f);
        const auto odd3 = tone (odd.left, 660.0, kSampleRate, 19200, 46080);
        const auto odd2 = tone (odd.left, 440.0, kSampleRate, 19200, 46080);

        std::printf ("    odd tuning: 440 %.6f, 660 %.6f\n", odd2, odd3);

        check (odd3 > odd2 * 5.0, "the odd tuning leaves the even partials out");

        const auto metallic = ring (Tuning::metallic, 220.0f, 1.5f, 1.0f);
        const auto metalRatio = tone (metallic.left, 220.0 * 1.47, kSampleRate, 19200, 46080);
        const auto metalSecond = tone (metallic.left, 440.0, kSampleRate, 19200, 46080);

        std::printf ("    metallic: 323 %.6f, 440 %.6f\n", metalRatio, metalSecond);

        check (metalRatio > metalSecond * 5.0, "the metallic tuning is inharmonic");

        // The tail has to outlive the note.
        const auto shortDecay = ring (Tuning::harmonic, 220.0f, 0.10f, 1.0f);
        const auto longDecay = ring (Tuning::harmonic, 220.0f, 4.0f, 1.0f);

        const auto shortTail = energyOf (shortDecay.left, 24000, 43200);
        const auto longTail = energyOf (longDecay.left, 24000, 43200);

        std::printf ("    tail energy after 0.5 s: short %.9f, long %.9f\n",
                     shortTail, longTail);

        check (longTail > shortTail * 50.0, "a long decay rings for a long time");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nthe interaction between the two stages:\n");
    {
        // The loop is fed by the bank, so the bank has to be excited for the
        // loop to have anything to say. A 220 Hz note into a 220 Hz harmonic
        // bank is the case the design is about: the distortion throws up
        // harmonics that land exactly on the partials, the partials come back
        // into the distortion, and the two stages drive each other.
        auto withLoop = [&] (float interaction, float feedback)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 0.9f;
            s.decay = 0.30f;          // short: after the note any sound is the loop
            s.resonance = 0.7f;
            s.drive = 0.55f;
            s.density = 0.85f;
            s.tune = 220.0f;
            s.tuning = Tuning::harmonic;
            s.interaction = interaction;
            s.feedback = feedback;
            s.routing = Routing::bidirectional;
            s.source = Source::res;
            s.destination = Destination::both;
            s.lowCut = 90.0f;
            s.highCut = 12000.0f;
            engine.setSettings (s);
            engine.reset();

            return run (engine, burst (220.0, 0.40, 1.60));
        };

        const auto quietLoop = withLoop (0.0f, 0.0f);
        const auto hotLoop   = withLoop (0.90f, 0.95f);

        const auto differenceRms = difference (quietLoop.left, hotLoop.left);

        std::printf ("    rms difference between a closed and an open loop: %.5f\n", differenceRms);

        check (differenceRms > 0.01, "INTERACTION changes the sound, not just the level");

        // After the note has stopped, a 0.3 s bank is silent. Whatever is
        // still there was put back into the loop.
        const auto openTail = energyOf (quietLoop.left, 38400, 72000);
        const auto closedTail = energyOf (hotLoop.left, 38400, 72000);

        std::printf ("    tail after the note: loop open %.9f, loop closed %.9f\n",
                     openTail, closedTail);

        check (closedTail > 1.0e-6 && closedTail > openTail * 20.0,
               "the closed loop keeps the bank fed after the input stops");

        // Whatever the loop does, it stays bounded: the saturators in the
        // return edge and on the output see to that.
        const auto hotPeak = peakOf (hotLoop.left);
        std::printf ("    loudest sample with the loop closed: %.4f\n", hotPeak);
        check (hotPeak < 1.2, "the closed loop saturates instead of running away");

        // The two routings have to be different from each other too.
        Engine reverseEngine;
        reverseEngine.prepare (kSampleRate);

        auto reverseSettings = baseSettings();
        reverseSettings.mix = 1.0f;
        reverseSettings.resGain = 0.9f;
        reverseSettings.decay = 0.30f;
        reverseSettings.resonance = 0.7f;
        reverseSettings.drive = 0.55f;
        reverseSettings.density = 0.85f;
        reverseSettings.tune = 220.0f;
        reverseSettings.tuning = Tuning::harmonic;
        reverseSettings.interaction = 0.90f;
        reverseSettings.feedback = 0.95f;
        reverseSettings.routing = Routing::resToDist;
        reverseSettings.source = Source::res;
        reverseSettings.destination = Destination::both;
        reverseSettings.lowCut = 90.0f;
        reverseSettings.highCut = 12000.0f;
        reverseEngine.setSettings (reverseSettings);
        reverseEngine.reset();

        const auto reverse = run (reverseEngine, burst (220.0, 0.40, 1.60));

        check (difference (hotLoop.left, reverse.left) > 0.01,
               "RES > DIST and RES < > DIST are different signal paths");

        // Everything at once, for a while: still a real number, still bounded.
        auto sweepOk = true;
        auto loudest = 0.0;

        for (int routing = 0; routing < 3 && sweepOk; ++routing)
            for (int shape = 0; shape < 7; ++shape)
                for (int tuning = 0; tuning < 5; ++tuning)
                {
                    Engine engine;
                    engine.prepare (kSampleRate);

                    auto s = baseSettings();
                    s.mix = 1.0f;
                    s.resGain = 1.0f;
                    s.drive = 1.0f;
                    s.interaction = 1.0f;
                    s.feedback = 1.0f;
                    s.fbDrive = 1.0f;
                    s.decay = 6.0f;
                    s.resonance = 1.0f;
                    s.shape = (Shape) shape;
                    s.routing = (Routing) routing;
                    s.tuning = (Tuning) tuning;
                    s.destination = Destination::both;
                    s.source = Source::combined;
                    engine.setSettings (s);
                    engine.reset();

                    const auto out = run (engine, sine (180.0, 0.30, 0.9f));

                    loudest = std::max (loudest, peakOf (out.left));

                    if (! allFinite (out.left) || ! allFinite (out.right) || loudest > 1.2)
                        sweepOk = false;
                }

        std::printf ("    loudest output over 105 extreme settings: %.4f\n", loudest);

        check (sweepOk, "105 extreme settings never blow up and never leave the rails");
    }

    //--------------------------------------------------------------------------
    std::printf ("\npitch tracking and alignment:\n");
    {
        // The tracker follows a note.
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 1.0f;
            s.decay = 3.0f;
            s.resonance = 0.8f;
            s.spread = 0.0f;
            s.drive = 0.4f;
            s.tune = 110.0f;          // deliberately wrong
            s.align = 1.0f;
            engine.setSettings (s);
            engine.reset();

            run (engine, sine (330.0, 1.2));

            const auto tracked = engine.getSnapshot().tracked.load();

            std::printf ("    tracker found %.1f Hz (tune was 110)\n", (double) tracked);

            check (std::abs (tracked - 330.0) < 12.0, "the tracker finds the input fundamental");
        }

        // ALIGN moves the bank onto that pitch. The probe note is 250 Hz and
        // the knob is 110: 250 is not a partial of a 110 harmonic series, so
        // only an aligned bank reinforces it.
        auto aligned = [&] (float align, float& bankHz)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 1.0f;
            s.decay = 1.5f;
            s.resonance = 0.8f;
            s.spread = 0.0f;
            s.tune = 110.0f;
            s.align = align;
            engine.setSettings (s);
            engine.reset();

            // Where the bank sits while the note is sounding, and what it
            // leaves behind when the note stops.
            run (engine, sine (250.0, 0.40));
            bankHz = engine.getSnapshot().partialFreq[0].load();

            engine.reset();
            engine.setSettings (s);

            return run (engine, burst (250.0, 0.50, 1.60));
        };

        float alignBankHz = 0.0f;
        float freeBankHz = 0.0f;

        const auto withAlign = aligned (1.0f, alignBankHz);
        const auto free = aligned (0.0f, freeBankHz);

        // After the note has stopped the distortion has nothing left to say,
        // so the 250 Hz ring is the bank's own voice.
        const auto align250 = tone (withAlign.left, 250.0, kSampleRate, 24000, 45600);
        const auto free250 = tone (free.left, 250.0, kSampleRate, 24000, 45600);

        std::printf ("    bank at %.1f Hz (ALIGN on) and %.1f Hz (off)\n",
                     (double) alignBankHz, (double) freeBankHz);
        std::printf ("    250 Hz ring after the note: ALIGN on %.6f, off %.6f\n",
                     align250, free250);

        check (std::abs (alignBankHz - 250.0f) < 6.0f, "ALIGN retunes the bank to the note");
        check (align250 > free250 * 10.0, "and the bank keeps the note ringing");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nmodulation:\n");
    {
        auto modulated = [&] (float depth, float rate)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 0.9f;
            s.decay = 1.5f;
            s.interaction = 0.5f;
            s.feedback = 0.5f;
            s.modDest = ModDest::tune;
            s.modRate = rate;
            s.modDepth = depth;
            engine.setSettings (s);
            engine.reset();

            return run (engine, sine (220.0, 1.0));
        };

        const auto still = modulated (0.0f, 4.0f);
        const auto moving = modulated (0.6f, 4.0f);

        check (difference (still.left, moving.left) > 0.005,
               "the LFO really moves the bank when the depth is up");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nsample rates and block sizes:\n");
    {
        bool ratesOk = true;

        for (const auto sr : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        {
            Engine engine;
            engine.prepare (sr);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 0.8f;
            s.decay = 2.0f;
            s.resonance = 0.7f;
            s.tune = 220.0f;
            s.tuning = Tuning::harmonic;
            s.spread = 0.0f;
            s.drive = 0.70f;
            s.interaction = 0.80f;
            s.feedback = 0.80f;
            engine.setSettings (s);
            engine.reset();

            const auto out = run (engine, sine (220.0, 0.60, 0.6f, sr), sr);

            const auto at220 = tone (out.left, 220.0, sr,
                                     (std::size_t) (0.20 * sr), (std::size_t) (0.55 * sr));
            const auto at300 = tone (out.left, 300.0, sr,
                                     (std::size_t) (0.20 * sr), (std::size_t) (0.55 * sr));

            std::printf ("    %.0f Hz: peak %.4f, 220 Hz %.5f, 300 Hz %.5f\n",
                         sr, peakOf (out.left), at220, at300);

            if (! allFinite (out.left) || peakOf (out.left) > 1.2 || at220 < at300)
                ratesOk = false;
        }

        check (ratesOk, "the bank is tuned and bounded at 44.1, 48, 88.2, 96 and 192 kHz");

        // With the modulation still, nothing on the per-sample path depends on
        // how many samples the host hands over at a time. The first block is
        // left out: the resonator coefficients are ramped across it on purpose,
        // which is what stops a knob move from clicking.
        auto blockRms = [&] (int blockSize)
        {
            Engine engine;
            engine.prepare (kSampleRate);

            auto s = baseSettings();
            s.mix = 1.0f;
            s.resGain = 0.8f;
            s.decay = 2.0f;
            s.tune = 220.0f;
            s.tuning = Tuning::harmonic;
            s.drive = 0.6f;
            s.interaction = 0.7f;
            s.feedback = 0.7f;
            s.modDepth = 0.0f;
            s.envDepth = 0.0f;
            engine.setSettings (s);
            engine.reset();

            return run (engine, burst (220.0, 0.50, 0.90, 0.5f), kSampleRate, blockSize);
        };

        const auto small = blockRms (16);
        const auto large = blockRms (1024);

        // Compare the tails, from 0.05 s on.
        Result trimmedSmall, trimmedLarge;
        trimmedSmall.left.assign (small.left.begin() + 2400, small.left.end());
        trimmedLarge.left.assign (large.left.begin() + 2400, large.left.end());

        const auto rms = difference (trimmedSmall.left, trimmedLarge.left);

        std::printf ("    rms difference between 16 and 1024 sample blocks: %.9f\n", rms);

        check (rms < 1.0e-6, "the output does not depend on the block size");
    }

    std::printf ("\n%s\n", failures == 0 ? "all checks passed"
                                        : (std::to_string (failures) + " check(s) failed").c_str());

    return failures == 0 ? 0 : 1;
}
