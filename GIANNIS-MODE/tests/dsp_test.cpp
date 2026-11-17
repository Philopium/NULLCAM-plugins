// The GIANNIS-MODE engine, measured without a host.
//
// A played note goes in; the divider, the two sub voices and the rectified
// octave up are checked one at a time, then the fuzz and the limiter.

#include "GiannisEngine.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void expect (bool condition, const char* what)
{
    std::printf ("%-64s %s\n", what, condition ? "ok" : "FAILED");

    if (! condition)
        ++failures;
}

constexpr double kRate = 48000.0;
constexpr double kPi = 3.14159265358979323846;

std::vector<float> sine (double hz, double seconds, float level = 0.5f)
{
    std::vector<float> out ((std::size_t) (seconds * kRate));

    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = level * (float) std::sin (2.0 * kPi * hz * (double) i / kRate);

    return out;
}

std::vector<float> silence (double seconds)
{
    return std::vector<float> ((std::size_t) (seconds * kRate), 0.0f);
}

// Counts sign changes, which is a rough but honest frequency meter for these
// single voice checks. The first and last 20 ms are skipped so the filters can
// settle.
double crossingsPerSecond (const std::vector<float>& buffer)
{
    const auto skip = (std::size_t) (0.05 * kRate);

    if (buffer.size() < skip * 2)
        return 0.0;

    auto crossings = 0;
    auto previous = buffer[skip];

    for (std::size_t i = skip; i < buffer.size(); ++i)
    {
        if ((buffer[i] >= 0.0f) != (previous >= 0.0f))
            ++crossings;

        previous = buffer[i];
    }

    const auto seconds = (double) (buffer.size() - skip) / kRate;
    return (double) crossings / seconds;
}

float peakOf (const std::vector<float>& buffer)
{
    auto peak = 0.0f;

    for (const auto value : buffer)
        peak = std::max (peak, std::abs (value));

    return peak;
}

double rmsOf (const std::vector<float>& buffer)
{
    double sum = 0.0;

    for (const auto value : buffer)
        sum += double (value) * double (value);

    return buffer.empty() ? 0.0 : std::sqrt (sum / (double) buffer.size());
}

std::vector<float> play (giannis::Engine& engine, const std::vector<float>& input)
{
    std::vector<float> output (input.size(), 0.0f);

    constexpr int block = 512;

    for (std::size_t offset = 0; offset < input.size(); offset += block)
    {
        const auto count = (int) std::min ((std::size_t) block, input.size() - offset);
        const float* in[1] = { input.data() + offset };
        float* out[1] = { output.data() + offset };
        engine.process (in, out, 1, count);
    }

    return output;
}

giannis::Settings bare()
{
    giannis::Settings s;
    s.sub = 0.0f;
    s.doom = 0.0f;
    s.oct = 0.0f;
    s.fuzz = 0.0f;
    s.tone = 1.0f;
    s.level = 1.0f;
    s.dry = 0.0f;
    return s;
}
} // namespace

