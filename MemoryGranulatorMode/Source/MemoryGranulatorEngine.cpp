#include "MemoryGranulatorEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace memorygranulator
{
namespace
{
constexpr float kTwoPiF = 6.28318530717958647692f;

float wrap01 (float value) noexcept
{
    value -= std::floor (value);
    return std::clamp (value, 0.0f, 0.999999f);
}

int scaleDegreeFor (int note, int scale, int root) noexcept
{
    static constexpr int chromatic[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    static constexpr int harmonicMinor[] = { 0, 2, 3, 5, 7, 8, 11 };
    static constexpr int pentatonicMinor[] = { 0, 3, 5, 7, 10 };
    static constexpr int pentatonicMajor[] = { 0, 2, 4, 7, 9 };
    static constexpr int dorian[] = { 0, 2, 3, 5, 7, 9, 10 };
    static constexpr int mixolydian[] = { 0, 2, 4, 5, 7, 9, 10 };
    static constexpr int lydian[] = { 0, 2, 4, 6, 7, 9, 11 };
    static constexpr int wholeTone[] = { 0, 2, 4, 6, 8, 10 };
    static constexpr int phrygian[] = { 0, 1, 3, 5, 7, 8, 10 };
    static constexpr int blues[] = { 0, 3, 5, 6, 7, 10 };

    const int* intervals = chromatic;
    int count = 12;

    switch (scale)
    {
        case 1:  intervals = major;           count = 7; break;
        case 2:  intervals = minor;           count = 7; break;
        case 3:  intervals = harmonicMinor;   count = 7; break;
        case 4:  intervals = pentatonicMinor; count = 5; break;
        case 5:  intervals = pentatonicMajor; count = 5; break;
        case 6:  intervals = dorian;          count = 7; break;
        case 7:  intervals = mixolydian;      count = 7; break;
        case 8:  intervals = lydian;          count = 7; break;
        case 9:  intervals = wholeTone;       count = 6; break;
        case 10: intervals = phrygian;        count = 7; break;
        case 11: intervals = blues;           count = 6; break;
        default: break;
    }

    const auto relative = ((note - root) % 12 + 12) % 12;
    const auto octave = (note - root) / 12;
    int best = 0;
    int bestDistance = 100;

    for (int i = 0; i < count; ++i)
    {
        const auto distance = std::abs (intervals[i] - relative);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = intervals[i];
        }
    }

    return root + octave * 12 + best;
}
} // namespace

//==============================================================================
class Engine::PitchTracker
{
public:
    static constexpr int kRing = 512;
    static constexpr int kWindow = 192;
    static constexpr int kDecimate = 8;
    static constexpr int kPeriod = 256;

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (8000.0, newSampleRate);
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

    void setRange (float low, float high) noexcept
    {
        minimumHz = std::clamp (low, 20.0f, 2000.0f);
        maximumHz = std::clamp (high, minimumHz + 20.0f, 5000.0f);
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

    float getFrequency() const noexcept { return f0; }
    float getConfidence() const noexcept { return confidence; }

private:
    void analyse() noexcept
    {
        const auto decimatedRate = sampleRate / (double) kDecimate;
        float window[kWindow];

        for (int i = 0; i < kWindow; ++i)
        {
            const auto index = (writeIndex - kWindow + i + 2 * kRing) % kRing;
            window[i] = ring[index];
        }

        const auto minLag = std::max (2, (int) std::floor (decimatedRate / maximumHz));
        const auto maxLag = std::min (kWindow / 2 - 1,
                                      (int) std::ceil (decimatedRate / minimumHz));

        if (maxLag <= minLag + 2)
            return;

        double totalEnergy = 0.0;

        for (int i = 0; i < kWindow; ++i)
            totalEnergy += (double) window[i] * (double) window[i];

        if (totalEnergy < 1.0e-7)
        {
            confidence *= 0.997f;

            if (confidence < 0.01f)
                f0 = 0.0f;

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

            const auto correlation = (float) (dot / std::sqrt (totalEnergy * energy));

            if (correlation > best)
            {
                best = correlation;
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
        found = std::clamp (found, minimumHz, maximumHz);

        if (f0 > 0.0f && std::abs (found - f0) / f0 < 0.20f)
            f0 = f0 * 0.65f + found * 0.35f;
        else
            f0 = found;

        confidence = confidence * 0.60f + best * 0.40f;
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

    double sampleRate = 48000.0;
    float lowpassState = 0.0f;
    float lowpassCoeff = 0.3f;
    float ring[kRing] {};
    int writeIndex = 0;
    int decimateCount = 0;
    int periodCount = 0;
    float minimumHz = 40.0f;
    float maximumHz = 1600.0f;
    float f0 = 0.0f;
    float confidence = 0.0f;
};

//==============================================================================
class Engine::Reverb
{
public:
    struct Comb
    {
        std::vector<float> buffer;
        int position = 0;
        int delay = 100;
        float feedback = 0.84f;
        float damping = 0.2f;
        float store = 0.0f;

        float process (float input) noexcept
        {
            if (! std::isfinite (input))
                input = 0.0f;

            auto output = buffer[(std::size_t) position];

            if (! std::isfinite (output))
            {
                output = 0.0f;
                buffer[(std::size_t) position] = 0.0f;
            }

            store = output * (1.0f - damping) + store * damping;

            if (! std::isfinite (store))
                store = 0.0f;

            auto next = input + store * feedback;

            if (! std::isfinite (next))
            {
                next = 0.0f;
                store = 0.0f;
            }

            buffer[(std::size_t) position] = next;

            if (++position >= delay)
                position = 0;

            return output;
        }

        void clear() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            position = 0;
            store = 0.0f;
        }
    };

    struct Allpass
    {
        std::vector<float> buffer;
        int position = 0;
        int delay = 300;

        float process (float input) noexcept
        {
            if (! std::isfinite (input))
                input = 0.0f;

            auto buffered = buffer[(std::size_t) position];

            if (! std::isfinite (buffered))
            {
                buffered = 0.0f;
                buffer[(std::size_t) position] = 0.0f;
            }

            const auto output = -input + buffered;
            auto next = input + buffered * 0.5f;

            if (! std::isfinite (output) || ! std::isfinite (next))
            {
                buffer[(std::size_t) position] = 0.0f;

                if (++position >= delay)
                    position = 0;

                return 0.0f;
            }

            buffer[(std::size_t) position] = next;

            if (++position >= delay)
                position = 0;

            return output;
        }

        void clear() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            position = 0;
        }
    };

    void prepare (double sampleRate) noexcept
    {
        this->sampleRate = sampleRate;
        static constexpr int combTunings[8] = { 1116, 1188, 1277, 1356,
                                                 1422, 1491, 1557, 1617 };
        static constexpr int allpassTunings[4] = { 556, 441, 341, 225 };
        const auto scale = sampleRate / 44100.0;
        const auto maxComb = (int) std::ceil (1617.0 * scale * 1.70) + 8;
        const auto maxAllpass = (int) std::ceil (556.0 * scale * 1.25) + 8;

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int i = 0; i < 8; ++i)
            {
                auto& comb = combs[(std::size_t) (channel * 8 + i)];
                comb.buffer.assign ((std::size_t) maxComb, 0.0f);
                comb.delay = std::max (8, (int) (combTunings[i] * scale));
            }

            for (int i = 0; i < 4; ++i)
            {
                auto& allpass = allpasses[(std::size_t) (channel * 4 + i)];
                allpass.buffer.assign ((std::size_t) maxAllpass, 0.0f);
                allpass.delay = std::max (8, (int) (allpassTunings[i] * scale));
            }
        }

        preDelaySize = std::max (8, (int) std::ceil (sampleRate * 0.30));
        preDelayL.assign ((std::size_t) preDelaySize, 0.0f);
        preDelayR.assign ((std::size_t) preDelaySize, 0.0f);
    }

    void reset() noexcept
    {
        for (auto& comb : combs)
            comb.clear();

        for (auto& allpass : allpasses)
            allpass.clear();

        std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
        std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
        feedbackL = feedbackR = 0.0f;
        lfoPhase = 0.0f;
        preDelayPosition = 0;
    }

    void setParameters (float roomSize, float damping, float diffusion,
                        float width, float modulation, float decay,
                        float preDelaySeconds, float feedback, bool freeze) noexcept
    {
        const auto scale = sampleRate / 44100.0;
        const auto lengthScale = 0.55f + std::clamp (roomSize, 0.0f, 1.0f) * 1.10f;
        const auto decayNormalised = std::clamp (
            std::log (std::max (0.10f, decay) / 0.10f) / std::log (120.0f), 0.0f, 1.0f);

        static constexpr int combTunings[8] = { 1116, 1188, 1277, 1356,
                                                 1422, 1491, 1557, 1617 };
        static constexpr int allpassTunings[4] = { 556, 441, 341, 225 };

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int i = 0; i < 8; ++i)
            {
                auto& comb = combs[(std::size_t) (channel * 8 + i)];
                comb.delay = std::clamp ((int) (combTunings[i] * scale * lengthScale),
                                         8, (int) comb.buffer.size() - 1);
                comb.feedback = freeze ? 1.0f : 0.72f + decayNormalised * 0.265f;
                comb.damping = std::clamp (damping * 0.85f + modulation * 0.05f, 0.0f, 0.92f);
            }

            for (int i = 0; i < 4; ++i)
            {
                auto& allpass = allpasses[(std::size_t) (channel * 4 + i)];
                allpass.delay = std::clamp ((int) (allpassTunings[i] * scale
                                                   * (0.82f + diffusion * 0.28f)),
                                            8, (int) allpass.buffer.size() - 1);
            }
        }

        this->diffusion = std::clamp (diffusion, 0.0f, 1.0f);
        this->width = std::clamp (width, 0.0f, 1.0f);
        this->modulation = std::clamp (modulation, 0.0f, 1.0f);
        this->feedbackAmount = std::clamp (feedback, 0.0f, 0.95f);
        this->freeze = freeze;
        preDelaySamples = std::clamp ((int) (preDelaySeconds * sampleRate),
                                      0, preDelaySize - 1);
    }

    void processSample (float inputL, float inputR, float* outputL, float* outputR) noexcept
    {
        if (! std::isfinite (inputL))
            inputL = 0.0f;

        if (! std::isfinite (inputR))
            inputR = 0.0f;

        preDelayL[(std::size_t) preDelayPosition] = inputL;
        preDelayR[(std::size_t) preDelayPosition] = inputR;

        auto delayedL = inputL;
        auto delayedR = inputR;

        if (preDelaySamples > 0)
        {
            const auto readPosition = (preDelayPosition - preDelaySamples + 2 * preDelaySize)
                                      % preDelaySize;
            delayedL = preDelayL[(std::size_t) readPosition];
            delayedR = preDelayR[(std::size_t) readPosition];
        }

        if (++preDelayPosition >= preDelaySize)
            preDelayPosition = 0;

        lfoPhase += (float) (0.07 * kTwoPi / sampleRate);
        if (lfoPhase > kTwoPiF)
            lfoPhase -= kTwoPiF;

        const auto lfo = std::sin (lfoPhase) * modulation * 0.025f;
        constexpr float cloudGain = 0.125f;
        constexpr float feedbackGain = 0.70f;

        const auto input = freeze
                         ? 0.0f
                         : delayedL + delayedR * 0.5f
                           + feedbackL * feedbackAmount * feedbackGain;
        const auto inputRight = freeze
                              ? 0.0f
                              : delayedR + delayedL * 0.5f
                                + feedbackR * feedbackAmount * feedbackGain;

        auto wetL = 0.0f;
        auto wetR = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            wetL += combs[(std::size_t) i].process (input);
            wetR += combs[(std::size_t) (8 + i)].process (inputRight);
        }

        for (int i = 0; i < 4; ++i)
        {
            wetL = allpasses[(std::size_t) i].process (wetL);
            wetR = allpasses[(std::size_t) (4 + i)].process (wetR);
        }

        // Eight parallel combs need explicit gain staging before their sum
        // is returned or fed back into the cloud.
        wetL *= cloudGain;
        wetR *= cloudGain;

        // Early reflections are taken from the same pre-delay ring. They are
        // mixed by DIFFUSION; the late Freeverb cloud remains behind them.
        const auto earlyL = (preDelayL[(std::size_t) ((preDelayPosition + preDelaySize - 37) % preDelaySize)]
                           + preDelayL[(std::size_t) ((preDelayPosition + preDelaySize - 113) % preDelaySize)] * 0.55f)
                          * cloudGain;
        const auto earlyR = (preDelayR[(std::size_t) ((preDelayPosition + preDelaySize - 43) % preDelaySize)]
                           + preDelayR[(std::size_t) ((preDelayPosition + preDelaySize - 127) % preDelaySize)] * 0.55f)
                          * cloudGain;

        wetL = wetL * (1.0f - diffusion * 0.32f) + earlyL * diffusion * 0.32f;
        wetR = wetR * (1.0f - diffusion * 0.32f) + earlyR * diffusion * 0.32f;

        const auto mid = (wetL + wetR) * 0.5f;
        const auto side = (wetL - wetR) * 0.5f * width;
        wetL = mid + side;
        wetR = mid - side;

        if (! std::isfinite (wetL) || ! std::isfinite (wetR))
        {
            wetL = 0.0f;
            wetR = 0.0f;
        }

        feedbackL = wetL;
        feedbackR = wetR;

        *outputL = wetL * (1.0f + lfo);
        *outputR = wetR * (1.0f - lfo);
    }

