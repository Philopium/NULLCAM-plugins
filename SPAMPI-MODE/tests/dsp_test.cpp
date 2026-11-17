// The pitch follower, measured without a host.
//
// No MIDI anywhere: a played note goes in, the detector finds its pitch, and the
// playback rate of a recorded sample follows it. These tests play notes the way
// a guitar would (a fundamental and its harmonics) and check what the engine
// does with them.

#include "SpampiEngine.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void expect (bool condition, const char* what)
{
    std::printf ("%-62s %s\n", what, condition ? "ok" : "FAILED");

    if (! condition)
        ++failures;
}

constexpr double kRate = 48000.0;
constexpr double kPi = 3.14159265358979323846;

// A plucked string, near enough: fundamental plus the harmonics that make a
// guitar sound unlike a sine wave.
std::vector<float> pluck (double hz, double seconds, float level = 0.35f)
{
    std::vector<float> out ((std::size_t) (seconds * kRate));

    for (std::size_t i = 0; i < out.size(); ++i)
    {
        const auto t = (double) i / kRate;
        const auto envelope = 0.35 + 0.65 * std::exp (-t * 1.5);

        out[i] = level * (float) envelope
                 * (float) (std::sin (2.0 * kPi * hz * t)
                            + 0.45 * std::sin (4.0 * kPi * hz * t)
                            + 0.22 * std::sin (6.0 * kPi * hz * t)
                            + 0.10 * std::sin (8.0 * kPi * hz * t));
    }

    return out;
}

std::vector<float> silence (double seconds)
{
    return std::vector<float> ((std::size_t) (seconds * kRate), 0.0f);
}

spampi::Sample makeSample (double hz)
{
    spampi::Sample sample;
    sample.data = pluck (hz, 1.0, 0.5f);
    sample.baseFrequency = hz;
    sample.name = "test";
    return sample;
}

// Plays a buffer through the engine in host-sized blocks.
std::vector<float> play (spampi::Engine& engine, const std::vector<float>& input)
{
    std::vector<float> output (input.size(), 0.0f);

    constexpr int block = 512;

    for (std::size_t offset = 0; offset < input.size(); offset += block)
    {
        const auto count = (int) std::min ((std::size_t) block, input.size() - offset);
        engine.process (input.data() + offset, output.data() + offset, count);
    }

    return output;
}

spampi::Settings baseSettings()
{
    spampi::Settings s;
    s.root = 57.0f;        // A3, 220 Hz
    s.follow = 1.0f;
    s.interval = 0.0f;
    s.spread = 0.0f;
    s.glide = 0.0f;
    s.drift = 0.0f;
    s.machine = 0.0f;
    s.triggerMode = 0;     // LOOP
    s.sens = 0.55f;
    s.tone = 1.0f;
    s.mix = 1.0f;
    s.level = 0.8f;
    return s;
}

float peakOf (const std::vector<float>& buffer)
{
    auto peak = 0.0f;

    for (const auto value : buffer)
        peak = std::max (peak, std::abs (value));

    return peak;
}
} // namespace

