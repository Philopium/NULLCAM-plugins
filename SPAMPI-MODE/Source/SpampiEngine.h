#pragma once

// SPAMPI-MODE engine.
//
// No JUCE, no MIDI: the input audio is analysed in real time, its pitch is
// recovered with YIN, and that pitch drives the playback rate of a real
// recorded sample - the same trick a Rainbow Machine pedal does with a MIDI
// note, except the note comes from the guitar itself.
//
// The engine is deliberately framework free so the DSP can be measured in a
// plain console executable.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace spampi
{

//==============================================================================
// A tiny radix-2 FFT. Only what the pitch detector needs: one real transform
// and one inverse transform of a power spectrum.
class FFT
{
public:
    explicit FFT (int size)
        : n (size),
          levels (0),
          cosTable (std::size_t (size / 2)),
          sinTable (std::size_t (size / 2)),
          reverse (std::size_t (size)),
          re (std::size_t (size)),
          im (std::size_t (size))
    {
        levels = 1;

        while ((1 << levels) < n)
            ++levels;

        for (int i = 0; i < n / 2; ++i)
        {
            cosTable[std::size_t (i)] = std::cos (2.0 * 3.14159265358979 * double (i) / double (n));
            sinTable[std::size_t (i)] = std::sin (2.0 * 3.14159265358979 * double (i) / double (n));
        }

        for (int i = 0; i < n; ++i)
        {
            std::size_t value = 0;

            for (int bit = 0; bit < levels; ++bit)
                value |= std::size_t ((i >> bit) & 1) << std::size_t (levels - 1 - bit);

            reverse[std::size_t (i)] = value;
        }
    }

    void transform (std::vector<float>& re, std::vector<float>& im, bool inverse) const noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const auto j = reverse[std::size_t (i)];

            if (j > (std::size_t) i)
            {
                std::swap (re[std::size_t (i)], re[j]);
                std::swap (im[std::size_t (i)], im[j]);
            }
        }

        for (int size = 2; size <= n; size *= 2)
        {
            const auto half = size / 2;
            const auto step = n / size;

            for (int i = 0; i < n; i += size)
            {
                for (int j = 0, k = 0; j < half; ++j, k += step)
                {
                    const auto wr = cosTable[std::size_t (k)];
                    const auto wi = inverse ? sinTable[std::size_t (k)] : -sinTable[std::size_t (k)];

                    const auto a = (std::size_t) (i + j);
                    const auto b = (std::size_t) (i + j + half);

                    const auto tr = re[b] * wr - im[b] * wi;
                    const auto ti = re[b] * wi + im[b] * wr;

                    re[b] = re[a] - tr;
                    im[b] = im[a] - ti;
                    re[a] += tr;
                    im[a] += ti;
                }
            }
        }

        if (inverse)
        {
            const auto scale = 1.0f / float (n);

            for (int i = 0; i < n; ++i)
            {
                re[std::size_t (i)] *= scale;
                im[std::size_t (i)] *= scale;
            }
        }
    }

private:
    int n;
    int levels;
    std::vector<float> cosTable, sinTable;
    std::vector<std::size_t> reverse;
    std::vector<float> re, im;
};

//==============================================================================
// YIN pitch detector.
//
// The difference function is obtained from the autocorrelation, so the whole
// detector costs one FFT pair instead of an O(N*maxLag) loop. That is what
// makes it cheap enough to run every few milliseconds inside a plugin.
class PitchDetector
{
public:
    static constexpr int kAnalysisSize = 2048;    // samples in the window
    static constexpr int kFftSize = 4096;
    static constexpr int kHop = 256;              // how often we look

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        minLag = std::max (2, int (sampleRate / 1600.0));    // 1600 Hz ceiling
        maxLag = std::min (kAnalysisSize - 2, int (sampleRate / 65.0));   // 65 Hz floor

        window.assign (std::size_t (kAnalysisSize), 0.0f);
        diff.assign (std::size_t (maxLag + 2), 0.0f);
        cmnd.assign (std::size_t (maxLag + 2), 0.0f);
        re.assign (std::size_t (kFftSize), 0.0f);
        im.assign (std::size_t (kFftSize), 0.0f);