private:
    std::array<Comb, 16> combs {};
    std::array<Allpass, 8> allpasses {};
    std::vector<float> preDelayL;
    std::vector<float> preDelayR;
    int preDelaySize = 1;
    int preDelaySamples = 0;
    int preDelayPosition = 0;
    double sampleRate = 48000.0;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
    float feedbackAmount = 0.0f;
    float diffusion = 0.65f;
    float width = 0.8f;
    float modulation = 0.35f;
    float lfoPhase = 0.0f;
    bool freeze = false;
};

//==============================================================================
Engine::Engine()
{
    pitchTracker = std::make_unique<PitchTracker>();
    reverb = std::make_unique<Reverb>();
}

Engine::~Engine() = default;

void Engine::Smooth::setup (double sampleRate, double milliseconds) noexcept
{
    const auto safeMilliseconds = std::max (0.01, milliseconds);
    coefficient = (float) (1.0 - std::exp (-1.0 / (0.001 * safeMilliseconds * sampleRate)));
}

float Engine::Smooth::next (float target) noexcept
{
    value += (target - value) * coefficient;
    return value;
}

void Engine::PeakFilter::setParameters (double sampleRate, double frequency,
                                         double resonanceAmount) noexcept
{
    const auto f = std::clamp (frequency, 20.0, sampleRate * 0.45);
    const auto amount = std::clamp (resonanceAmount, 0.0, 1.0);
    const auto omega = 2.0 * kPi * f / sampleRate;
    const auto q = 0.707 + amount * amount * 8.0;
    const auto alpha = std::sin (omega) / (2.0 * q);
    const auto peakGain = std::pow (10.0, amount / 40.0);
    const auto a0 = 1.0 + alpha / peakGain;
    b0 = (1.0 + alpha * peakGain) / a0;
    b1 = (-2.0 * std::cos (omega)) / a0;
    b2 = (1.0 - alpha * peakGain) / a0;
    a1 = (-2.0 * std::cos (omega)) / a0;
    a2 = (1.0 - alpha / peakGain) / a0;
}