int main()
{
    //--------------------------------------------------------------------------
    // 1. The detector finds the note a guitar would play.
    {
        spampi::PitchDetector detector;
        detector.prepare (kRate);

        const auto input = pluck (220.0, 0.5);

        for (const auto value : input)
            detector.push (value);

        const auto hz = detector.getFrequency();
        const auto midi = spampi::PitchDetector::midiFromFrequency (hz);

        expect (std::abs (hz - 220.0f) < 2.5f, "A3 (220 Hz) is measured as 220 Hz");
        expect (detector.getNote() == 57, "A3 is note 57 (A3)");
        expect (detector.getConfidence() > 0.8f, "the confidence of a clean note is high");
    }

    //--------------------------------------------------------------------------
    // 2. Low and high notes stay inside the range a guitar can reach.
    {
        for (const auto pair : { std::pair<double, int> { 82.41, 40 },      // E2
                                 std::pair<double, int> { 146.83, 50 },     // D3
                                 std::pair<double, int> { 329.63, 64 },     // E4
                                 std::pair<double, int> { 659.26, 76 } })   // E5
        {
            spampi::PitchDetector detector;
            detector.prepare (kRate);

            for (const auto value : pluck (pair.first, 0.4))
                detector.push (value);

            const auto label = std::string (spampi::noteLabel (pair.second))
                               + " is detected";

            expect (detector.getNote() == pair.second, label.c_str());
        }
    }

    //--------------------------------------------------------------------------
    // 3. The playback rate follows the played note.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);
        engine.setSettings (baseSettings());

        play (engine, pluck (220.0, 0.5));
        expect (std::abs (engine.getCurrentRate() - 1.0f) < 0.02f,
                "playing the root note plays the sample at its own speed");

        engine.setSettings (baseSettings());
        play (engine, pluck (329.63, 0.5));
        expect (std::abs (engine.getCurrentRate() - 1.4983f) < 0.03f,
                "playing E4 makes the sample run 1.5x (a fifth up)");

        engine.setSettings (baseSettings());
        play (engine, pluck (110.0, 0.5));
        expect (std::abs (engine.getCurrentRate() - 0.5f) < 0.02f,
                "playing an octave down makes the sample run at half speed");
    }

    //--------------------------------------------------------------------------
    // 4. FOLLOW decides how much of the playing is believed.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);

        auto settings = baseSettings();
        settings.follow = 0.0f;
        settings.interval = 0.0f;
        engine.setSettings (settings);

        play (engine, pluck (329.63, 0.5));
        expect (std::abs (engine.getCurrentRate() - 1.0f) < 0.02f,
                "FOLLOW at zero ignores the playing: the sample stays put");

        settings = baseSettings();
        settings.follow = 1.0f;
        settings.interval = -12.0f;
        engine.setSettings (settings);

        play (engine, pluck (220.0, 0.5));
        expect (std::abs (engine.getCurrentRate() - 0.5f) < 0.02f,
                "INTERVAL -12 detunes the whole thing an octave down");
    }

    //--------------------------------------------------------------------------
    // 5. Silence is silence, and the gate works.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);

        auto settings = baseSettings();
        settings.triggerMode = 1;   // GATE
        engine.setSettings (settings);

        const auto output = play (engine, silence (0.5));

        expect (engine.getDetectedNote() == -1, "silence has no note");
        expect (peakOf (output) < 1.0e-4f, "the gate keeps the machine quiet when nothing is played");
    }

    //--------------------------------------------------------------------------
    // 6. The note events fire when the player changes note.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);
        engine.setSettings (baseSettings());

        play (engine, pluck (220.0, 0.4));
        const auto afterFirst = engine.getNoteEvents();

        play (engine, pluck (329.63, 0.4));

        expect (engine.getNoteEvents() > afterFirst, "changing note raises a new note event");
    }

    //--------------------------------------------------------------------------
    // 7. The output never leaves the rails, however mean the machine is.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);

        auto settings = baseSettings();
        settings.machine = 1.0f;
        settings.level = 2.0f;
        settings.follow = 1.0f;
        engine.setSettings (settings);

        const auto output = play (engine, pluck (196.0, 0.6, 0.5f));

        expect (peakOf (output) <= 1.5f, "the machine is soft clipped: output stays inside 1.5");
        expect (peakOf (output) > 0.05f, "and there is plenty of noise coming out");
    }

    //--------------------------------------------------------------------------
    // 8. LEVEL at zero is a mute, and MIX at zero is the dry guitar.
    {
        spampi::Engine engine;
        engine.prepare (kRate);

        auto sample = makeSample (220.0);
        engine.setSample (&sample);

        auto settings = baseSettings();
        settings.level = 0.0f;
        engine.setSettings (settings);

        expect (peakOf (play (engine, pluck (220.0, 0.3))) < 1.0e-6f, "LEVEL 0 is silent");

        engine.setSettings (baseSettings());

        auto dry = baseSettings();
        dry.mix = 0.0f;
        dry.level = 1.0f;   // LEVEL is a master gain, so it scales the dry signal too
        engine.setSettings (dry);

        const auto input = pluck (220.0, 0.3);
        const auto output = play (engine, input);

        auto difference = 0.0f;

        for (std::size_t i = 0; i < input.size(); ++i)
            difference = std::max (difference, std::abs (input[i] - output[i]));

        expect (difference < 1.0e-5f, "MIX 0 passes the guitar through untouched");
    }

    //--------------------------------------------------------------------------
    // 9. Resampling keeps the length proportional and the pitch unchanged.
    {
        const auto input = pluck (440.0, 1.0);
        const auto output = spampi::resampleMono (input, 48000.0, 44100.0);

        expect (std::abs ((double) output.size() - 44100.0) <= 2.0,
                "48 kHz to 44.1 kHz keeps the length right");

        spampi::PitchDetector detector;
        detector.prepare (44100.0);

        for (const auto value : output)
            detector.push (value);

        expect (std::abs (detector.getFrequency() - 440.0f) < 4.0f,
                "and the resampled note is still A4");
    }

    //--------------------------------------------------------------------------
    // 10. The helpers the panel uses.
    {
        expect (std::string (spampi::noteLabel (60)) == "C4", "note 60 is C4");
        expect (std::string (spampi::noteLabel (-1)) == "--", "no note prints as --");
        expect (std::string (spampi::triggerModeName (2)) == "TRIG", "trigger mode 2 is TRIG");
    }

    if (failures == 0)
        std::printf ("\nall checks passed\n");
    else
        std::printf ("\n%d check(s) failed\n", failures);

    return failures == 0 ? 0 : 1;
}