        writePos = 0;
        filled = 0;
        hopCounter = 0;
        rms = 0.0f;
        lastFrequency = 0.0f;
        confidence = 0.0f;
        note = -1;
        cents = 0;
        noteEvents = 0;
    }

    void reset() noexcept
    {
        std::fill (window.begin(), window.end(), 0.0f);
        writePos = 0;
        filled = 0;
        hopCounter = 0;
        rms = 0.0f;
        lastFrequency = 0.0f;
        confidence = 0.0f;
        note = -1;
        cents = 0;
    }

    // Feed one sample. Returns true when a new analysis was run.
    bool push (float sample) noexcept
    {
        window[std::size_t (writePos)] = sample;
        writePos = (writePos + 1) % kAnalysisSize;

        if (filled < kAnalysisSize)
            ++filled;

        if (++hopCounter < kHop)
            return false;

        hopCounter = 0;

        if (filled < kAnalysisSize)
            return false;

        analyse();
        return true;
    }

    float getFrequency() const noexcept { return lastFrequency; }
    float getConfidence() const noexcept { return confidence; }
    float getRms() const noexcept { return rms; }
    int getNote() const noexcept { return note; }
    int getCents() const noexcept { return cents; }
    int getNoteEvents() const noexcept { return noteEvents; }

    // 69 = A4, middle C is 60.
    static float midiFromFrequency (float hz) noexcept
    {
        if (hz <= 0.0f)
            return 0.0f;

        return 69.0f + 12.0f * std::log2 (hz / 440.0f);
    }

    static float frequencyFromMidi (float midi) noexcept
    {
        return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
    }