float Engine::PeakFilter::process (float x) noexcept
{
    if (! std::isfinite (x))
    {
        reset();
        return 0.0f;
    }

    const auto y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

    if (! std::isfinite (y))
    {
        reset();
        return 0.0f;
    }

    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
}

void Engine::PeakFilter::reset() noexcept
{
    x1 = x2 = y1 = y2 = 0.0f;
}

float Engine::DCBlocker::process (float x) noexcept
{
    const auto y = x - lastIn + 0.9975f * lastOut;
    lastIn = x;
    lastOut = y;
    return y;
}

void Engine::DCBlocker::reset() noexcept
{
    lastIn = lastOut = 0.0f;
}

void Engine::prepare (double newSampleRate)
{
    sampleRate = std::clamp (newSampleRate, 8000.0, 384000.0);
    memoryCapacity = (std::size_t) std::ceil (sampleRate * 30.0);
    memoryLeft.assign (memoryCapacity, 0.0f);
    memoryRight.assign (memoryCapacity, 0.0f);
    feedbackHistorySize = (std::size_t) std::ceil (sampleRate * 0.25);
    feedbackHistoryL.assign (feedbackHistorySize, 0.0f);
    feedbackHistoryR.assign (feedbackHistorySize, 0.0f);
    pitchTracker->prepare (sampleRate);
    reverb->prepare (sampleRate);
    prepared = true;
    reset();
}

void Engine::reset() noexcept
{
    writePosition = 0;
    feedbackWritePosition = 0;
    capturedSamples = 0;
    logicalLength = (std::size_t) std::ceil (sampleRate * settings.memoryLength);
    playhead = 0.0;
    triggerPhase = 0.0;
    tempoPhase = 0.0;
    lfoPhase = 0.0;
    sampleHoldPhase = 0.0;
    randomState = 0.0f;
    sampleHoldValue = 0.0f;
    randomSeed = 0x9e3779b9u;
    memoryLevel = 0.0f;
    velocity = 0.0f;
    aftertouch = 0.0f;
    modWheel = 0.0f;
    pitchWheel = 0.0f;
    pendingRelease = false;
    previousClear = settings.clearTrigger;
    previousRelease = settings.releaseTrigger;
    previousCaptureArmed = settings.captureArmed;
    captureReleased = false;
    trackedHz = 0.0f;
    trackedConfidence = 0.0f;
    smoothedPitchHz = 0.0f;
    smoothedAudioPitchHz = 0.0f;
    pitchPosition = 0.5f;
    controlPitchSemitones = 0.0f;
    inputEnvelope = 0.0f;
    feedbackEnvelope = 0.0f;
    modulationValue = 0.0f;
    modulationValue2 = 0.0f;
    feedbackFilterStateL = feedbackFilterStateR = 0.0f;
    feedbackDampL = feedbackDampR = 0.0f;
    feedbackDcL.reset();
    feedbackDcR.reset();
    spatialFilterL.reset();
    spatialFilterR.reset();
    feedbackPeakFilterL.reset();
    feedbackPeakFilterR.reset();

    for (auto& grain : grains)
        grain = Grain {};

    for (auto& voice : voices)
        voice = MidiVoice {};

    if (! memoryLeft.empty())
        std::fill (memoryLeft.begin(), memoryLeft.end(), 0.0f);

    if (! memoryRight.empty())
        std::fill (memoryRight.begin(), memoryRight.end(), 0.0f);

    if (! feedbackHistoryL.empty())
        std::fill (feedbackHistoryL.begin(), feedbackHistoryL.end(), 0.0f);

    if (! feedbackHistoryR.empty())
        std::fill (feedbackHistoryR.begin(), feedbackHistoryR.end(), 0.0f);

    if (pitchTracker != nullptr)
        pitchTracker->reset();

    if (reverb != nullptr)
        reverb->reset();

    pitchSmoother.snap (0.0f);
    attackSmoother.snap (0.0f);
    releaseSmoother.snap (0.0f);

    for (auto& smoother : mapSmoothers)
        smoother.snap (0.5f);
}

void Engine::handleMidi (const MidiEvent& event) noexcept
{
    if (settings.midiChannel > 0 && event.channel + 1 != settings.midiChannel)
        return;

    switch (event.kind)
    {
        case MidiKind::noteOn:
        {
            if (event.note < (int) settings.noteLow || event.note > (int) settings.noteHigh)
                return;

            for (auto& voice : voices)
            {
                if (voice.active && voice.note == event.note)
                {
                    voice.active = true;
                    voice.released = false;
                    voice.velocity = event.value;
                    voice.age = ++voiceAge;
                    velocity = event.value;
                    return;
                }
            }

            auto* target = &voices[0];

            for (auto& voice : voices)
            {
                if (! voice.active)
                {
                    target = &voice;
                    break;
                }

                if (voice.released && voice.age < target->age)
                    target = &voice;
            }

            target->active = true;
            target->released = false;
            target->note = event.note;
            target->velocity = event.value;
            target->age = ++voiceAge;
            velocity = event.value;
            break;
        }

        case MidiKind::noteOff:
            for (auto& voice : voices)
                if (voice.active && voice.note == event.note)
                {
                    voice.active = false;
                    voice.released = true;
                    voice.age = ++voiceAge;

                    if (velocity == voice.velocity)
                        velocity = 0.0f;
                }
            break;

        case MidiKind::controller:
            if (event.controller == 1)
                modWheel = event.value;
            else if (event.controller == settings.releaseCC)
                pendingRelease = true;
            break;

        case MidiKind::aftertouch:
            aftertouch = event.value;
            break;

        case MidiKind::pitchWheel:
            pitchWheel = event.value;
            break;
    }
}

float Engine::random() noexcept
{
    randomSeed ^= randomSeed << 13;
    randomSeed ^= randomSeed >> 17;
    randomSeed ^= randomSeed << 5;
    return (float) ((randomSeed >> 8) & 0x00ffffffu) / (float) 0x01000000u;
}

float Engine::smoothRandom() noexcept
{
    randomState += (random() * 2.0f - 1.0f - randomState) * 0.0025f;
    return std::clamp (randomState, -1.0f, 1.0f);
}

float Engine::chooseEnvelope (int shape, float phase) const noexcept
{
    phase = std::clamp (phase, 0.0f, 1.0f);

    switch ((Envelope) shape)
    {
        case Envelope::exponential:
            return std::pow (1.0f - phase, 2.2f);

        case Envelope::triangle:
            return 1.0f - std::abs (phase * 2.0f - 1.0f);

        case Envelope::plateau:
        {
            const auto edge = std::min (phase, 1.0f - phase);
            return std::clamp (edge / 0.18f, 0.0f, 1.0f);
        }

        case Envelope::hann:
        default:
            return 0.5f - 0.5f * std::cos (phase * kTwoPiF);
    }
}

