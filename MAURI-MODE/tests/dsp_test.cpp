// The slicer, measured without a host: silence stays silent, the input can be the
// clock, the time signature drives the grid, and the playing dynamics really do
// turn a simple pattern into a complex one.

#include "MauriEngine.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void expect (bool condition, const char* what)
{
    std::printf ("%-58s %s\n", what, condition ? "ok" : "FAILED");

    if (! condition)
        ++failures;
}

constexpr double kSampleRate = 48000.0;

mauri::Settings defaults()
{
    mauri::Settings s;
    s.clock = 0;       // host
    s.timeSig = 0;     // 4/4
    s.rateIndex = 3;   // 1/16
    s.style = 0;       // GROOVE
    s.slice = 0.65f;
    s.repeat = 0.0f;
    s.reverse = 0.0f;
    s.scatter = 0.0f;
    s.dynPat = 1.0f;
    s.dynSlice = 0.5f;
    s.dynRate = 0.45f;
    s.sens = 0.55f;
    s.tone = 1.0f;
    s.mix = 1.0f;
    s.output = 1.0f;
    return s;
}

std::vector<float> run (mauri::Slicer& slicer, const std::vector<float>& input,
                        int blockSize = 128)
{
    // The engine works in place, so the input is copied into the working buffer first.
    std::vector<float> output = input;

    for (std::size_t pos = 0; pos < output.size(); pos += (std::size_t) blockSize)
    {
        const auto count = (int) std::min ((std::size_t) blockSize, output.size() - pos);
        slicer.process (output.data() + pos, output.data() + pos, count);
    }

    return output;
}

std::vector<float> impulse (std::size_t count, std::size_t at, float amplitude = 0.8f)
{
    std::vector<float> signal (count, 0.0f);

    for (std::size_t i = at; i < std::min (at + 60, count); ++i)
        signal[i] = amplitude * std::exp (-(float) (i - at) / 12.0f);

    return signal;
}

std::vector<float> tone (std::size_t count, float amplitude, float hz = 220.0f)
{
    std::vector<float> signal (count, 0.0f);

    for (std::size_t i = 0; i < count; ++i)
        signal[i] = amplitude * std::sin ((float) (2.0 * 3.14159265358979 * hz * (double) i / kSampleRate));

    return signal;
}

bool finite (const std::vector<float>& signal)
{
    for (auto v : signal)
        if (! std::isfinite (v))
            return false;

    return true;
}

int countEvents (int timeSig, int style, float energy)
{
    int n = 0;

    for (int i = 0; i < mauri::stepsForTimeSig (timeSig); ++i)
        if (mauri::evaluateStep (timeSig, style, i, energy).on)
            ++n;

    return n;
}
} // namespace