private:
    void analyse() noexcept
    {
        // Linearise the ring buffer and measure its level.
        double energy = 0.0;

        for (int i = 0; i < kAnalysisSize; ++i)
        {
            const auto index = (writePos + i) % kAnalysisSize;
            const auto value = window[std::size_t (index)];
            re[std::size_t (i)] = value;
            im[std::size_t (i)] = 0.0f;
            energy += double (value) * double (value);
        }

        rms = std::sqrt (float (energy / double (kAnalysisSize)));

        for (int i = kAnalysisSize; i < kFftSize; ++i)
        {
            re[std::size_t (i)] = 0.0f;
            im[std::size_t (i)] = 0.0f;
        }

        fft.transform (re, im, false);

        for (int i = 0; i < kFftSize; ++i)
            re[std::size_t (i)] = re[std::size_t (i)] * re[std::size_t (i)]
                                + im[std::size_t (i)] * im[std::size_t (i)];

        std::fill (im.begin(), im.end(), 0.0f);
        fft.transform (re, im, true);

        // re[lag] is now the autocorrelation of the window at that lag.
        const auto r0 = re[0];

        if (! (r0 > 1.0e-9f) || rms < 1.0e-5f)
        {
            confidence = 0.0f;
            return;
        }

        // YIN difference function: d(tau) = r(0) + tail(0) - 2 r(tau)
        //
        // The autocorrelation comes from the FFT, so r(tau) already sums over
        // the W - tau overlapping samples and the tail terms cancel out:
        // d(tau) = r(0) + energy - 2 r(tau).
        for (int lag = 0; lag <= maxLag; ++lag)
        {
            const auto value = double (r0) + energy - 2.0 * double (re[std::size_t (lag)]);
            diff[std::size_t (lag)] = float (value > 0.0 ? value : 0.0);
        }

        // Cumulative mean normalised difference.
        double running = 0.0;
        cmnd[0] = 1.0f;

        for (int lag = 1; lag <= maxLag; ++lag)
        {
            running += double (diff[std::size_t (lag)]);
            cmnd[std::size_t (lag)] = running > 1.0e-12
                ? float (double (diff[std::size_t (lag)]) * double (lag) / running)
                : 1.0f;
        }

        // Absolute threshold: take the first dip under it.
        constexpr float threshold = 0.14f;
        int chosen = -1;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            if (cmnd[std::size_t (lag)] < threshold)
            {
                while (lag + 1 <= maxLag && cmnd[std::size_t (lag + 1)] < cmnd[std::size_t (lag)])
                    ++lag;

                chosen = lag;
                break;
            }
        }

        if (chosen < 0)
        {
            // No clear dip: fall back to the global minimum, but only trust it
            // when the signal really looks periodic.
            int best = minLag;
            for (int lag = minLag + 1; lag <= maxLag; ++lag)
                if (cmnd[std::size_t (lag)] < cmnd[std::size_t (best)])
                    best = lag;

            if (cmnd[std::size_t (best)] > 0.55f)
            {
                confidence = 0.0f;
                return;
            }

            chosen = best;
        }

        // Parabolic interpolation around the chosen lag.
        float refined = float (chosen);

        if (chosen > minLag && chosen < maxLag)
        {
            const auto a = cmnd[std::size_t (chosen - 1)];
            const auto b = cmnd[std::size_t (chosen)];
            const auto c = cmnd[std::size_t (chosen + 1)];
            const auto denom = 2.0f * (2.0f * b - a - c);

            if (std::abs (denom) > 1.0e-9f)
                refined += (c - a) / denom;
        }

        if (refined < 1.0f)
            refined = 1.0f;

        const auto hz = float (sr) / refined;
        const auto clarity = 1.0f - std::clamp (cmnd[std::size_t (chosen)], 0.0f, 1.0f);

        if (hz < 60.0f || hz > 1700.0f)
        {
            confidence = 0.0f;
            return;
        }

        // Smooth the result so the sampler does not jump on every little wobble.
        if (lastFrequency <= 0.0f)
            lastFrequency = hz;
        else
            lastFrequency = lastFrequency * 0.55f + hz * 0.45f;

        confidence = std::max (confidence * 0.6f, clarity);

        const auto midi = midiFromFrequency (lastFrequency);
        const auto rounded = int (std::lround (midi));

        if (rounded != note)
        {
            if (note >= 0)
                ++noteEvents;

            note = rounded;
        }

        cents = int (std::lround ((midi - float (rounded)) * 100.0f));
    }

    double sr = 48000.0;
    int minLag = 30;
    int maxLag = 738;
    int writePos = 0;
    int filled = 0;
    int hopCounter = 0;
    int note = -1;
    int cents = 0;
    int noteEvents = 0;

    float rms = 0.0f;
    float lastFrequency = 0.0f;
    float confidence = 0.0f;

    std::vector<float> window;
    std::vector<float> re, im;
    std::vector<float> diff, cmnd;
    FFT fft { kFftSize };
};

//==============================================================================
// Four point cubic (Catmull-Rom) interpolation. Cheap, smooth, and good enough
// to keep a pitched sample from sounding like a chipmunk.
inline float hermiteRead (const std::vector<float>& d, double phase) noexcept
{
    if (d.empty())
        return 0.0f;

    const auto n = (double) d.size();

    if (phase < 0.0)
        phase = 0.0;

    if (phase >= n)
        phase = std::fmod (phase, n);

    const auto base = (long long) std::floor (phase);
    const auto t = float (phase - double (base));

    const auto wrap = [n] (long long k) noexcept
    {
        const auto m = (long long) n;
        auto q = k % m;
        if (q < 0) q += m;
        return (std::size_t) q;
    };

    const auto y0 = d[wrap (base - 1)];
    const auto y1 = d[wrap (base)];
    const auto y2 = d[wrap (base + 1)];
    const auto y3 = d[wrap (base + 2)];

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * t + c2) * t + c1) * t + c0;
}