float Engine::scaleFrequency (float frequency) const noexcept
{
    if (frequency <= 0.0f || ! settings.quantize)
        return frequency;

    const auto midi = 69.0f + 12.0f * std::log2 (frequency / 440.0f);
    const auto quantized = scaleDegreeFor ((int) std::round (midi), settings.scale,
                                           (int) std::round (settings.rootNote));
    return 440.0f * std::pow (2.0f, ((float) quantized - 69.0f) / 12.0f);
}

float Engine::mapDepth (float depth, float position, float range) const noexcept
{
    auto centred = std::clamp (position, 0.0f, 1.0f) - 0.5f;

    switch ((CurveType) settings.mapCurve)
    {
        case CurveType::exponential:
            centred = std::copysign (std::pow (std::abs (centred), 1.55f), centred);
            break;

        case CurveType::logarithmic:
            centred = std::copysign (std::log1p (std::abs (centred) * 3.0f) / std::log (4.0f), centred);
            break;

        case CurveType::linear:
        default:
            break;
    }

    return depth * centred * range;
}

float Engine::modulationSource (int source) noexcept
{
    if (source < 0)
        return 0.0f;

    switch ((ModSource) source)
    {
        case ModSource::envelope:
            return std::clamp (inputEnvelope * 3.0f, -1.0f, 1.0f);

        case ModSource::input:
            return std::clamp (memoryLevel * 3.0f, -1.0f, 1.0f);

        case ModSource::velocity:
            return velocity * 2.0f - 1.0f;

        case ModSource::aftertouch:
            return aftertouch * 2.0f - 1.0f;

        case ModSource::pitch:
            return pitchPosition * 2.0f - 1.0f;

        case ModSource::random:
            return smoothRandom();

        case ModSource::sampleHold:
            sampleHoldPhase += (float) (settings.modRate / sampleRate);

            if (sampleHoldPhase >= 1.0f)
            {
                sampleHoldPhase -= 1.0f;
                sampleHoldValue = random() * 2.0f - 1.0f;
            }

            return sampleHoldValue;

        case ModSource::tempo:
            return std::sin ((float) tempoPhase * kTwoPiF);

        case ModSource::feedback:
            return std::clamp (feedbackEnvelope * 3.0f, -1.0f, 1.0f);

        case ModSource::lfo:
        default:
            lfoPhase += (float) (settings.modRate * kTwoPi / sampleRate);

            if (lfoPhase > kTwoPi)
                lfoPhase -= kTwoPi;

            return std::sin (lfoPhase);
    }
}

float Engine::modulationDestinationValue (int destination, float modulation) const noexcept
{
    switch ((ModDestination) destination)
    {
        case ModDestination::size:        return 1.0f + modulation * 0.55f;
        case ModDestination::density:     return 1.0f + modulation * 0.80f;
        case ModDestination::position:    return modulation * 0.28f;
        case ModDestination::rate:        return 1.0f + modulation * 0.90f;
        case ModDestination::speed:       return 1.0f + modulation * 0.85f;
        case ModDestination::pitch:       return modulation * 12.0f;
        case ModDestination::spread:      return modulation * 0.55f;
        case ModDestination::reverbDecay:  return 1.0f + modulation * 0.80f;
        case ModDestination::diffusion:   return modulation * 0.35f;
        case ModDestination::feedback:    return modulation * 0.28f;
        case ModDestination::filter:      return std::pow (2.0f, modulation * 2.0f);
        case ModDestination::resonance:   return modulation * 0.60f;
        default:                          return 0.0f;
    }
}

float Engine::modulationContribution (int destination, float lane1, float lane2) const noexcept
{
    auto value = 0.0f;

    if (settings.modDestination == destination)
        value += lane1;

    if (settings.modDestination2 == destination)
        value += lane2;

    return value;
}

int Engine::chooseMidiNote() const noexcept
{
    const MidiVoice* selected = nullptr;

    for (const auto& voice : voices)
    {
        if (! voice.active || voice.released)
            continue;

        if (voice.note < (int) settings.noteLow || voice.note > (int) settings.noteHigh)
            continue;

        if (selected == nullptr)
        {
            selected = &voice;
            continue;
        }

        bool better = false;

        switch ((NotePriority) settings.notePriority)
        {
            case NotePriority::lowest:
                better = voice.note < selected->note;
                break;

            case NotePriority::highest:
                better = voice.note > selected->note;
                break;

            case NotePriority::velocity:
                better = voice.velocity > selected->velocity;
                break;

            case NotePriority::latest:
            default:
                better = voice.age > selected->age;
                break;
        }

        if (better)
            selected = &voice;
    }

    return selected != nullptr ? selected->note : -1;
}

int Engine::chooseHarmonicLayer (float pitchPosition) noexcept
{
    const auto count = std::clamp (settings.harmonicLayers, 1, 4);
    const auto spread = std::clamp (settings.harmonicSpread, 0.0f, 1.0f);
    const auto target = std::clamp (pitchPosition * (float) (count - 1)
                                   + (random() - 0.5f) * spread * 1.8f,
                                   0.0f, (float) (count - 1));

    return std::clamp ((int) std::round (target), 0, count - 1);
}

float Engine::harmonicRatioForLayer (int layer, float pitchPosition) const noexcept
{
    if (layer <= 0)
        return 1.0f;

    if ((FollowMode) settings.followMode == FollowMode::interval)
    {
        if (layer == 1)
            return std::pow (2.0f, settings.intervalA / 12.0f);

        if (layer == 2)
            return std::pow (2.0f, settings.intervalB / 12.0f);
    }

    if (layer == 1)
    {
        const auto octaveBias = 0.65f + settings.octaveMix * 0.55f;
        return std::pow (2.0f, octaveBias);
    }

    if (layer == 2)
        return std::pow (2.0f, settings.intervalA / 12.0f);

    if (layer == 3)
        return std::pow (2.0f, settings.intervalB / 12.0f);

    return 1.0f + pitchPosition * 0.05f;
}

float Engine::harmonicLayerGain (int layer) const noexcept
{
    switch (layer)
    {
        case 0:  return settings.harmLevel1 * settings.fundamental;
        case 1:  return settings.harmLevel2;
        case 2:  return settings.harmLevel3;
        case 3:  return settings.harmLevel4;
        default: return 0.0f;
    }
}

