// Tests for the MemoryGranulatorMode engine, without JUCE and without a host.
//
// The engine is the part that has to be right: memory must capture and freeze,
// grains must be made from that memory, MIDI must control them independently
// of the recorded audio, monophonic tracking must find a real fundamental,
// feedback must stay bounded, and the output must not depend on block size.
//
//   memorygranulator_dsp_test

#include "MemoryGranulatorEngine.h"

#include <algorithm>
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
constexpr double kTwoPi = 6.28318530717958647692;

struct Result
{
    std::vector<float> left;
    std::vector<float> right;
};

memorygranulator::Settings baseSettings()
{
    memorygranulator::Settings settings;
    settings.memoryLength = 2.0f;
    settings.captureEnable = true;
    settings.freeze = false;
    settings.grainMode = (int) memorygranulator::GrainMode::uniform;
    settings.grainSize = 0.06f;
    settings.density = 18.0f;
    settings.overlap = 1.5f;
    settings.bufferPosition = 0.0f;
    settings.scanSpeed = 0.0f;
    settings.spray = 0.0f;
    settings.reverseProbability = 0.0f;
    settings.grainGain = 0.75f;
    settings.stereoSpread = 0.0f;
    settings.stereoRandom = 0.0f;
    settings.maxGrains = 24;
    settings.pitchSource = (int) memorygranulator::PitchSource::hybrid;
    settings.trackRangeLow = 40.0f;
    settings.trackRangeHigh = 1600.0f;
    settings.confidenceThreshold = 0.45f;
    settings.pitchAttack = 10.0f;
    settings.pitchRelease = 80.0f;
    settings.pitchSmoothing = 10.0f;
    settings.mapTranspose = 0.0f;
    settings.mapSpeed = 0.0f;
    settings.mapPosition = 0.0f;
    settings.mapDensity = 0.0f;
    settings.mapSize = 0.0f;
    settings.mapRate = 0.0f;
    settings.mapHarmonic = 0.0f;
    settings.mapStereo = 0.0f;
    settings.harmonicLayers = 1;
    settings.fundamental = 1.0f;
    settings.octaveMix = 0.0f;
    settings.harmLevel2 = 0.0f;
    settings.harmLevel3 = 0.0f;
    settings.harmLevel4 = 0.0f;
    settings.reverbMix = 0.0f;
    settings.reverbLevel = 0.0f;
    settings.dryWet = 1.0f;              // full wet: the dry signal is removed
    settings.grainLevel = 1.0f;
    settings.reverbFeedback = 0.0f;
    settings.feedbackAmount = 0.0f;
    settings.inputGain = 1.0f;
    settings.outputGain = 1.0f;
    return settings;
}

std::vector<float> sine (double frequency, double seconds, float amplitude = 0.45f,
                         double sampleRate = kSampleRate)
{
    std::vector<float> signal ((std::size_t) (seconds * sampleRate), 0.0f);

    for (std::size_t i = 0; i < signal.size(); ++i)
        signal[i] = amplitude * (float) std::sin (kTwoPi * frequency * (double) i / sampleRate);

    return signal;
}

std::vector<float> silence (double seconds, double sampleRate = kSampleRate)
{
    return std::vector<float> ((std::size_t) (seconds * sampleRate), 0.0f);
}

Result run (memorygranulator::Engine& engine, const std::vector<float>& input,
            double sampleRate = kSampleRate, int blockSize = 64)
{
    Result result;
    result.left.assign (input.size(), 0.0f);
    result.right.assign (input.size(), 0.0f);

    std::vector<float> inL ((std::size_t) blockSize, 0.0f);
    std::vector<float> inR ((std::size_t) blockSize, 0.0f);
    std::vector<float> outL ((std::size_t) blockSize, 0.0f);
    std::vector<float> outR ((std::size_t) blockSize, 0.0f);

    for (std::size_t position = 0; position < input.size();
         position += (std::size_t) blockSize)
    {
        const auto count = (int) std::min ((std::size_t) blockSize, input.size() - position);

        for (int i = 0; i < count; ++i)
        {
            inL[(std::size_t) i] = input[position + (std::size_t) i];
            inR[(std::size_t) i] = input[position + (std::size_t) i];
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
            result.left[position + (std::size_t) i] = outL[(std::size_t) i];
            result.right[position + (std::size_t) i] = outR[(std::size_t) i];
        }
    }

    return result;
}