// The same interpolation without the wrap around: used when a buffer is
// resampled, where the last sample must not bleed back into the first one.
inline float hermiteReadClamped (const std::vector<float>& d, double phase) noexcept
{
    if (d.empty())
        return 0.0f;

    const auto n = (double) d.size();

    if (phase < 0.0)
        phase = 0.0;

    if (phase > n - 1.0)
        phase = n - 1.0;

    const auto base = (long long) std::floor (phase);
    const auto t = float (phase - double (base));

    const auto last = (long long) d.size() - 1;

    const auto clampIndex = [last] (long long index) noexcept
    {
        if (index < 0) return (std::size_t) 0;
        if (index > last) return (std::size_t) last;
        return (std::size_t) index;
    };

    const auto i0 = clampIndex (base - 1);
    const auto i1 = clampIndex (base);
    const auto i2 = clampIndex (base + 1);
    const auto i3 = clampIndex (base + 2);

    const float y0 = d[i0];
    const float y1 = d[i1];
    const float y2 = d[i2];
    const float y3 = d[i3];

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * t + c2) * t + c1) * t + c0;
}

// Resample a mono buffer to another rate.
inline std::vector<float> resampleMono (const std::vector<float>& in, double fromRate,
                                        double toRate) noexcept
{
    if (in.empty() || fromRate <= 0.0 || toRate <= 0.0)
        return {};

    const auto ratio = toRate / fromRate;

    if (std::abs (ratio - 1.0) < 1.0e-6)
        return in;

    const auto outLength = (std::size_t) std::max (1.0, std::floor (double (in.size()) * ratio));
    std::vector<float> out (outLength, 0.0f);

    for (std::size_t i = 0; i < outLength; ++i)
        out[i] = hermiteReadClamped (in, double (i) / ratio);

    return out;
}

//==============================================================================
// A recorded sample. Real audio, loaded from disk, played back at the rate the
// detected pitch asks for.
struct Sample
{
    std::vector<float> data;     // mono, at the host sample rate
    double baseFrequency = 0.0;  // measured pitch of the recording, 0 if unknown
    std::string name;            // only used by the plugin layer
};

//==============================================================================
// How the sample is triggered.
enum class TriggerMode
{
    loop = 0,   // runs forever, the pitch bends it continuously
    gate = 1,   // only sounds while the input is above the gate
    trig = 2    // restarts from the top every time the detected note changes
};

inline const char* triggerModeName (int mode) noexcept
{
    switch (mode)
    {
        case 0:  return "LOOP";
        case 1:  return "GATE";
        case 2:  return "TRIG";
        default: return "LOOP";
    }
}

//==============================================================================
struct Settings
{
    int sampleIndex = 0;
    float root = 48.0f;        // MIDI note the sample was recorded at
    float follow = 1.0f;       // how much the detected pitch bends the sample
    float interval = 0.0f;     // fixed offset, in semitones
    float spread = 0.0f;       // random octave jumps
    float glide = 0.15f;       // portamento
    float drift = 0.0f;        // slow wobble
    float machine = 0.0f;      // the Rainbow Machine crunch
    int triggerMode = 0;
    float sens = 0.55f;        // gate + confidence sensitivity
    float tone = 0.85f;        // output low pass
    float mix = 1.0f;          // dry / wet
    float level = 0.9f;        // output gain
};