float Engine::controlPitchRatio (float pitchPosition, int* selectedHarmonic, int forcedLayer) noexcept
{
    const auto midiNote = chooseMidiNote();
    const auto hasMidi = midiNote >= 0;
    const auto hasAudio = trackedConfidence >= settings.confidenceThreshold && trackedHz > 20.0f;
    const auto source = (PitchSource) settings.pitchSource;

    float sourceSemitones = 0.0f;

    if (source == PitchSource::midi)
    {
        if (hasMidi)
        {
            const auto sourceNote = settings.quantize
                                  ? scaleDegreeFor (midiNote, settings.scale,
                                                    (int) std::round (settings.rootNote))
                                  : midiNote;
            sourceSemitones = (float) (sourceNote - 60);
        }
    }
    else if (source == PitchSource::audio)
    {
        if (hasAudio)
        {
            const auto frequency = scaleFrequency (trackedHz);
            sourceSemitones = 12.0f * std::log2 (
                std::max (20.0f, frequency) / 261.625565f);
        }
    }
    else
    {
        // Hybrid keeps MIDI as the musical pitch anchor. Audio is allowed to
        // move the secondary mappings below, but it never gets averaged into
        // the note that defines the transposition.
        if (hasMidi)
        {
            const auto sourceNote = settings.quantize
                                  ? scaleDegreeFor (midiNote, settings.scale,
                                                    (int) std::round (settings.rootNote))
                                  : midiNote;
            sourceSemitones = (float) (sourceNote - 60);
        }
        else if (hasAudio)
        {
            const auto frequency = scaleFrequency (trackedHz);
            sourceSemitones = 12.0f * std::log2 (
                std::max (20.0f, frequency) / 261.625565f);
        }
    }

    auto followScale = 1.0f;

    switch ((FollowMode) settings.followMode)
    {
        case FollowMode::harmonic:  followScale = 0.42f; break;
        case FollowMode::interval:  followScale = 0.30f; break;
        case FollowMode::resonant:  followScale = 0.0f; break;
        case FollowMode::loose:     followScale = 0.78f; break;
        case FollowMode::free:      followScale = 0.0f; break;
        case FollowMode::follow:
        default:                    followScale = 1.0f; break;
    }

    auto semitones = sourceSemitones * followScale + settings.transposition;
    semitones += settings.tuning / 100.0f;
    semitones += mapDepth (settings.mapTranspose, pitchPosition, 24.0f);
    semitones += settings.pitchSensitivity * pitchWheel * 2.0f;
    semitones += modulationDestinationValue (
        (int) ModDestination::pitch,
        modulationContribution ((int) ModDestination::pitch,
                                modulationValue, modulationValue2));

    const auto layer = forcedLayer >= 0
                     ? std::clamp (forcedLayer, 0, 3)
                     : chooseHarmonicLayer (pitchPosition);

    if (selectedHarmonic != nullptr)
        *selectedHarmonic = layer;

    const auto harmonicRatio = harmonicRatioForLayer (layer, pitchPosition);
    controlPitchSemitones = semitones;
    return std::pow (2.0f, semitones / 12.0f) * harmonicRatio;
}

float Engine::readMemory (double position, bool rightChannel) const noexcept
{
    if (memoryCapacity == 0 || capturedSamples == 0)
        return 0.0f;

    const auto span = std::min (capturedSamples, logicalLength);

    if (span == 0)
        return 0.0f;

    position -= std::floor (position);
    const auto offset = (std::size_t) (position * (double) span);
    const auto start = (writePosition + memoryCapacity - span) % memoryCapacity;
    const auto index = (start + offset) % memoryCapacity;
    return rightChannel ? memoryRight[index] : memoryLeft[index];
}

void Engine::spawnGrain (float pitchPosition, float mappedStereo, float currentDensity,
                         float currentSize, float currentPosition, float currentSpeed) noexcept
{
    const auto layerCount = std::clamp (settings.harmonicLayers, 1, 4);
    const auto primaryLayer = chooseHarmonicLayer (pitchPosition);
    const auto mode = (GrainMode) settings.grainMode;
    const auto spread = std::clamp (settings.stereoSpread, 0.0f, 1.0f);
    const auto velocityGain = hasMidiForGrain() ? 0.72f + velocity * 0.55f : 0.92f;

    for (int layer = 0; layer < layerCount; ++layer)
    {
        auto* grain = &grains[0];
        auto shortestLife = std::numeric_limits<float>::max();

        for (auto& candidate : grains)
        {
            if (! candidate.active)
            {
                grain = &candidate;
                break;
            }

            const auto remaining = (float) (candidate.duration - candidate.age);

            if (remaining < shortestLife)
            {
                shortestLife = remaining;
                grain = &candidate;
            }
        }

        auto position = currentPosition;

        switch (mode)
        {
            case GrainMode::uniform:
                position += (random() - 0.5f) * 0.025f;
                break;

            case GrainMode::random:
                position = random();
                break;

            case GrainMode::cluster:
            {
                const auto centre = random();
                position = centre + (random() - 0.5f) * (0.04f + settings.spray * 0.18f);
                break;
            }

            case GrainMode::rhythmic:
                position = std::floor (currentPosition * 8.0f) / 8.0f
                         + (random() - 0.5f) * settings.spray * 0.025f;
                break;

            case GrainMode::cloud:
                position = random();
                break;

            case GrainMode::sparse:
                position = random();
                break;

            case GrainMode::scan:
                position = currentPosition;
                break;

            case GrainMode::pitchAware:
                position = wrap01 (pitchPosition * 0.88f + currentPosition * 0.12f);
                break;
        }

        position += (random() - 0.5f) * settings.spray
                  * (mode == GrainMode::random ? 0.10f : 0.42f);
        position = wrap01 (position);

        grain->active = true;
        grain->position = position;
        grain->age = 0.0;
        grain->duration = std::max (16.0, currentSize * sampleRate
                                          * (0.55f + settings.overlap * 0.68f));
        grain->pitchRatio = controlPitchRatio (pitchPosition, &grain->harmonic, layer);

        if ((FollowMode) settings.followMode == FollowMode::loose)
            grain->pitchRatio *= std::pow (2.0f,
                (random() - 0.5f) * settings.mapTranspose * 0.20f);

        grain->pitchRatio *= std::pow (2.0f,
            (random() - 0.5f) * settings.detune / 1200.0f);
        grain->pitchRatio *= std::pow (2.0f,
            (random() - 0.5f) * settings.harmonicSpread * 12.0f / 1200.0f);

        const auto layerPosition = layerCount > 1
                                 ? (float) layer / (float) (layerCount - 1)
                                 : 0.5f;
        const auto harmonicBalance = 1.0f + (pitchPosition - 0.5f)
                                          * (layerPosition - 0.5f) * 1.40f;
        const auto primaryBoost = layer == primaryLayer ? 1.20f : 0.88f;
        grain->gain = settings.grainGain * velocityGain
                    * harmonicLayerGain (layer)
                    * harmonicBalance * primaryBoost;
        grain->envelope = settings.grainEnvelope;
        grain->direction = random() < settings.reverseProbability ? -1 : 1;

        const auto mappedPan = mapDepth (settings.mapStereo, mappedStereo, 1.0f) * spread;
        const auto randomPan = (random() - 0.5f) * settings.stereoRandom * spread;
        const auto modulationPan = modulationDestinationValue (
            (int) ModDestination::spread,
            modulationContribution ((int) ModDestination::spread,
                                    modulationValue, modulationValue2));
        grain->pan = std::clamp (mappedPan + randomPan + modulationPan, -1.0f, 1.0f);
    }

    (void) currentDensity;
    (void) currentSpeed;
}

bool Engine::hasMidiForGrain() const noexcept
{
    return chooseMidiNote() >= 0;
}

bool Engine::shouldCapture() const noexcept
{
    if (settings.freeze)
        return false;

    switch ((MemoryMode) settings.memoryMode)
    {
        case MemoryMode::freeze:
            return false;

        case MemoryMode::captureRelease:
            return settings.captureArmed && ! captureReleased;

        case MemoryMode::overdub:
            return true;

        case MemoryMode::hold:
            return false;

        case MemoryMode::live:
        default:
            return settings.captureEnable;
    }
}

float Engine::effectivePlaybackSpeed() const noexcept
{
    return settings.scanSpeed * settings.hostRate;
}