int main()
{
    //--------------------------------------------------------------------------
    // 1. The divider: 110 Hz in, 55 Hz sub out.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.sub = 1.0f;
        engine.setSettings (settings);

        const auto output = play (engine, sine (110.0, 2.0));

        const auto rate = crossingsPerSecond (output);

        expect (std::abs (rate - 110.0) < 12.0, "SUB plays the played note one octave down (55 Hz)");
        expect (peakOf (output) > 0.25f, "and the sub is loud, not a hint");

        expect (std::abs (engine.getTrackedFrequency() - 110.0f) < 4.0f,
                "the divider tracks 110 Hz for the panel");
        expect (engine.getSnapshot().note.load() == 45, "110 Hz is MIDI note 45 (A2)");
    }

    //--------------------------------------------------------------------------
    // 2. DOOM is two octaves down: 110 Hz in, 27.5 Hz out.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.doom = 1.0f;
        engine.setSettings (settings);

        const auto output = play (engine, sine (110.0, 3.0));

        const auto rate = crossingsPerSecond (output);

        expect (std::abs (rate - 55.0) < 8.0, "DOOM plays two octaves down (27.5 Hz)");
    }

    //--------------------------------------------------------------------------
    // 3. The octave up is a rectifier: 110 Hz in, 220 Hz out, same attack.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.oct = 1.0f;
        engine.setSettings (settings);

        const auto output = play (engine, sine (110.0, 1.5));

        const auto rate = crossingsPerSecond (output);

        expect (std::abs (rate - 440.0) < 25.0, "OCT plays one octave up (220 Hz)");
    }

    //--------------------------------------------------------------------------
    // 4. With everything at zero the pedal is a wire.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.dry = 1.0f;
        engine.setSettings (settings);

        const auto input = sine (110.0, 0.5);
        const auto output = play (engine, input);

        auto difference = 0.0f;

        for (std::size_t i = 0; i < input.size(); ++i)
            difference = std::max (difference, std::abs (input[i] - output[i]));

        expect (difference < 1.0e-6f, "with the octaves off the dry signal passes untouched");
    }

    //--------------------------------------------------------------------------
    // 5. Silence in, silence out.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.dry = 1.0f;
        settings.sub = 1.0f;
        settings.doom = 1.0f;
        settings.oct = 1.0f;
        engine.setSettings (settings);

        const auto output = play (engine, silence (1.0));

        expect (peakOf (output) < 1.0e-5f, "silence stays silence");
        expect (engine.getSnapshot().active.load() == 0, "and nothing is reported as tracked");
    }

    //--------------------------------------------------------------------------
    // 6. FUZZ makes the voice dirty and louder (it is a saturation).
    {
        giannis::Engine cleanEngine, dirtyEngine;
        cleanEngine.prepare (kRate);
        dirtyEngine.prepare (kRate);

        auto settings = bare();
        settings.dry = 1.0f;
        cleanEngine.setSettings (settings);

        settings.fuzz = 1.0f;
        dirtyEngine.setSettings (settings);

        const auto input = sine (110.0, 1.0, 0.4f);
        const auto clean = play (cleanEngine, input);
        const auto dirty = play (dirtyEngine, input);

        expect (rmsOf (dirty) > rmsOf (clean) * 1.15, "FUZZ drives the voice into saturation");
        expect (peakOf (dirty) <= 1.0001f, "and never leaves the rails");
    }

    //--------------------------------------------------------------------------
    // 7. Everything at maximum still comes out of a limiter, not of a speaker.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        giannis::Settings s;
        s.sub = 1.0f;
        s.doom = 1.0f;
        s.oct = 1.0f;
        s.fuzz = 1.0f;
        s.tone = 0.2f;
        s.level = 1.5f;
        engine.setSettings (s);

        const auto output = play (engine, sine (82.41, 1.5, 0.8f));

        expect (peakOf (output) <= 1.0001f, "maximum everything stays inside +/-1");
        expect (peakOf (output) > 0.5f, "and it is still a bass, not a whisper");
    }

    //--------------------------------------------------------------------------
    // 8. LEVEL at zero is a mute.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        giannis::Settings s;
        s.level = 0.0f;
        engine.setSettings (s);

        expect (peakOf (play (engine, sine (110.0, 0.4))) < 1.0e-6f, "LEVEL 0 is silent");
    }

    //--------------------------------------------------------------------------
    // 9. A DC input does not become a speaker pushed out of its basket.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.sub = 1.0f;
        settings.doom = 1.0f;
        engine.setSettings (settings);

        std::vector<float> input ((std::size_t) (1.5 * kRate), 0.30f);
        const auto output = play (engine, input);

        auto mean = 0.0;

        for (std::size_t i = (std::size_t) (1.0 * kRate); i < output.size(); ++i)
            mean += output[i];

        mean /= (double) (output.size() - (std::size_t) (1.0 * kRate));

        expect (std::abs (mean) < 0.01, "DC on the input is blocked, not amplified");
    }

    //--------------------------------------------------------------------------
    // 10. The sub follows a new note, and stops when the note stops.
    {
        giannis::Engine engine;
        engine.prepare (kRate);

        auto settings = bare();
        settings.sub = 1.0f;
        engine.setSettings (settings);

        auto input = sine (110.0, 0.8);
        const auto second = sine (220.0, 0.8);
        input.insert (input.end(), second.begin(), second.end());

        const auto output = play (engine, input);

        const auto early = std::vector<float> (output.begin() + (std::size_t) (0.3 * kRate),
                                               output.begin() + (std::size_t) (0.7 * kRate));
        const auto late = std::vector<float> (output.begin() + (std::size_t) (1.1 * kRate),
                                              output.begin() + (std::size_t) (1.5 * kRate));

        expect (std::abs (crossingsPerSecond (early) - 110.0) < 12.0,
                "the sub tracks the first note");
        expect (std::abs (crossingsPerSecond (late) - 220.0) < 18.0,
                "and follows when the player moves up an octave");
    }

    //--------------------------------------------------------------------------
    // 11. The helpers the panel uses.
    {
        expect (std::string (giannis::noteLabel (45)) == "A2", "note 45 is A2");
        expect (std::string (giannis::noteLabel (-1)) == "--", "no note prints as --");
        expect (std::abs (giannis::Engine::frequencyFromMidi (45) - 110.0f) < 0.01f,
                "A2 is 110 Hz");
    }

    if (failures == 0)
        std::printf ("\nall checks passed\n");
    else
        std::printf ("\n%d check(s) failed\n", failures);

    return failures == 0 ? 0 : 1;
}