//==============================================================================
// What the editor reads, straight from the audio thread's atomics.
struct Snapshot
{
    std::atomic<float> frequency { 0.0f };     // Hz
    std::atomic<float> confidence { 0.0f };   // 0..1
    std::atomic<float> level { 0.0f };         // input level, 0..1
    std::atomic<float> note { -1.0f };        // MIDI note, -1 when unknown
    std::atomic<float> cents { 0.0f };
    std::atomic<float> playbackRate { 1.0f };  // what the sampler is actually doing
    std::atomic<float> octaveJump { 0.0f };   // the random jump currently applied
    std::atomic<int> noteEvents { 0 };
    std::atomic<int> active { 0 };            // 1 while a note is being tracked
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        detector.prepare (sampleRate);
        detector.reset();
        phase = 0.0;
        currentRate = 1.0;
        targetRate = 1.0;
        driftPhase = 0.0;
        randomOctave = 0;
        lastNoteEvents = 0;
        toneState = 0.0f;
        envelope = 0.0f;
        resetSamplePlayback();
    }

    void setSettings (const Settings& s) noexcept
    {
        settings = s;
    }

    const Settings& getSettings() const noexcept { return settings; }

    void setSample (const Sample* sample) noexcept
    {
        currentSample = sample;
        resetSamplePlayback();
    }

    const Sample* getSample() const noexcept { return currentSample; }

    const Snapshot& getSnapshot() const noexcept { return snapshot; }

    // The main hook. The input is analysed, the sample is played at the rate
    // the detected pitch asks for, and the two are mixed.
    void process (const float* input, float* output, int numSamples) noexcept
    {
        if (numSamples <= 0)
            return;

        const auto& s = settings;
        const auto hasSample = currentSample != nullptr && ! currentSample->data.empty();

        // Gate threshold: sensitivity decides how quiet the input can be and
        // still count as a note.
        const auto gateThreshold = 0.004f + (1.0f - std::clamp (s.sens, 0.0f, 1.0f)) * 0.10f;
        const auto confidenceNeeded = 0.25f + (1.0f - std::clamp (s.sens, 0.0f, 1.0f)) * 0.35f;

        const auto glideSeconds = 0.002f + std::clamp (s.glide, 0.0f, 1.0f) * 0.30f;
        const auto glideCoef = glideSeconds > 0.0f
            ? std::exp (-1.0 / (glideSeconds * float (sr)))
            : 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto in = input != nullptr ? input[i] : 0.0f;

            // ---- pitch tracking ------------------------------------------------
            detector.push (in);

            const auto hz = detector.getFrequency();
            const auto confidence = detector.getConfidence();
            const auto rms = detector.getRms();
            const auto loudEnough = rms >= gateThreshold && confidence >= confidenceNeeded;

            // ---- target playback rate -------------------------------------------
            auto target = 1.0f;

            if (loudEnough && hz > 0.0f)
            {
                const auto detectedMidi = PitchDetector::midiFromFrequency (hz);
                const auto followed = s.root + (detectedMidi - s.root) * std::clamp (s.follow, 0.0f, 1.0f);
                target = std::pow (2.0f, (followed - s.root + s.interval) / 12.0f);
            }
            else
            {
                target = std::pow (2.0f, s.interval / 12.0f);
            }

            // The random octave jump is re-rolled on every new note: this is
            // the signature wobble of the machine.
            const auto events = detector.getNoteEvents();

            if (events != lastNoteEvents)
            {
                lastNoteEvents = events;

                if (loudEnough)
                {
                    const auto spreadOctaves = int (std::lround (std::clamp (s.spread, 0.0f, 1.0f) * 3.0f));
                    randomOctave = 0;

                    if (spreadOctaves > 0)
                    {
                        const auto roll = (float) std::rand() / (float) RAND_MAX;
                        randomOctave = int (std::lround ((roll * 2.0f - 1.0f) * float (spreadOctaves)));
                        randomOctave = std::clamp (randomOctave, -spreadOctaves, spreadOctaves);
                    }
                }
            }

            // Slow drift: the machine never sits perfectly still.
            driftPhase += 0.00007f * float (sr) / 48000.0f;

            if (driftPhase > 1.0f)
                driftPhase -= 1.0f;

            const auto driftSemis = std::clamp (s.drift, 0.0f, 1.0f) * 0.6f
                                  * std::sin (driftPhase * 6.2831853f);
            const auto octaveSemis = float (randomOctave) * 12.0f;

            target *= std::pow (2.0f, (driftSemis + octaveSemis) / 12.0f);
            target = std::clamp (target, 0.02f, 24.0f);

            targetRate = target;

            // Portamento towards the new rate.
            currentRate = currentRate + (targetRate - currentRate) * glideCoef;

            if (currentRate < 0.02f)
                currentRate = 0.02f;

            // ---- sample playback ------------------------------------------------
            float wet = 0.0f;

            if (hasSample)
            {
                const auto& data = currentSample->data;
                const auto size = (double) data.size();

                if (phase >= size)
                    phase = 0.0;

                wet = hermiteRead (data, phase);
                phase += currentRate;

                if (phase >= size)
                    phase -= size;

                // Trigger modes.
                if (s.triggerMode == 1)   // GATE
                {
                    const auto target2 = loudEnough ? 1.0f : 0.0f;
                    envelope += (target2 - envelope) * 0.002f;
                }
                else if (s.triggerMode == 2)   // TRIG
                {
                    if (events != lastTriggerEvent)
                    {
                        lastTriggerEvent = events;
                        phase = 0.0;
                    }

                    envelope += (1.0f - envelope) * 0.002f;
                }
                else   // LOOP
                {
                    envelope += (1.0f - envelope) * 0.002f;
                }
            }
            else
            {
                envelope += (0.0f - envelope) * 0.002f;
            }

            // ---- the machine ----------------------------------------------------
            if (s.machine > 0.001f)
            {
                const auto amount = std::clamp (s.machine, 0.0f, 1.0f);
                const auto drive = 1.0f + amount * 7.0f;

                wet = wet * drive;
                wet = wet / (1.0f + std::abs (wet) * 0.6f);      // soft clip
                wet = std::tanh (wet * 1.2f);

                // Bit crush: the resolution collapses as the machine gets meaner.
                const auto bits = std::max (2.0f, 16.0f - amount * 13.0f);
                const auto steps = std::pow (2.0f, bits);
                wet = std::round (wet * steps) / steps;

                // A little grit on top.
                wet += (std::rand() / (float) RAND_MAX - 0.5f) * amount * 0.04f;
            }

            wet *= envelope;

            // ---- output tone ----------------------------------------------------
            if (s.tone < 0.999f)
            {
                const auto cutoff = 0.02f + std::clamp (s.tone, 0.0f, 1.0f) * 0.95f;
                toneState += cutoff * (wet - toneState);
                wet = toneState;
            }
            else
            {
                toneState = wet;
            }

            const auto dry = in;
            const auto mixAmount = std::clamp (s.mix, 0.0f, 1.0f);
            const auto blended = dry * (1.0f - mixAmount) + wet * mixAmount;
            const auto out = std::clamp (blended * std::clamp (s.level, 0.0f, 2.0f), -1.5f, 1.5f);

            if (output != nullptr)
                output[i] = out;
        }

        // ---- publish the state for the panel ------------------------------------
        snapshot.frequency.store (detector.getFrequency(), std::memory_order_relaxed);
        snapshot.confidence.store (detector.getConfidence(), std::memory_order_relaxed);
        snapshot.level.store (std::clamp (detector.getRms() * 6.0f, 0.0f, 1.0f), std::memory_order_relaxed);
        snapshot.note.store (float (detector.getNote()), std::memory_order_relaxed);
        snapshot.cents.store (float (detector.getCents()), std::memory_order_relaxed);
        snapshot.playbackRate.store (currentRate, std::memory_order_relaxed);
        snapshot.octaveJump.store (float (randomOctave), std::memory_order_relaxed);
        snapshot.noteEvents.store (detector.getNoteEvents(), std::memory_order_relaxed);
        snapshot.active.store (loudEnough ? 1 : 0, std::memory_order_relaxed);
    }

    // Used by the tests: the pitch the detector currently believes in.
    float getDetectedFrequency() const noexcept { return detector.getFrequency(); }
    int getDetectedNote() const noexcept { return detector.getNote(); }
    float getConfidence() const noexcept { return detector.getConfidence(); }
    float getCurrentRate() const noexcept { return currentRate; }
    int getNoteEvents() const noexcept { return detector.getNoteEvents(); }

private:
    void resetSamplePlayback() noexcept
    {
        phase = 0.0;
        envelope = 0.0f;
        lastTriggerEvent = -1;
    }

    double sr = 48000.0;
    double phase = 0.0;
    float currentRate = 1.0f;
    float targetRate = 1.0f;
    float driftPhase = 0.0f;
    float toneState = 0.0f;
    float envelope = 0.0f;
    int randomOctave = 0;
    int lastNoteEvents = 0;
    int lastTriggerEvent = -1;

    Settings settings;
    const Sample* currentSample = nullptr;
    PitchDetector detector;
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

} // namespace spampi