void Engine::process (const float* const* input, float* const* output,
                      int numChannels, int numSamples) noexcept
{
    if (! prepared || numChannels <= 0 || numSamples <= 0)
        return;

    const auto clearEdge = settings.clearTrigger && ! previousClear;
    const auto releaseEdge = settings.releaseTrigger && ! previousRelease;
    const auto armEdge = settings.captureArmed && ! previousCaptureArmed;

    previousClear = settings.clearTrigger;
    previousRelease = settings.releaseTrigger;
    previousCaptureArmed = settings.captureArmed;

    if (armEdge)
        captureReleased = false;

    if (clearEdge)
    {
        std::fill (memoryLeft.begin(), memoryLeft.end(), 0.0f);
        std::fill (memoryRight.begin(), memoryRight.end(), 0.0f);
        writePosition = 0;
        capturedSamples = 0;
        memoryLevel = 0.0f;

        for (auto& grain : grains)
            grain.active = false;
    }

    if (releaseEdge || pendingRelease)
    {
        captureReleased = true;
        pendingRelease = false;
    }

    logicalLength = std::clamp ((std::size_t) std::ceil (sampleRate * settings.memoryLength),
                                (std::size_t) 1, memoryCapacity);
    pitchTracker->setRange (settings.trackRangeLow, settings.trackRangeHigh);
    const auto pitchResponse = trackedConfidence >= settings.confidenceThreshold
                             ? settings.pitchAttack : settings.pitchRelease;
    pitchSmoother.setup (sampleRate,
                         std::max (0.1, settings.pitchSmoothing + pitchResponse * 0.5));

    for (auto& smoother : mapSmoothers)
        smoother.setup (sampleRate, settings.mapSmoothing);

    const auto midiNote = chooseMidiNote();
    const auto hasMidi = midiNote >= 0;
    float midiPosition = 0.5f;

    if (hasMidi)
    {
        const auto quantized = settings.quantize
            ? scaleDegreeFor (midiNote, settings.scale, (int) std::round (settings.rootNote))
            : midiNote;
        midiPosition = std::clamp ((float) (quantized - settings.noteLow)
                                   / std::max (1.0f, settings.noteHigh - settings.noteLow),
                                   0.0f, 1.0f);
    }

    float audioPosition = 0.5f;

    if (trackedConfidence >= settings.confidenceThreshold && trackedHz > 20.0f)
    {
        const auto low = std::log (std::max (20.0f, settings.trackRangeLow));
        const auto high = std::log (std::max (settings.trackRangeLow + 20.0f,
                                              settings.trackRangeHigh));
        audioPosition = std::clamp ((std::log (std::max (20.0f, trackedHz)) - low)
                                    / std::max (0.01f, high - low), 0.0f, 1.0f);
    }

    pitchPosition = (PitchSource) settings.pitchSource == PitchSource::midi
        ? midiPosition
        : (PitchSource) settings.pitchSource == PitchSource::audio
            ? audioPosition
            : (hasMidi ? std::clamp (midiPosition * 0.78f
                                     + audioPosition * 0.22f * settings.hybridBlend,
                                     0.0f, 1.0f)
                       : audioPosition);

    const auto mappedResonant = mapSmoothers[7].next (pitchPosition);
    const auto decay = std::clamp (
        settings.reverbDecay
        * modulationDestinationValue (
            (int) ModDestination::reverbDecay,
            modulationContribution ((int) ModDestination::reverbDecay,
                                    modulationValue, modulationValue2)),
        0.10f, 14.0f);
    const auto diffusion = std::clamp (
        settings.diffusion
        + modulationDestinationValue (
            (int) ModDestination::diffusion,
            modulationContribution ((int) ModDestination::diffusion,
                                    modulationValue, modulationValue2)),
        0.0f, 1.0f);

    auto filterFrequency = std::clamp (
        settings.filterFreq
        * modulationDestinationValue (
            (int) ModDestination::filter,
            modulationContribution ((int) ModDestination::filter,
                                    modulationValue, modulationValue2)),
        40.0f, (float) (sampleRate * 0.45));

    auto resonance = std::clamp (
        settings.resonance
        + modulationDestinationValue (
            (int) ModDestination::resonance,
            modulationContribution ((int) ModDestination::resonance,
                                    modulationValue, modulationValue2))
        + mapDepth (settings.mapResonant, mappedResonant, 0.45f),
        0.0f, 1.0f);

    if ((FollowMode) settings.followMode == FollowMode::resonant)
    {
        const auto sourceHz = hasMidi
                            ? 440.0f * std::pow (2.0f, ((float) midiNote - 69.0f) / 12.0f)
                            : ((trackedConfidence >= settings.confidenceThreshold
                                && trackedHz > 20.0f) ? trackedHz : 0.0f);

        if (sourceHz > 20.0f)
        {
            const auto resonantHz = sourceHz * std::pow (2.0f, settings.transposition / 12.0f);
            filterFrequency = std::clamp (
                settings.filterFreq * 0.35f + resonantHz * 2.60f,
                40.0f, (float) (sampleRate * 0.45));
            resonance = std::clamp (resonance + 0.32f, 0.0f, 1.0f);
        }
    }

    spatialFilterL.setParameters (sampleRate, filterFrequency, resonance);
    spatialFilterR.setParameters (sampleRate, filterFrequency, resonance);
    feedbackPeakFilterL.setParameters (sampleRate, filterFrequency, resonance);
    feedbackPeakFilterR.setParameters (sampleRate, filterFrequency, resonance);
    reverb->setParameters (settings.roomSize, settings.damping, diffusion, settings.width,
                           settings.spaceModulation, decay, settings.preDelay,
                           settings.reverbFeedback, settings.reverbFreeze);

    auto inputPeak = 0.0f;
    auto grainPeak = 0.0f;
    auto reverbPeak = 0.0f;
    auto outputPeak = 0.0f;
    auto feedbackPeak = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto inputL = input[0] != nullptr ? input[0][sample] : 0.0f;
        const auto inputR = numChannels > 1 && input[1] != nullptr ? input[1][sample] : inputL;
        const auto dryL = inputL * settings.inputGain;
        const auto dryR = inputR * settings.inputGain;

        pitchTracker->push (0.5f * (dryL + dryR));
        trackedHz = pitchSmoother.next (pitchTracker->getFrequency());
        trackedConfidence = pitchTracker->getConfidence();

        const auto inputMagnitude = std::max (std::abs (dryL), std::abs (dryR));
        inputPeak = std::max (inputPeak, inputMagnitude);
        inputEnvelope += (inputMagnitude - inputEnvelope) * (inputMagnitude > inputEnvelope ? 0.08f : 0.0015f);

        modulationValue = modulationSource (settings.modSource) * settings.modDepth;
        modulationValue2 = modulationSource (settings.modSource2) * settings.modDepth2;

        const auto mappedPosition = mapSmoothers[2].next (pitchPosition);
        const auto mappedSpeed = mapSmoothers[1].next (pitchPosition);
        const auto mappedDensity = mapSmoothers[3].next (pitchPosition);
        const auto mappedSize = mapSmoothers[4].next (pitchPosition);
        const auto mappedRate = mapSmoothers[5].next (pitchPosition);
        const auto mappedHarmonic = mapSmoothers[6].next (pitchPosition);
        const auto mappedReverb = mapSmoothers[8].next (pitchPosition);
        const auto mappedStereo = mapSmoothers[9].next (pitchPosition);

        auto currentDensity = settings.density
            * std::max (0.05f, modulationDestinationValue (
                (int) ModDestination::density,
                modulationContribution ((int) ModDestination::density,
                                        modulationValue, modulationValue2)))
            * (1.0f + mapDepth (settings.mapDensity, mappedDensity, 0.80f));
        auto currentSize = settings.grainSize
            * std::max (0.05f, modulationDestinationValue (
                (int) ModDestination::size,
                modulationContribution ((int) ModDestination::size,
                                        modulationValue, modulationValue2)))
            * (1.0f + mapDepth (settings.mapSize, mappedSize, 0.75f));
        auto currentRate = currentDensity
            * std::max (0.05f, modulationDestinationValue (
                (int) ModDestination::rate,
                modulationContribution ((int) ModDestination::rate,
                                        modulationValue, modulationValue2)))
            * (1.0f + mapDepth (settings.mapRate, mappedRate, 0.90f));
        auto currentSpeed = settings.scanSpeed * settings.hostRate
            * std::max (0.05f, modulationDestinationValue (
                (int) ModDestination::speed,
                modulationContribution ((int) ModDestination::speed,
                                        modulationValue, modulationValue2)))
            * (1.0f + mapDepth (settings.mapSpeed, mappedSpeed, 1.20f));
        auto currentPosition = settings.bufferPosition + playhead
            + mapDepth (settings.mapPosition, mappedPosition, 0.42f)
            + modulationDestinationValue (
                (int) ModDestination::position,
                modulationContribution ((int) ModDestination::position,
                                        modulationValue, modulationValue2));

        if ((GrainMode) settings.grainMode == GrainMode::cloud)
            currentRate *= 3.2f;
        else if ((GrainMode) settings.grainMode == GrainMode::sparse)
            currentRate *= 0.24f;

        if (settings.tempoSync && (GrainMode) settings.grainMode == GrainMode::rhythmic)
            currentRate = std::max (0.1f, settings.hostBpm / 30.0f);

        currentDensity = std::clamp (currentDensity, 0.05f, 120.0f);
        currentRate = std::clamp (currentRate, 0.05f, 140.0f);
        currentSize = std::clamp (currentSize, 0.004f, 2.5f);
        currentSpeed = std::clamp (currentSpeed, -4.0f, 4.0f);

        triggerPhase += currentRate / sampleRate;

        while (triggerPhase >= 1.0)
        {
            triggerPhase -= 1.0;
            const auto harmonicPosition = std::clamp (mappedHarmonic
                                                      + mapDepth (settings.mapHarmonic,
                                                                  mappedHarmonic, 0.60f),
                                                      0.0f, 1.0f);
            spawnGrain (harmonicPosition, mappedStereo, currentDensity, currentSize,
                        currentPosition, currentSpeed);
        }

        playhead += currentSpeed / sampleRate;
        playhead -= std::floor (playhead);

        auto granularL = 0.0f;
        auto granularR = 0.0f;

        for (auto& grain : grains)
        {
            if (! grain.active)
                continue;

            const auto phase = (float) std::clamp (grain.age / grain.duration, 0.0, 1.0);
            const auto envelope = chooseEnvelope (grain.envelope, phase);
            const auto sourceL = readMemory (grain.position, false);
            const auto sourceR = readMemory (grain.position, true);
            const auto pan = std::clamp (grain.pan, -1.0f, 1.0f);
            const auto angle = (pan + 1.0f) * 0.25f * (float) kPi;
            const auto gainL = std::cos (angle);
            const auto gainR = std::sin (angle);
            const auto level = grain.gain * envelope;

            granularL += (sourceL * gainL + sourceR * (1.0f - gainL)) * level;
            granularR += (sourceR * gainR + sourceL * (1.0f - gainR)) * level;

            const auto validMemorySamples = (double) std::min (capturedSamples, logicalLength);

            if (validMemorySamples > 0.0)
                grain.position += (double) grain.direction * grain.pitchRatio / validMemorySamples;

            grain.age += 1.0;

            if (grain.age >= grain.duration)
                grain.active = false;
        }

        const auto activeLimit = std::clamp (settings.maxGrains, 1, kMaxGrains);
        auto activeCount = 0;

        for (const auto& grain : grains)
            if (grain.active)
                ++activeCount;

        if (activeCount > activeLimit)
        {
            auto toRemove = activeCount - activeLimit;

            for (auto& grain : grains)
            {
                if (toRemove <= 0)
                    break;

                if (grain.active)
                {
                    grain.active = false;
                    --toRemove;
                }
            }
        }

        const auto granularSend = granularL * settings.reverbMix;
        const auto granularSendR = granularR * settings.reverbMix;
        float reverbL = 0.0f;
        float reverbR = 0.0f;
        reverb->processSample (granularSend, granularSendR, &reverbL, &reverbR);
        reverbL = spatialFilterL.process (reverbL);
        reverbR = spatialFilterR.process (reverbR);

        const auto mappedReverbDepth = mapDepth (settings.mapReverb, mappedReverb, 0.75f);
        reverbL *= 1.0f + mappedReverbDepth;
        reverbR *= 1.0f + mappedReverbDepth;

        const auto mix = std::clamp (settings.dryWet, 0.0f, 1.0f);
        const auto dryWeight = 1.0f - mix;
        const auto wetL = granularL * settings.grainLevel + reverbL * settings.reverbLevel;
        const auto wetR = granularR * settings.grainLevel + reverbR * settings.reverbLevel;
        auto outputL = dryL * dryWeight + wetL * mix;
        auto outputR = dryR * dryWeight + wetR * mix;

        if (! std::isfinite (outputL) || ! std::isfinite (outputR))
        {
            outputL = 0.0f;
            outputR = 0.0f;
            reverb->reset();
            spatialFilterL.reset();
            spatialFilterR.reset();
            feedbackPeakFilterL.reset();
            feedbackPeakFilterR.reset();
        }

        // A bounded soft clipper is the final safety net for long feedback
        // chains. It is deliberately above 0 dBFS so ordinary material still
        // has headroom, but it can never run away into NaN or a huge square.
        outputL = std::tanh (outputL * 1.05f) / 1.05f;
        outputR = std::tanh (outputR * 1.05f) / 1.05f;
        outputL *= settings.outputGain;
        outputR *= settings.outputGain;

        output[0][sample] = outputL;

        if (numChannels > 1)
            output[1][sample] = outputR;

        // ---- write the selected processed signal back into memory ----
        float feedbackL = 0.0f;
        float feedbackR = 0.0f;

        switch ((FeedbackSource) settings.feedbackSource)
        {
            case FeedbackSource::input:
                feedbackL = dryL;
                feedbackR = dryR;
                break;

            case FeedbackSource::reverb:
                feedbackL = reverbL;
                feedbackR = reverbR;
                break;

            case FeedbackSource::combined:
                feedbackL = granularL + reverbL;
                feedbackR = granularR + reverbR;
                break;

            case FeedbackSource::filtered:
                feedbackL = feedbackPeakFilterL.process (granularL);
                feedbackR = feedbackPeakFilterR.process (granularR);
                break;

            case FeedbackSource::granular:
            default:
                feedbackL = granularL;
                feedbackR = granularR;
                break;
        }

        if (! std::isfinite (feedbackL) || ! std::isfinite (feedbackR))
        {
            feedbackL = 0.0f;
            feedbackR = 0.0f;
        }

        const auto feedbackIndex = feedbackHistoryIndex;
        feedbackHistoryL[feedbackIndex] = feedbackL;
        feedbackHistoryR[feedbackIndex] = feedbackR;
        feedbackHistoryIndex = (feedbackHistoryIndex + 1) % feedbackHistorySize;

        const auto pitchDelay = std::clamp ((int) (32.0 + settings.feedbackPitch * 144.0f),
                                           1, (int) feedbackHistorySize - 2);
        const auto readIndex = (feedbackIndex + feedbackHistorySize - (std::size_t) pitchDelay)
                               % feedbackHistorySize;
        feedbackL = feedbackHistoryL[readIndex];
        feedbackR = feedbackHistoryR[readIndex];

        const auto feedbackCoeff = (float) (1.0 - std::exp (-2.0 * kPi
                                                               * std::clamp (settings.feedbackFilter,
                                                                            40.0f, (float) (sampleRate * 0.45))
                                                               / sampleRate));
        feedbackFilterStateL += (feedbackL - feedbackFilterStateL) * feedbackCoeff;
        feedbackFilterStateR += (feedbackR - feedbackFilterStateR) * feedbackCoeff;
        const auto dampCoeff = 1.0f - settings.feedbackDamping * 0.85f;
        feedbackDampL = feedbackDampL * dampCoeff + feedbackFilterStateL * (1.0f - dampCoeff);
        feedbackDampR = feedbackDampR * dampCoeff + feedbackFilterStateR * (1.0f - dampCoeff);

        if (! std::isfinite (feedbackDampL) || ! std::isfinite (feedbackDampR))
        {
            feedbackDampL = 0.0f;
            feedbackDampR = 0.0f;
            feedbackFilterStateL = 0.0f;
            feedbackFilterStateR = 0.0f;
        }

        feedbackL = feedbackDampL;
        feedbackR = feedbackDampR;

        const auto saturation = settings.feedbackSaturation;
        feedbackL = std::tanh (feedbackL * (1.0f + saturation * 3.0f)) / (1.0f + saturation);
        feedbackR = std::tanh (feedbackR * (1.0f + saturation * 3.0f)) / (1.0f + saturation);

        const auto diffusionAmount = settings.feedbackDiffusion;
        const auto diffusedL = feedbackL * (1.0f - diffusionAmount * 0.5f)
                             + feedbackR * diffusionAmount * 0.5f;
        const auto diffusedR = feedbackR * (1.0f - diffusionAmount * 0.5f)
                             + feedbackL * diffusionAmount * 0.5f;
        feedbackL = feedbackDcL.process (diffusedL);
        feedbackR = feedbackDcR.process (diffusedR);

        const auto feedbackModulation = modulationDestinationValue (
            (int) ModDestination::feedback,
            modulationContribution ((int) ModDestination::feedback,
                                    modulationValue, modulationValue2));
        const auto feedbackAmount = std::clamp (
            settings.feedbackAmount + feedbackModulation, 0.0f, 0.95f);
        const auto feedbackWrite = feedbackAmount * 0.32f;
        feedbackL *= feedbackWrite;
        feedbackR *= feedbackWrite;
        feedbackPeak = std::max (feedbackPeak, std::max (std::abs (feedbackL), std::abs (feedbackR)));
        feedbackEnvelope += (std::max (std::abs (feedbackL), std::abs (feedbackR))
                             - feedbackEnvelope) * 0.02f;

        const auto capture = shouldCapture();
        const auto overdub = (MemoryMode) settings.memoryMode == MemoryMode::overdub;
        auto writeL = capture ? dryL * settings.recLevel : 0.0f;
        auto writeR = capture ? dryR * settings.recLevel : 0.0f;

        if (! settings.stereoCapture)
            writeR = writeL;

        if (feedbackWrite > 0.0f)
        {
            writeL += feedbackL;
            writeR += feedbackR;
        }

        if (capture && overdub && writePosition < memoryCapacity)
        {
            const auto oldL = memoryLeft[writePosition];
            const auto oldR = memoryRight[writePosition];
            writeL = oldL * (1.0f - settings.feedbackStability * 0.35f) + writeL;
            writeR = oldR * (1.0f - settings.feedbackStability * 0.35f) + writeR;
        }

        const auto memoryIsFrozen = settings.freeze
                                || (MemoryMode) settings.memoryMode == MemoryMode::freeze;
        const auto writeToMemory = ! memoryIsFrozen && (capture || feedbackWrite > 0.0f);

        if (memoryCapacity > 0 && writeToMemory)
        {
            if (! std::isfinite (writeL))
                writeL = 0.0f;

            if (! std::isfinite (writeR))
                writeR = 0.0f;

            memoryLeft[writePosition] = std::clamp (writeL, -4.0f, 4.0f);
            memoryRight[writePosition] = std::clamp (writeR, -4.0f, 4.0f);
            writePosition = (writePosition + 1) % memoryCapacity;
            capturedSamples = std::min (capturedSamples + 1,
                                        std::min (logicalLength, memoryCapacity));
        }

        grainPeak = std::max (grainPeak, std::max (std::abs (granularL), std::abs (granularR)));
        reverbPeak = std::max (reverbPeak, std::max (std::abs (reverbL), std::abs (reverbR)));
        outputPeak = std::max (outputPeak, std::max (std::abs (outputL), std::abs (outputR)));
        tempoPhase += (float) (settings.hostBpm / (60.0 * sampleRate));
        tempoPhase -= std::floor (tempoPhase);
    }

    memoryLevel = std::max (memoryLevel * 0.92f, inputPeak);
    snapshot.inputLevel.store (inputPeak, std::memory_order_relaxed);
    snapshot.grainLevel.store (grainPeak, std::memory_order_relaxed);
    snapshot.reverbLevel.store (reverbPeak, std::memory_order_relaxed);
    snapshot.outputLevel.store (outputPeak, std::memory_order_relaxed);
    snapshot.feedbackLevel.store (feedbackPeak, std::memory_order_relaxed);
    snapshot.trackedHz.store (trackedHz, std::memory_order_relaxed);
    snapshot.confidence.store (trackedConfidence, std::memory_order_relaxed);
    snapshot.controlPitch.store (controlPitchSemitones, std::memory_order_relaxed);
    snapshot.playhead.store (wrap01 (settings.bufferPosition + playhead), std::memory_order_relaxed);
    snapshot.modulationValue.store (modulationValue, std::memory_order_relaxed);
    snapshot.velocity.store (velocity, std::memory_order_relaxed);
    snapshot.aftertouch.store (aftertouch, std::memory_order_relaxed);
    snapshot.controlSource.store (settings.pitchSource, std::memory_order_relaxed);

    updateSnapshot();
}

