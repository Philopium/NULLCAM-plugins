// NULLCAM CHAIN — the audio damage of the app, as plain C++.
// No framework here on purpose: this file is the sound, and it can be compiled
// on its own to be measured (see ../tests/dsp_test.cpp).
//
// Signal flow, copied from renderAudioOffline() in src/main.js:
//
//   input ──┬──────────────────────────────────────────────► dry ──┐
//           └─► highpass ─► lowpass ─► bitcrush ─┬─► delay+feedback ─► wet ─┤
//                                               └─► ring modulator ────────┴─► trim ─► out
//                                                                    (sample & hold last)

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace nullcam
{
constexpr float kPi = 3.14159265358979323846f;

// Transposed direct form II biquad, the same shapes the app builds with
// createBiquadFilter (Q 0.7, Butterworth-ish).
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() { z1 = z2 = 0.0f; }

    void setHighpass (float sampleRate, float hz, float q)
    {
        const float w = 2.0f * kPi * std::min (hz, sampleRate * 0.49f) / sampleRate;
        const float cw = std::cos (w), sw = std::sin (w);
        const float alpha = sw / (2.0f * q);
        const float a0 = 1.0f + alpha;
        b0 = (1.0f + cw) * 0.5f / a0;
        b1 = -(1.0f + cw) / a0;
        b2 = b0;
        a1 = -2.0f * cw / a0;
        a2 = (1.0f - alpha) / a0;
    }

    void setLowpass (float sampleRate, float hz, float q)
    {
        const float w = 2.0f * kPi * std::min (hz, sampleRate * 0.49f) / sampleRate;
        const float cw = std::cos (w), sw = std::sin (w);
        const float alpha = sw / (2.0f * q);
        const float a0 = 1.0f + alpha;
        b0 = (1.0f - cw) * 0.5f / a0;
        b1 = (1.0f - cw) / a0;
        b2 = b0;
        a1 = -2.0f * cw / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

struct Settings
{
    float hpfHz = 330.0f;      // 70 .. 1200
    float lpfHz = 16000.0f;    // 350 .. 16000
    float crush = 0.0f;        // 0 .. 1   (0 = untouched, 1 = 2 bit + heavy hold)
    float delayMs = 344.0f;    // 80 .. 1600
    float echo = 1.0f;         // 0 .. 1   amount of the echo branch
    float feedback = -1.0f;    // < 0 = derived from echo, like the app does
    float ringHz = 117.0f;     // 38 .. 400
    float ring = 1.0f;         // 0 .. 1
    float dry = 1.0f;          // 0 .. 1
    float outputTrim = -1.0f;  // < 0 = derived, so a clean take stays at unity
};

class Chain
{
public:
    // The app walks this ladder by amount: 10 bits down to 2.
    static constexpr float kBitLadder[7] = { 10.0f, 8.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        hp.reset();
        lp.reset();
        delayLine.assign ((std::size_t) (sr * 1.7) + 4, 0.0f);
        writePos = 0;
        phase = 0.0f;
        holdCount = 1;
        holdLeft = 0;
        held = 0.0f;
        setSettings (settings);
    }

    void setSettings (const Settings& s)
    {
        settings = s;

        hp.setHighpass (sr, std::max (20.0f, s.hpfHz), 0.7f);
        lp.setLowpass (sr, std::max (200.0f, s.lpfHz), 0.7f);

        const int idx = std::clamp ((int) std::floor (s.crush * 6.0f), 0, 6);
        bits = kBitLadder[idx];
        steps = std::pow (2.0f, bits);

        // sample and hold only kicks in once the crush is heavy, exactly like the app
        holdCount = s.crush > 0.3f ? 1 + (int) std::lround (s.crush * 7.0f) : 1;

        const float dtime = std::clamp (s.delayMs, 20.0f, 1600.0f) / 1000.0f;
        delaySamples = (int) std::lround (std::min (dtime, 1.6f) * sr);

        wet = std::min (1.0f, s.echo + s.crush * 0.45f + s.ring * 0.2f + dtime * 0.04f);
        fb = s.feedback >= 0.0f
                ? std::clamp (s.feedback, 0.0f, 0.98f)
                : std::min (1.0f, s.echo * 0.95f + dtime * 0.08f);
        trim = s.outputTrim >= 0.0f ? s.outputTrim : 1.0f / (1.0f + wet * 0.6f);
        ringDepth = std::clamp (s.ring, 0.0f, 1.0f);
        ringInc = 2.0f * kPi * std::clamp (s.ringHz, 1.0f, sr * 0.45f) / sr;
    }

    float processSample (float x)
    {
        // wet path: band pass, then bit crush
        float v = lp.process (hp.process (x));
        v = std::round (v * steps) / steps;

        // echo, read before the write so the delay length is exact
        int readPos = writePos - delaySamples;
        while (readPos < 0) readPos += (int) delayLine.size();
        const float delayed = delayLine[(std::size_t) readPos];
        delayLine[(std::size_t) writePos] = v + fb * delayed;
        if (++writePos >= (int) delayLine.size()) writePos = 0;

        // ring modulation: amplitude modulation, carrier at ringHz
        const float car = std::sin (phase);
        phase += ringInc;
        if (phase > 2.0f * kPi) phase -= 2.0f * kPi;
        const float ringed = v * (1.0f + ringDepth * car) * ringDepth;

        float out = (x * settings.dry + delayed * wet + ringed) * trim;

        // sample and hold across the whole mix, last in the chain
        if (holdCount > 1)
        {
            if (holdLeft <= 0) { held = out; holdLeft = holdCount; }
            else { out = held; }
            --holdLeft;
        }

        return std::clamp (out, -1.5f, 1.5f);
    }

    Settings settings {};

private:
    float sr = 48000.0f;
    Biquad hp, lp;

    std::vector<float> delayLine;
    int writePos = 0;
    int delaySamples = 1;

    float phase = 0.0f;
    float ringInc = 0.0f;
    float ringDepth = 0.0f;

    float bits = 10.0f;
    float steps = 1024.0f;
    int holdCount = 1;
    int holdLeft = 0;
    float held = 0.0f;

    float wet = 0.0f;
    float fb = 0.0f;
    float trim = 1.0f;
};
} // namespace nullcam
