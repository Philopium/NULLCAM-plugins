// Tests for the EchoMatrixMode engine, without JUCE and without a host.
//
// The engine is the part that has to be right: the delay has to repeat at the
// right time, the pattern has to decide which repeats happen, a new repeat
// must never disturb the ones already ringing, and nothing may ever blow up.
//
//   echomatrix_dsp_test

#include "EchoMatrixEngine.h"

#include <cmath>
#include <cstdio>
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

// Runs the engine over a mono input and returns the left and right outputs.
struct Result
{
    std::vector<float> left;
    std::vector<float> right;
};

Result run (echomatrix::Engine& engine, const std::vector<float>& input,
            double sampleRate = kSampleRate, int blockSize = 64)
{
    Result result;
    result.left.assign (input.size(), 0.0f);
    result.right.assign (input.size(), 0.0f);

    std::vector<float> inL (blockSize, 0.0f);
    std::vector<float> inR (blockSize, 0.0f);
    std::vector<float> outL (blockSize, 0.0f);
    std::vector<float> outR (blockSize, 0.0f);

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

echomatrix::Settings baseSettings()
{
    echomatrix::Settings s;
    s.delayTime = 0.25f;
    s.feedback = 0.0f;
    s.mix = 1.0f;
    s.inputGain = 1.0f;
    s.outputGain = 1.0f;
    s.hpfCutoff = 20.0f;
    s.lpfCutoff = 20000.0f;
    s.drive = 0.0f;

    // Only the first step is on: one repeat every delay time.
    s.steps[0].enabled = true;
    s.steps[0].level = 1.0f;
    s.steps[0].pan = 0.0f;

    return s;
}

std::vector<float> click (int length, int position = 0)
{
    std::vector<float> buffer ((std::size_t) length, 0.0f);

    if (position >= 0 && position < length)
        buffer[(std::size_t) position] = 1.0f;

    return buffer;
}

int firstAudibleSample (const std::vector<float>& buffer, float threshold = 1.0e-5f)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
        if (std::abs (buffer[i]) > threshold)
            return (int) i;

    return -1;
}

double energy (const std::vector<float>& buffer, int from, int to)
{
    double sum = 0.0;

    for (int i = std::max (0, from); i < std::min ((int) buffer.size(), to); ++i)
        sum += (double) buffer[(std::size_t) i] * (double) buffer[(std::size_t) i];

    return sum;
}

} // namespace