void Engine::updateSnapshot() noexcept
{
    auto activeGrains = 0;
    auto activeNotes = 0;

    for (int i = 0; i < kMaxGrains; ++i)
    {
        snapshot.grainPosition[i].store (grains[(std::size_t) i].position,
                                         std::memory_order_relaxed);
        snapshot.grainLife[i].store (grains[(std::size_t) i].active
                                     ? (float) (grains[(std::size_t) i].age
                                                / std::max (1.0, grains[(std::size_t) i].duration))
                                     : 0.0f,
                                     std::memory_order_relaxed);

        if (grains[(std::size_t) i].active)
            ++activeGrains;
    }

    int noteIndex = 0;

    for (const auto& voice : voices)
    {
        if (voice.active && ! voice.released && noteIndex < 8)
        {
            snapshot.midiNotes[noteIndex++].store (voice.note, std::memory_order_relaxed);
            ++activeNotes;
        }
    }

    for (int i = noteIndex; i < 8; ++i)
        snapshot.midiNotes[i].store (-1, std::memory_order_relaxed);

    snapshot.activeGrains.store (activeGrains, std::memory_order_relaxed);
    snapshot.activeNotes.store (activeNotes, std::memory_order_relaxed);

    const auto span = std::min (capturedSamples, logicalLength);

    for (int i = 0; i < kWaveformPoints; ++i)
    {
        if (span == 0)
        {
            snapshot.waveform[i].store (0.0f, std::memory_order_relaxed);
            continue;
        }

        const auto position = (double) i / (double) kWaveformPoints;
        const auto value = 0.5f * (std::abs (readMemory (position, false))
                                  + std::abs (readMemory (position, true)));
        snapshot.waveform[i].store (std::clamp (value, 0.0f, 1.0f),
                                    std::memory_order_relaxed);
    }
}

} // namespace memorygranulator