int main()
{
    // ---- silence in, silence out -------------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);
        slicer.setSettings (defaults());

        const auto input = std::vector<float> (48000, 0.0f);
        const auto output = run (slicer, input);

        float peak = 0.0f;
        for (auto v : output)
            peak = std::max (peak, std::abs (v));

        expect (peak < 1.0e-6f, "silence in, silence out");
        expect (finite (output), "silence output is finite");
    }

    // ---- dry passthrough ----------------------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.mix = 0.0f;
        slicer.setSettings (settings);

        const auto input = tone (48000, 0.5f);
        const auto output = run (slicer, input);

        float worst = 0.0f;
        for (std::size_t i = 0; i < input.size(); ++i)
            worst = std::max (worst, std::abs (output[i] - input[i]));

        expect (worst < 1.0e-5f, "mix 0 passes the input through untouched");
    }

    // ---- output gain --------------------------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.mix = 0.0f;
        settings.output = 1.5f;
        slicer.setSettings (settings);

        const auto input = tone (48000, 0.4f);
        const auto output = run (slicer, input);

        float worst = 0.0f;
        for (std::size_t i = 0; i < input.size(); ++i)
            worst = std::max (worst, std::abs (output[i] - input[i] * 1.5f));

        expect (worst < 1.0e-4f, "output trim scales the signal");
    }

    // ---- the input is the clock --------------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.clock = 1;
        slicer.setSettings (settings);

        std::vector<float> input (96000, 0.0f);
        for (int i = 0; i < 6; ++i)
        {
            const auto burst = impulse (96000, (std::size_t) i * 9600);
            for (std::size_t n = 0; n < input.size(); ++n)
                input[n] += burst[n];
        }

        run (slicer, input);

        const auto& snapshot = slicer.getSnapshot();
        expect (snapshot.onsets.load() >= 5, "input clock sees the transients");
        expect (snapshot.step.load() != 0 || snapshot.gateOpen.load() == 1,
                "input clock advances the pattern");
    }

    // ---- silence does not move an input-clocked pattern ---------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.clock = 1;
        slicer.setSettings (settings);

        const auto input = std::vector<float> (96000, 0.0f);
        run (slicer, input);

        const auto& snapshot = slicer.getSnapshot();
        expect (snapshot.step.load() == 0 && snapshot.onsets.load() == 0,
                "input clock stays put on silence");
    }

    // ---- the host clock keeps the beat --------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.clock = 0;
        settings.rateIndex = 0;   // 1/4 at 120 bpm = 500 ms per step
        settings.timeSig = 3;     // 2/4 = 8 steps in the bar
        slicer.setSettings (settings);
        slicer.setTransport (120.0, true);

        const auto length = mauri::stepsForTimeSig (3);
        auto scratch = std::vector<float> (240000, 0.0f);
        int advances = 0;
        int previous = -1;

        for (std::size_t pos = 0; pos < scratch.size(); pos += 128)
        {
            const auto count = (int) std::min ((std::size_t) 128, scratch.size() - pos);
            slicer.process (scratch.data() + pos, scratch.data() + pos, count);

            const auto step = slicer.getSnapshot().step.load();

            if (previous >= 0 && step == (previous + 1) % length)
                ++advances;

            previous = step;
        }

        // five seconds at 500 ms per step: ten steps have to have gone by
        expect (advances >= 8 && advances <= 11, "host clock keeps the host tempo");
    }

    // ---- the chosen time signature sets the length of the bar ---------------
    {
        const auto steps44 = mauri::stepsForTimeSig (0);
        const auto steps58 = mauri::stepsForTimeSig (5);
        const auto steps78 = mauri::stepsForTimeSig (6);

        expect (steps44 == 16 && steps58 == 10 && steps78 == 14,
                "time signatures give 16 / 10 / 14 steps");
        expect (std::string (mauri::kTimeSigsInfo[6].name) == "7/8",
                "the odd signature is labelled 7/8");
    }

    // ---- weak dynamics = simple pattern, strong = complex ------------------
    {
        const auto soft = countEvents (0, 0, 0.10f);
        const auto mid = countEvents (0, 0, 0.50f);
        const auto hard = countEvents (0, 0, 0.95f);

        expect (soft >= 1 && soft <= 3, "soft playing makes a bare pattern");
        expect (mid > soft && mid < hard, "the pattern thickens with the dynamics");
        expect (hard >= 12, "hard playing fills the bar");
    }

    // ---- the same rule holds for every style --------------------------------
    {
        bool monotone = true;

        for (int style = 0; style < mauri::kStyles; ++style)
        {
            const auto soft = countEvents (0, style, 0.08f);
            const auto hard = countEvents (0, style, 0.98f);

            if (hard < soft)
                monotone = false;
        }

        expect (monotone, "every style gets denser as the dynamics rise");
    }

    // ---- complexity tiers gate the fancy moves ------------------------------
    {
        const auto pulse = mauri::evaluateStep (0, 0, 0, 0.10f);
        const auto shred = mauri::evaluateStep (0, 0, 3, 0.95f);

        expect (! pulse.reverse && pulse.extraRepeats == 0,
                "a soft hand gets plain forward chops only");
        expect (shred.extraRepeats >= 1 || shred.mode != mauri::Mode::live,
                "a hard hand unlocks ratchets and alternate modes");
    }

    // ---- dynamics reach the audio: harder hit = shorter slice ---------------
    {
        auto measure = [] (float amplitude)
        {
            mauri::Slicer slicer;
            slicer.prepare (kSampleRate);

            auto settings = defaults();
            settings.clock = 0;
            settings.rateIndex = 3;
            settings.style = 6;        // TREM: always firing, so the slice is measurable
            settings.slice = 0.5f;
            settings.dynSlice = 1.0f;
            settings.mix = 1.0f;
            slicer.setSettings (settings);
            slicer.setTransport (120.0, true);

            const auto input = tone (96000, amplitude);
            run (slicer, input);

            return slicer.getSnapshot().sliceFraction.load();
        };

        const auto soft = measure (0.05f);
        const auto hard = measure (0.9f);

        expect (hard > 0.0f && soft > 0.0f, "the slice length is measured");
        expect (hard < soft * 0.8f, "a harder hit slices shorter");
    }

    // ---- the pattern energy follows the playing -----------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.clock = 0;
        settings.rateIndex = 3;
        settings.style = 0;
        slicer.setSettings (settings);
        slicer.setTransport (120.0, true);

        const auto quiet = tone (96000, 0.02f);
        run (slicer, quiet);
        const auto quietEnergy = slicer.getSnapshot().patternEnergy.load();

        const auto loud = tone (96000, 0.95f);
        run (slicer, loud);
        const auto loudEnergy = slicer.getSnapshot().patternEnergy.load();

        expect (quietEnergy < 0.35f, "soft playing keeps the pattern simple");
        expect (loudEnergy > 0.65f, "hard playing makes the pattern complex");
        expect (loudEnergy > quietEnergy, "the energy follows the dynamics");
    }

    // ---- live slicing chops the input in real time --------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.clock = 0;
        settings.rateIndex = 3;
        settings.style = 6;     // TREM chops every step
        settings.slice = 0.5f;
        settings.mix = 1.0f;
        settings.tone = 1.0f;
        slicer.setSettings (settings);
        slicer.setTransport (120.0, true);

        // A continuous tone: the output must be chopped, not a copy.
        const auto input = tone (48000, 0.5f);
        const auto output = run (slicer, input);

        int silentChunks = 0;
        const auto chunk = 512;

        for (std::size_t pos = 0; pos + chunk <= output.size(); pos += chunk)
        {
            float peak = 0.0f;
            for (std::size_t i = pos; i < pos + chunk; ++i)
                peak = std::max (peak, std::abs (output[i]));

            if (peak < 1.0e-4f)
                ++silentChunks;
        }

        expect (silentChunks > 0, "the live chop really gates the signal");
        expect (finite (output), "the chopped signal is finite");
    }

    // ---- wet signal stays clean ---------------------------------------------
    {
        mauri::Slicer slicer;
        slicer.prepare (kSampleRate);

        auto settings = defaults();
        settings.mix = 1.0f;
        settings.scatter = 0.5f;
        settings.reverse = 0.5f;
        slicer.setSettings (settings);
        slicer.setTransport (128.0, true);

        auto input = impulse (96000, 1000);
        const auto extra = impulse (96000, 40000);
        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] += extra[i];

        const auto output = run (slicer, input);

        float peak = 0.0f;
        for (auto v : output)
            peak = std::max (peak, std::abs (v));

        expect (finite (output), "the wet signal is finite");
        expect (peak < 4.0f, "the wet signal stays inside the rails");
    }

    std::printf ("\n%s (%d failed)\n", failures == 0 ? "ALL OK" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