int main()
{
    std::printf ("EchoMatrixMode engine tests\n\n");

    //--------------------------------------------------------------------------
    // A click is repeated after the delay time.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 1.0));
        const auto output = run (engine, input);

        const auto expected = (int) (0.25 * kSampleRate);
        const auto found = firstAudibleSample (output.left);

        std::printf ("the repeat starts at sample %d, expected around %d\n", found, expected);
        check (found > 0, "the click is repeated");
        check (found >= expected - 64 && found <= expected + 64,
               "the repeat lands exactly one delay time later");
    }

    //--------------------------------------------------------------------------
    // The sequencer moves one step at a time.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());

        auto settings = baseSettings();

        for (int i = 0; i < 16; ++i)
            settings.steps[i].enabled = true;

        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 2.0));
        run (engine, input);

        const auto step = engine.getSnapshot().currentStep.load();

        // Two seconds is eight delay periods, so the pattern has gone round.
        check (step >= 0 && step < 16, "the playhead stays inside the pattern");
    }

    //--------------------------------------------------------------------------
    // The pattern decides which repeats happen.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();

        for (int i = 0; i < 16; ++i)
            settings.steps[i].enabled = false;

        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 1.0));
        const auto silent = run (engine, input);

        check (firstAudibleSample (silent.left) < 0,
               "an empty pattern repeats nothing");

        engine.reset();
        engine.setSettings (baseSettings());

        const auto loud = run (engine, input);
        check (firstAudibleSample (loud.left) > 0,
               "the first step brings the repeat back");
    }

    //--------------------------------------------------------------------------
    // A new repeat never disturbs the ones already ringing.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto alone = run (engine, input);

        engine.reset();

        auto settings = baseSettings();
        settings.steps[8].enabled = true;
        settings.steps[8].level = 0.8f;
        settings.steps[8].pan = -0.7f;
        engine.setSettings (settings);

        const auto together = run (engine, input);

        // The second step fires at 8 sixteenths, that is half a delay period
        // later, and its repeat opens one whole delay period after that: so
        // everything before 1.5 delay periods must be untouched.
        const auto boundary = (int) (1.4 * 0.25 * kSampleRate);
        double difference = 0.0;

        for (int i = 0; i < boundary; ++i)
            difference += std::abs ((double) alone.left[(std::size_t) i]
                                    - (double) together.left[(std::size_t) i]);

        check (difference < 1.0e-6,
               "a new repeat does not alter the repeats already running");
    }

    //--------------------------------------------------------------------------
    // A step panned left goes left.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.steps[0].pan = -1.0f;
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 1.0));
        const auto output = run (engine, input);

        const auto from = (int) (0.25 * kSampleRate);
        const auto leftEnergy = energy (output.left, from, from + 2000);
        const auto rightEnergy = energy (output.right, from, from + 2000);

        check (leftEnergy > rightEnergy * 10.0, "a step panned left stays on the left");
    }

    //--------------------------------------------------------------------------
    // Feedback keeps the tail alive.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto dry = baseSettings();
        dry.feedback = 0.0f;
        engine.setSettings (dry);

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto without = run (engine, input);

        engine.reset();

        auto wet = baseSettings();
        wet.feedback = 0.85f;
        engine.setSettings (wet);

        const auto with = run (engine, input);

        // The feedback copies land in the memory and are picked up by the
        // grains fired later, so the tail after the first repeat grows.
        const auto tailFrom = (int) (0.5 * kSampleRate);
        const auto tailWithout = energy (without.left, tailFrom, (int) without.left.size());
        const auto tailWith = energy (with.left, tailFrom, (int) with.left.size());

        check (tailWith > tailWithout * 5.0,
               "the feedback keeps the repeats ringing");

        // The first step fires again after one full pattern, and that grain
        // now carries the echoes of the feedback loop.
        const auto from = (int) (1.25 * kSampleRate);
        const auto to = (int) (1.40 * kSampleRate);

        check (energy (with.left, from, to) > energy (without.left, from, to) * 5.0,
               "the feedback echoes come back on the next grain");
    }

    //--------------------------------------------------------------------------
    // Mix at zero is a clean pass-through.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.mix = 0.0f;
        engine.setSettings (settings);

        std::vector<float> input ((int) (kSampleRate * 0.2), 0.0f);

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = (float) std::sin ((double) i * 0.05);

        const auto output = run (engine, input);

        double difference = 0.0;

        for (std::size_t i = 0; i < input.size(); ++i)
            difference += std::abs ((double) input[i] - (double) output.left[i]);

        check (difference < 1.0e-6, "mix at zero passes the input through untouched");
    }

    //--------------------------------------------------------------------------
    // A step with the level at zero says nothing.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.steps[0].level = 0.0f;
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 1.0));
        const auto output = run (engine, input);

        check (firstAudibleSample (output.left) < 0, "a step at level zero is mute");
    }

    //--------------------------------------------------------------------------
    // Silence in, silence out.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());

        const auto input = std::vector<float> ((int) (kSampleRate * 1.0), 0.0f);
        const auto output = run (engine, input);

        double peak = 0.0;

        for (auto value : output.left)
            peak = std::max (peak, (double) std::abs (value));

        check (peak < 1.0e-9, "silence stays silence");
    }

    //--------------------------------------------------------------------------
    // Tempo sync follows the host.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.syncEnabled = true;
        settings.delayTime = 0.25f;   // ignored while the sync is on
        engine.setSettings (settings);
        engine.setBpm (120.0);

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto output = run (engine, input);

        const auto expected = (int) (0.5 * kSampleRate);   // 60 / 120
        const auto found = firstAudibleSample (output.left);

        std::printf ("the synced repeat starts at sample %d, expected around %d\n",
                     found, expected);
        check (found >= expected - 64 && found <= expected + 64,
               "at 120 BPM the repeat waits half a second");
    }

    //--------------------------------------------------------------------------
    // Every step taps a different moment of the delay window: the pattern is
    // a matrix of taps, not one delay line with a gate on it.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.steps[0].enabled = false;
        settings.steps[8].enabled = true;
        settings.steps[8].level = 1.0f;
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto output = run (engine, input);

        // Step 9 is the middle of the window: it repeats 1.5 delay times later.
        const auto expected = (int) (1.5 * 0.25 * kSampleRate);
        const auto found = firstAudibleSample (output.left);

        std::printf ("the eighth step repeats at sample %d, expected around %d\n",
                     found, expected);
        check (found >= expected - 64 && found <= expected + 64,
               "every step touches its own moment of the delay window");

        // And the first step, one delay time later, is still untouched.
        const auto firstExpected = (int) (0.25 * kSampleRate);
        const auto firstFound = firstAudibleSample (output.left, 1.0e-5f);

        check (firstFound == found,
               "a step on its own repeats only at its own moment");
        check (firstExpected > 0, "the delay window starts at one delay time");
    }

    //--------------------------------------------------------------------------
    // Two steps in the pattern make two different repeats.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.steps[0].enabled = true;
        settings.steps[4].enabled = true;
        settings.steps[4].level = 1.0f;
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto output = run (engine, input);

        const auto firstTap = (int) (1.0 * 0.25 * kSampleRate);
        const auto secondTap = (int) (1.25 * 0.25 * kSampleRate);

        const auto firstEnergy = energy (output.left, firstTap, firstTap + 200);
        const auto secondEnergy = energy (output.left, secondTap, secondTap + 200);

        std::printf ("the two taps carry %.4f and %.4f of energy\n",
                     firstEnergy, secondEnergy);

        check (firstEnergy > 0.0, "the first step makes its own repeat");
        check (secondEnergy > 0.0, "the fifth step makes another repeat");
    }

    //--------------------------------------------------------------------------
    // The whole grid makes sixteen different repeats, one per step.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();

        for (int i = 0; i < 16; ++i)
        {
            settings.steps[i].enabled = true;
            settings.steps[i].level = 1.0f;
        }

        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 2.0));
        const auto output = run (engine, input);

        const auto window = (int) (0.25 * kSampleRate / 16);
        int tapsFound = 0;

        for (int step = 0; step < 16; ++step)
        {
            const auto at = (int) (0.25 * kSampleRate) + step * window;

            if (energy (output.left, at - 4, at + 4) > 0.0)
                ++tapsFound;
        }

        std::printf ("the grid makes %d different repeats\n", tapsFound);
        check (tapsFound == 16, "the sixteen steps make sixteen different repeats");
    }

    //--------------------------------------------------------------------------
    // The hand over from one repeat to the next is seamless: no click.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());

        std::vector<float> input ((int) (kSampleRate * 1.0), 0.0f);

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = 0.5f * (float) std::sin ((double) i * 0.03);

        const auto output = run (engine, input);

        // The first repeat of the pattern takes over at one delay time.
        const auto boundary = (int) (0.25 * kSampleRate);
        double biggestJump = 0.0;

        for (int i = boundary - 32; i < boundary + 32; ++i)
            biggestJump = std::max (biggestJump,
                                    std::abs ((double) output.left[(std::size_t) i]
                                              - (double) output.left[(std::size_t) (i - 1)]));

        std::printf ("the biggest jump at the hand over is %.5f\n", biggestJump);
        check (biggestJump < 0.05, "the hand over from one repeat to the next is seamless");
    }

    //--------------------------------------------------------------------------
    // A step that is switched off fades away instead of stopping dead.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());

        std::vector<float> input ((int) (kSampleRate * 2.0), 0.0f);

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = 0.5f * (float) std::sin ((double) i * 0.03);

        // The first part plays with the step on.
        const auto split = (int) (0.8 * kSampleRate);
        run (engine, std::vector<float> (input.begin(), input.begin() + split));

        // Then the step is switched off while the repeat is still ringing.
        auto settings = baseSettings();
        settings.steps[0].enabled = false;
        engine.setSettings (settings);

        const auto second = run (engine,
                                 std::vector<float> (input.begin() + split,
                                                     input.end()));

        double biggestJump = 0.0;

        for (std::size_t i = 1; i < second.left.size(); ++i)
            biggestJump = std::max (biggestJump,
                                    std::abs ((double) second.left[i] - (double) second.left[i - 1]));

        // The repeat that was already ringing keeps going, and then fades.
        const auto stillRinging = energy (second.left, 0, (int) (0.05 * kSampleRate));

        // After the life of that repeat is over, nothing is left.
        const auto tail = energy (second.left,
                                  (int) (second.left.size() - 0.05 * kSampleRate),
                                  (int) second.left.size());

        std::printf ("the biggest jump after the switch off is %.5f, the tail carries %.6f\n",
                     biggestJump, tail);

        check (biggestJump < 0.1, "a step that is switched off does not click");
        check (stillRinging > 0.0, "the repeat that was ringing keeps going for a while");
        check (tail < 1.0e-6, "a step that is switched off fades away completely");
    }

    //--------------------------------------------------------------------------
    // The result does not depend on the size of the audio block: the host
    // can call the plugin with 1 sample or with 512 and hear the same thing.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.steps[4].enabled = true;
        settings.steps[8].enabled = true;
        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 1.0));

        const auto reference = run (engine, input, kSampleRate, 512);
        double worst = 0.0;

        for (const auto blockSize : { 1, 7, 16, 32, 64, 128, 333 })
        {
            echomatrix::Engine other;
            other.prepare (kSampleRate);
            other.setSettings (settings);

            const auto output = run (other, input, kSampleRate, blockSize);

            for (std::size_t i = 0; i < reference.left.size(); ++i)
                worst = std::max (worst,
                                  std::abs ((double) reference.left[i]
                                            - (double) output.left[i]));
        }

        std::printf ("the biggest difference between the block sizes is %.9f\n", worst);
        check (worst < 1.0e-6, "the sound does not depend on the size of the audio block");
    }

    //--------------------------------------------------------------------------
    // The common sample rates all work, and the repeat lands where it should.
    {
        for (const auto sampleRate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        {
            echomatrix::Engine engine;
            engine.prepare (sampleRate);
            engine.setSettings (baseSettings());

            const auto input = click ((int) (sampleRate * 1.0));
            const auto output = run (engine, input, sampleRate, 64);

            const auto expected = (int) (0.25 * sampleRate);
            const auto found = firstAudibleSample (output.left);

            std::printf ("at %.0f Hz the repeat lands at sample %d, expected %d\n",
                         sampleRate, found, expected);

            check (found >= expected - 64 && found <= expected + 64,
                   "the repeat lands right at every common sample rate");
        }
    }

    //--------------------------------------------------------------------------
    // The pool is bounded and the audio never explodes.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.feedback = 0.97f;
        settings.drive = 1.0f;
        settings.delayTime = 0.05f;

        for (int i = 0; i < 16; ++i)
        {
            settings.steps[i].enabled = true;
            settings.steps[i].level = 1.0f;
            settings.steps[i].pan = (float) (i % 5 - 2) * 0.4f;
        }

        engine.setSettings (settings);

        const auto input = click ((int) (kSampleRate * 5.0));
        const auto output = run (engine, input);

        double peak = 0.0;
        bool finite = true;

        for (auto value : output.left)
        {
            if (! std::isfinite (value))
                finite = false;

            peak = std::max (peak, (double) std::abs (value));
        }

        check (finite, "the output never turns into NaN or infinity");
        check (peak < 8.0, "a crazy pattern stays inside a sane range");
        check (engine.getActiveVoiceCount() <= echomatrix::Engine::kMaxEvents,
               "the voice pool never grows past its limit");
    }

    //--------------------------------------------------------------------------
    // The wet meter on the panel falls back down instead of sticking on the
    // loudest moment ever heard.
    {
        echomatrix::Engine engine;
        engine.prepare (kSampleRate);

        auto settings = baseSettings();
        settings.feedback = 0.8f;
        engine.setSettings (settings);

        // A second of loud noise, then two seconds of silence.
        std::vector<float> noise ((int) (kSampleRate * 1.0), 0.0f);

        for (int i = 0; i < (int) (kSampleRate * 1.0); ++i)
            noise[(std::size_t) i] = 0.8f * (float) std::sin ((double) i * 0.2);

        run (engine, noise);

        const auto loud = engine.getSnapshot().wetLevel.load();

        const std::vector<float> silence ((int) (kSampleRate * 2.0), 0.0f);
        run (engine, silence);

        const auto quiet = engine.getSnapshot().wetLevel.load();

        std::printf ("the wet meter reads %.3f during the noise, %.3f after it stops\n",
                     loud, quiet);

        check (loud > 0.0f, "the wet meter sees the repeats");
        check (quiet < loud, "the wet meter comes back down when the input stops");
    }

    std::printf ("\n%s\n", failures == 0 ? "all checks passed"
                                         : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