double peakOf (const std::vector<float>& signal, std::size_t from = 0)
{
    auto peak = 0.0;

    for (auto i = from; i < signal.size(); ++i)
        peak = std::max (peak, (double) std::abs (signal[i]));

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

double difference (const std::vector<float>& a, const std::vector<float>& b)
{
    auto sum = 0.0;
    const auto count = std::min (a.size(), b.size());

    for (std::size_t i = 0; i < count; ++i)
    {
        const auto d = (double) a[i] - (double) b[i];
        sum += d * d;
    }

    return std::sqrt (sum / (double) std::max<std::size_t> (1, count));
}

bool allFinite (const std::vector<float>& signal)
{
    for (auto value : signal)
        if (! std::isfinite (value))
            return false;

    return true;
}
} // namespace

int main()
{
    using namespace memorygranulator;

    std::printf ("MemoryGranulatorMode engine\n\n");

    //--------------------------------------------------------------------------
    std::printf ("silence and finite output:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);
        engine.setSettings (baseSettings());
        engine.reset();

        const auto output = run (engine, silence (0.25));
        check (peakOf (output.left) < 1.0e-6, "silence in stays silence out");
        check (allFinite (output.left) && allFinite (output.right),
               "the output is always a real number");
    }

    //--------------------------------------------------------------------------
    std::printf ("\ncapture, freeze and grains from memory:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);
        auto settings = baseSettings();
        settings.grainMode = (int) GrainMode::uniform;
        settings.grainSize = 0.08f;
        settings.density = 20.0f;
        engine.setSettings (settings);
        engine.reset();

        const auto captured = run (engine, sine (220.0, 0.60));

        settings.captureEnable = false;
        settings.freeze = true;
        engine.setSettings (settings);

        const auto tail = run (engine, silence (0.90));
        const auto tailStart = (std::size_t) (0.05 * kSampleRate);
        const auto tailEnergy = energyOf (tail.left, tailStart, tail.left.size());

        check (peakOf (captured.left) > 0.01, "the captured phrase is audible while it is recorded");
        check (tailEnergy > 1.0e-4,
               "grains keep coming from the frozen memory after the input stops");
        check (peakOf (tail.left) < 1.2, "the frozen tail stays inside the rails");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nreal MIDI control and polyphony:\n");
    {
        auto renderWithNote = [] (int note)
        {
            Engine engine;
            engine.prepare (kSampleRate);
            auto settings = baseSettings();
            settings.pitchSource = (int) PitchSource::midi;
            settings.grainMode = (int) GrainMode::uniform;
            settings.grainSize = 0.05f;
            settings.density = 24.0f;
            settings.harmonicLayers = 1;
            settings.mapTranspose = 0.0f;
            engine.setSettings (settings);
            engine.reset();

            run (engine, sine (220.0, 0.35));

            MidiEvent event;
            event.kind = MidiKind::noteOn;
            event.channel = 0;
            event.note = note;
            event.value = 0.9f;
            engine.handleMidi (event);

            settings.captureEnable = false;
            settings.freeze = true;
            engine.setSettings (settings);

            return run (engine, silence (0.50));
        };

        const auto c4 = renderWithNote (60);
        const auto c5 = renderWithNote (72);
        check (difference (c4.left, c5.left) > 0.005,
               "a MIDI note changes the grains even though the recorded audio is identical");

        Engine poly;
        poly.prepare (kSampleRate);
        auto settings = baseSettings();
        settings.pitchSource = (int) PitchSource::midi;
        poly.setSettings (settings);
        poly.reset();

        for (const auto note : { 60, 64, 67 })
        {
            MidiEvent event;
            event.kind = MidiKind::noteOn;
            event.note = note;
            event.value = 0.8f;
            poly.handleMidi (event);
        }

        run (poly, silence (0.05));
        check (poly.getSnapshot().activeNotes.load() == 3,
               "three simultaneous MIDI notes are three active voices");

        MidiEvent off;
        off.kind = MidiKind::noteOff;
        off.note = 64;
        poly.handleMidi (off);
        run (poly, silence (0.05));
        check (poly.getSnapshot().activeNotes.load() == 2,
               "note off releases one voice without dropping the chord");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nmonophonic audio pitch tracking:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);
        auto settings = baseSettings();
        settings.pitchSource = (int) PitchSource::audio;
        settings.trackRangeLow = 80.0f;
        settings.trackRangeHigh = 800.0f;
        settings.confidenceThreshold = 0.35f;
        settings.grainGain = 0.0f;
        engine.setSettings (settings);
        engine.reset();

        run (engine, sine (220.0, 0.85));

        const auto& snapshot = engine.getSnapshot();
        const auto hz = snapshot.trackedHz.load();
        const auto confidence = snapshot.confidence.load();

        std::printf ("    tracked %.1f Hz, confidence %.3f\n", hz, confidence);
        check (hz > 190.0f && hz < 250.0f, "a 220 Hz tone is tracked as 220 Hz");
        check (confidence > 0.35f, "the tracker reports usable confidence");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nhybrid source and independent pitch/speed:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);
        auto settings = baseSettings();
        settings.pitchSource = (int) PitchSource::hybrid;
        settings.hybridBlend = 0.65f;
        settings.grainGain = 0.2f;
        engine.setSettings (settings);
        engine.reset();

        MidiEvent event;
        event.kind = MidiKind::noteOn;
        event.note = 67;
        event.value = 0.8f;
        engine.handleMidi (event);

        run (engine, sine (220.0, 0.45));
        check (engine.getSnapshot().activeNotes.load() == 1,
               "hybrid mode keeps the MIDI voice");
        check (engine.getSnapshot().trackedHz.load() > 150.0f,
               "hybrid mode also keeps the audio pitch estimate");

        auto moving = baseSettings();
        moving.pitchSource = (int) PitchSource::midi;
        moving.scanSpeed = 1.2f;
        moving.bufferPosition = 0.2f;
        moving.grainGain = 0.75f;

        auto still = moving;
        still.scanSpeed = 0.0f;

        Engine movingEngine;
        movingEngine.prepare (kSampleRate);
        movingEngine.setSettings (moving);
        movingEngine.reset();
        run (movingEngine, sine (220.0, 0.30));

        Engine stillEngine;
        stillEngine.prepare (kSampleRate);
        stillEngine.setSettings (still);
        stillEngine.reset();
        run (stillEngine, sine (220.0, 0.30));

        const auto movingOutput = run (movingEngine, silence (0.35));
        const auto stillOutput = run (stillEngine, silence (0.35));
        check (difference (movingOutput.left, stillOutput.left) > 0.001,
               "playback speed moves the playhead independently of pitch");
    }

    //--------------------------------------------------------------------------
    std::printf ("\ngeneration modes:\n");
    {
        bool everyModeSounds = true;

        for (int mode = 0; mode <= (int) GrainMode::pitchAware; ++mode)
        {
            Engine engine;
            engine.prepare (kSampleRate);
            auto settings = baseSettings();
            settings.grainMode = mode;
            settings.spray = mode == (int) GrainMode::uniform ? 0.0f : 0.25f;
            settings.grainGain = 0.8f;
            engine.setSettings (settings);
            engine.reset();

            run (engine, sine (330.0, 0.35));
            const auto output = run (engine, silence (0.25));

            if (! allFinite (output.left) || peakOf (output.left) < 0.002)
                everyModeSounds = false;
        }

        check (everyModeSounds, "all eight grain-generation modes make finite audible grains");
    }

    //--------------------------------------------------------------------------
    std::printf ("\nfeedback safeguards:\n");
    {
        Engine engine;
        engine.prepare (kSampleRate);
        auto settings = baseSettings();
        settings.feedbackAmount = 0.95f;
        settings.feedbackSource = (int) FeedbackSource::combined;
        settings.feedbackSaturation = 0.8f;
        settings.feedbackDiffusion = 0.7f;
        settings.reverbMix = 0.4f;
        settings.reverbLevel = 0.7f;
        settings.reverbFeedback = 0.7f;
        settings.grainGain = 0.8f;
        engine.setSettings (settings);
        engine.reset();

        const auto output = run (engine, sine (110.0, 6.0));

        check (allFinite (output.left) && allFinite (output.right),
               "a long feedback chain never produces NaN or infinity");
        check (peakOf (output.left) < 1.2 && peakOf (output.right) < 1.2,
               "the bounded output stays below the test rail");
    }

    //--------------------------------------------------------------------------
    std::printf ("\ncommon sample rates and block sizes:\n");
    {
        bool finiteEverywhere = true;

        for (const auto sampleRate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        {
            for (const auto blockSize : { 16, 64, 512, 1024 })
            {
                Engine engine;
                engine.prepare (sampleRate);
                auto settings = baseSettings();
                settings.density = 30.0f;
                settings.feedbackAmount = 0.25f;
                engine.setSettings (settings);
                engine.reset();

                const auto output = run (engine, sine (220.0, 0.12, 0.35f, sampleRate),
                                         sampleRate, blockSize);
                if (! allFinite (output.left) || ! allFinite (output.right))
                    finiteEverywhere = false;
            }
        }

        check (finiteEverywhere,
               "44.1, 48, 88.2, 96 and 192 kHz stay finite with 16..1024 sample blocks");
    }

    std::printf ("\n%s\n", failures == 0 ? "all engine checks passed"
                                        : "one or more engine checks failed");
    return failures == 0 ? 0 : 1;
}
