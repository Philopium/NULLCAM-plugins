// Measures the chain without any plugin host: feeds test signals and checks that
// each stage does what the app does. Build: see ../README.md
#include "../Source/NullcamChain.h"

#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <vector>

namespace
{
constexpr double kSr = 48000.0;

double rms (const std::vector<float>& v, std::size_t from)
{
    double sum = 0.0;
    for (std::size_t i = from; i < v.size(); ++i) sum += (double) v[i] * v[i];
    const std::size_t n = v.size() - from;
    return n ? std::sqrt (sum / (double) n) : 0.0;
}

// magnitude of one frequency, cheap and accurate enough for a Goertzel bin
double tone (const std::vector<float>& v, double hz, std::size_t from)
{
    const double w = 2.0 * M_PI * hz / kSr;
    const double coeff = 2.0 * std::cos (w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (std::size_t i = from; i < v.size(); ++i)
    {
        s0 = (double) v[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return std::sqrt (s1 * s1 + s2 * s2 - coeff * s1 * s2) / (double) (v.size() - from);
}

std::vector<float> render (const nullcam::Settings& s, double hz, double seconds = 1.0)
{
    nullcam::Chain chain;
    chain.prepare (kSr);
    chain.setSettings (s);
    const std::size_t n = (std::size_t) (kSr * seconds);
    std::vector<float> out (n);
    for (std::size_t i = 0; i < n; ++i)
        out[i] = chain.processSample ((float) (0.5 * std::sin (2.0 * M_PI * hz * (double) i / kSr)));
    return out;
}

int failures = 0;

void check (bool ok, const char* what, double value)
{
    std::printf ("%-58s %s  (%.4f)\n", what, ok ? "ok  " : "FAIL", value);
    if (!ok) ++failures;
}
} // namespace

int main()
{
    // 1. high pass: 330 Hz cuts the low end, leaves the midrange
    {
        nullcam::Settings s;
        s.hpfHz = 330.0f; s.lpfHz = 16000.0f; s.echo = 0.0f; s.ring = 0.0f; s.dry = 0.0f;
        const auto low = render (s, 100.0);
        const auto mid = render (s, 2000.0);
        const double ratio = rms (low, 24000) / rms (mid, 24000);
        check (ratio < 0.2, "highpass 330 Hz kills 100 Hz, keeps 2 kHz", ratio);
    }

    // 2. low pass: 800 Hz muffles the top
    {
        nullcam::Settings s;
        s.hpfHz = 70.0f; s.lpfHz = 800.0f; s.echo = 0.0f; s.ring = 0.0f; s.dry = 0.0f;
        const auto high = render (s, 8000.0);
        const auto lowish = render (s, 400.0);
        const double ratio = rms (high, 24000) / rms (lowish, 24000);
        check (ratio < 0.25, "lowpass 800 Hz kills 8 kHz, keeps 400 Hz", ratio);
    }

    // 3. ring modulation puts sidebands at f0 +/- ringHz
    {
        nullcam::Settings s;
        s.hpfHz = 70.0f; s.lpfHz = 16000.0f; s.echo = 0.0f; s.ring = 1.0f; s.ringHz = 117.0f; s.dry = 0.0f;
        // the app amplitude-modulates with 1 + sin, so the carrier survives and the
        // sidebands sit around 40% of it: what matters is that they are there at all
        const auto out = render (s, 1000.0, 2.0);
        const double carrier = tone (out, 1000.0, 48000);
        const double lower = tone (out, 883.0, 48000);
        const double upper = tone (out, 1117.0, 48000);
        check (lower > carrier * 0.2 && upper > carrier * 0.2,
               "ring 117 Hz puts energy in the 883/1117 Hz sidebands", lower / carrier);

        nullcam::Settings off = s;
        off.ring = 0.0f;
        const auto dryish = render (off, 1000.0, 2.0);
        check (tone (dryish, 883.0, 48000) < tone (out, 883.0, 48000) * 0.05,
               "ring 0 leaves no sideband behind", tone (dryish, 883.0, 48000));
    }

    // 4. the echo comes back where the delay knob says
    {
        nullcam::Settings s;
        s.hpfHz = 70.0f; s.lpfHz = 16000.0f; s.crush = 0.0f;
        s.delayMs = 344.0f; s.echo = 1.0f; s.ring = 0.0f; s.dry = 0.0f; s.outputTrim = 1.0f;
        nullcam::Chain chain;
        chain.prepare (kSr);
        chain.setSettings (s);
        std::vector<float> out (48000, 0.0f);
        for (std::size_t i = 0; i < out.size(); ++i) out[i] = chain.processSample (i == 0 ? 1.0f : 0.0f);

        int peak = 0;
        float best = 0.0f;
        for (int i = 1; i < (int) out.size(); ++i)
            if (std::fabs (out[i]) > best) { best = std::fabs (out[i]); peak = i; }
        const double ms = peak * 1000.0 / kSr;
        check (std::fabs (ms - 344.0) < 3.0, "echo lands at the delay time (ms)", ms);
    }

    // 5. crush quantises hard at the top of the knob
    {
        nullcam::Settings s;
        // the dry branch bypasses the crusher, as in the app: mute it to hear the wet one
        // feedback off as well, otherwise the delay sums partial levels back in
        s.hpfHz = 70.0f; s.lpfHz = 16000.0f; s.echo = 0.0f; s.ring = 0.0f; s.dry = 0.0f; s.crush = 1.0f;
        s.feedback = 0.0f;
        const auto out = render (s, 220.0, 1.0);
        std::vector<float> levels;
        for (std::size_t i = 24000; i < out.size(); ++i)
        {
            bool seen = false;
            for (float l : levels) if (std::fabs (l - out[i]) < 1e-6f) { seen = true; break; }
            if (!seen) levels.push_back (out[i]);
        }
        check (levels.size() > 0 && levels.size() <= 9,
               "crush 1.0 drops the signal to a handful of levels", (double) levels.size());
    }

    // 6. a clean setting passes the sound through untouched
    {
        nullcam::Settings s;
        s.hpfHz = 20.0f; s.lpfHz = 20000.0f; s.crush = 0.0f;
        s.echo = 0.0f; s.ring = 0.0f; s.dry = 1.0f; s.outputTrim = 1.0f;
        const auto out = render (s, 1000.0, 1.0);
        const double inRms = 0.5 / std::sqrt (2.0);
        const double ratio = rms (out, 24000) / inRms;
        check (std::fabs (ratio - 1.0) < 0.05, "clean settings keep unity gain", ratio);
    }

    std::printf ("\n%s\n", failures == 0 ? "all chain checks passed" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
