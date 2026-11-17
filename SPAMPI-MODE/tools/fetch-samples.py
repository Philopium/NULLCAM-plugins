#!/usr/bin/env python3
"""Fetch the factory recordings for SPAMPI-MODE from Freesound.

Every sound is searched with the "Creative Commons 0" (CC0) filter, downloaded
as the public preview, checked on its own page for the CC0 licence, and then
converted to the 48 kHz mono 16 bit WAV the plugin expects.

The result of a run is written to samples.json, which is what SAMPLES.md is
made from, so the provenance of every bundled file stays in the repository.

    python tools/fetch-samples.py                 # fetch everything that is missing
    python tools/fetch-samples.py --only monkey   # just one sound

Needs: numpy, miniaudio (pip install numpy miniaudio)
"""

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

import numpy as np

try:
    import miniaudio
except ImportError:
    sys.exit("miniaudio is missing: pip install miniaudio")

# The titles come from Freesound and can hold any character.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except AttributeError:
    pass

HERE = os.path.dirname(os.path.abspath(__file__))
SAMPLE_DIR = os.path.normpath(os.path.join(HERE, "..", "Source", "Samples"))
JSON_PATH = os.path.join(HERE, "samples.json")

TARGET_RATE = 48000
PEAK = 0.88

# The factory voices: the file, the search words, how long the excerpt is,
# whether the sound should hold a steady pitch (the Rainbow Machine bends
# whatever it is given, so a pitched voice is the funny one), and the words
# the title has to contain, so a "creaky door" never passes for an elephant.
RECIPES = [
    ("monkey_howl",       "monkey howl",         4.0, True,  ["monkey", "ape", "howler"]),
    ("ambulance_siren",   "ambulance siren",     4.0, True,  ["ambulance", "siren", "sirene"]),
    ("baby_cry",          "baby crying",         3.5, True,  ["baby", "infant"]),
    ("cat_meow",          "cat meow",            3.0, True,  ["cat", "meow"]),
    ("dog_bark",          "dog barking",         2.5, False, ["bark"]),
    ("toilet_flush",      "toilet flush",        3.5, False, ["toilet", "flush", "wc"]),
    ("train_whistle",     "train whistle",       4.0, True,  ["train", "whistle", "horn"]),
    ("evil_laugh",        "evil laugh",          3.5, False, ["laugh", "laughter", "joker"]),
    ("elephant_trumpet",  "elephant trumpet",    3.5, True,  ["elephant"]),
    ("cow_moo",           "cow moo",             3.5, True,  ["cow", "moo", "cattle"]),
    ("rooster_crow",      "rooster crow",        3.0, True,  ["rooster", "cock", "crow"]),
    ("can_opening",       "tin can opening",     2.0, False, ["can", "soda", "beer", "tin"]),
]

# How long the search is allowed to look: a long recording is fine, the
# excerpt is cut out of the loudest part of it.
SEARCH_MAX_DURATION = 25.0

USER_AGENT = "Mozilla/5.0 (compatible; NULLCAM-SPAMPI/1.0; +https://github.com/Philopium/NULLCAM-plugins)"


def fetch(url, timeout=90):
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return response.read()


def search_freesound(query, max_duration=SEARCH_MAX_DURATION):
    """The public search page, filtered to CC0 and to short sounds."""
    params = {
        "q": query,
        "f": 'license:"Creative Commons 0"',
        "d1": str(max_duration),
        "s": "Automatic by relevance",
    }
    url = "https://freesound.org/search/?" + urllib.parse.urlencode(params)
    html = fetch(url).decode("utf-8", "replace")

    blocks = []
    for match in re.finditer(
        r'data-sound-id="(?P<id>\d+)".*?data-username="(?P<user>[^"]+)".*?'
        r'data-user-id="(?P<uid>\d+)".*?data-mp3="(?P<mp3>[^"]+)".*?'
        r'data-title="(?P<title>[^"]*)".*?data-duration="(?P<duration>[0-9.]+)".*?'
        r'data-samplerate="(?P<rate>[0-9.]+)"',
        html,
        re.S,
    ):
        g = match.groupdict()
        try:
            duration = float(g["duration"])
        except ValueError:
            continue

        if duration < 0.25 or duration > max_duration:
            continue

        blocks.append(
            {
                "id": int(g["id"]),
                "author": g["user"],
                "title": g["title"],
                "duration": duration,
                "samplerate": float(g["rate"]),
                "preview_lq": g["mp3"],
                "page": "https://freesound.org/people/%s/sounds/%s/" % (g["user"], g["id"]),
            }
        )

    return blocks


def choose(candidates):
    """A short list to look at: not too short, not too long, good sample rate."""
    def score(item):
        duration = item["duration"]
        duration_score = 1.0 - min(1.0, abs(duration - 2.5) / 4.0)
        rate_score = min(1.0, item["samplerate"] / 48000.0)
        return duration_score * 0.6 + rate_score * 0.4

    return sorted(candidates, key=score, reverse=True)[:4]


def realness(samples, rate):
    """How much high frequency energy there is.

    A field recording always has some air, hiss and brush noise up top. A
    synthesised tone has almost none, so this separates a real recording from
    a machine made one.
    """
    if samples.size < 2048:
        return 0.0

    total = 0.0
    high = 0.0

    for start in range(0, samples.size - 2048, 2048):
        window = samples[start:start + 2048]

        if np.sqrt(np.mean(window ** 2)) < 1.0e-5:
            continue

        spectrum = np.abs(np.fft.rfft(window * np.hanning(2048))) ** 2
        freqs = np.fft.rfftfreq(2048, 1.0 / rate)
        total += float(spectrum.sum())
        high += float(spectrum[freqs > 8000].sum())

    if total <= 0.0:
        return 0.0

    return high / total


def pitch_stability(samples, rate):
    """How steady the pitch is, in semitones, and how often it is there.

    The machine bends the sample, so a voice that holds a note is the one
    that makes the effect funny.
    """
    if samples.size < 4096:
        return 0.0, 0.0

    notes = []
    min_lag = max(2, int(rate / 1200.0))
    max_lag = min(2046, int(rate / 70.0))

    for start in range(0, samples.size - 4096, 2048):
        window = samples[start:start + 4096]

        if np.sqrt(np.mean(window ** 2)) < 1.0e-4:
            continue

        best_value = 0.0
        best_lag = 0

        for lag in range(min_lag, max_lag, 2):
            value = float(np.dot(window[:-lag:2], window[lag::2]))
            if value > best_value:
                best_value = value
                best_lag = lag

        if best_lag == 0:
            continue

        # The correlation has to be strong enough to be a real period.
        norm = float(np.sqrt(np.dot(window, window)))
        if norm > 0.0 and best_value / (norm * norm + 1.0e-12) > 0.35:
            notes.append(69.0 + 12.0 * np.log2((rate / best_lag) / 440.0))

    if len(notes) < 3:
        return 0.0, 0.0

    spread = float(np.std(notes))
    steadiness = max(0.0, 1.0 - spread / 6.0)
    coverage = min(1.0, len(notes) / 8.0)

    return steadiness * coverage, spread


def verify_cc0(page_html):
    """The sound page has to say CC0 too: the search filter is not proof."""
    text = page_html.lower()
    return ("creative commons 0" in text) or ("creativecommons.org/publicdomain/zero" in text)


def decode_mp3(data):
    decoded = miniaudio.decode(data)
    samples = np.frombuffer(decoded.samples, dtype=np.int16).astype(np.float32) / 32768.0

    if decoded.nchannels > 1:
        samples = samples.reshape(-1, decoded.nchannels).mean(axis=1)

    return samples, decoded.sample_rate


def resample(samples, source_rate, target_rate):
    """Windowed sinc, so 44.1 kHz previews do not grow a bright aliasing edge."""
    if source_rate == target_rate or samples.size == 0:
        return samples.astype(np.float64)

    ratio = target_rate / float(source_rate)
    out_length = int(round(samples.size * ratio))
    positions = np.arange(out_length) / ratio

    half = 16
    base = np.floor(positions).astype(np.int64)
    fraction = positions - base

    out = np.zeros(out_length, dtype=np.float64)

    for offset in range(-half + 1, half + 1):
        index = base + offset
        distance = fraction - offset

        # The ideal low pass is sinc(), cut down with a Blackman window.
        window = np.where(
            np.abs(distance) < half,
            0.42 + 0.5 * np.cos(np.pi * distance / half)
            + 0.08 * np.cos(2.0 * np.pi * distance / half),
            0.0,
        )
        weight = np.sinc(distance) * window

        valid = (index >= 0) & (index < samples.size)
        out[valid] += samples[index[valid]] * weight[valid]

    return out


def cut_excerpt(samples, seconds, rate):
    """The loudest, busiest moment of the recording, with a short fade."""
    if samples.size == 0:
        return samples

    window = int(rate * 0.02)
    if samples.size <= window:
        return samples

    frames = samples.size // window
    energy = np.array([np.mean(samples[i * window:(i + 1) * window] ** 2) for i in range(frames)])
    loudest = int(np.argmax(energy))

    start = max(0, loudest * window - int(rate * 0.35))
    length = min(samples.size - start, int(rate * seconds))

    if length < rate * 0.4:
        start = 0
        length = min(samples.size, int(rate * seconds))

    # Keep the whole excerpt inside the recording: if the loudest moment is
    # near the end, slide the window back so it is not cut short.
    wanted = int(rate * seconds)
    if samples.size >= wanted:
        start = max(0, min(start, samples.size - wanted))
        length = wanted

    excerpt = samples[start:start + length].copy()

    # Remove the DC offset, then fade the edges so the loop does not click.
    excerpt -= float(np.mean(excerpt))
    fade = min(int(rate * 0.008), len(excerpt) // 4)

    if fade > 0:
        ramp = np.linspace(0.0, 1.0, fade)
        excerpt[:fade] *= ramp
        excerpt[-fade:] *= ramp[::-1]

    return excerpt


def write_wav(path, samples, rate):
    """16 bit mono PCM WAV, written by hand so there is no extra dependency."""
    clipped = np.clip(samples, -1.0, 1.0)
    pcm = (clipped * 32767.0).astype("<i2")

    with open(path, "wb") as handle:
        handle.write(b"RIFF")
        handle.write((36 + len(pcm) * 2).to_bytes(4, "little"))
        handle.write(b"WAVEfmt ")
        handle.write((16).to_bytes(4, "little"))
        handle.write((1).to_bytes(2, "little"))       # PCM
        handle.write((1).to_bytes(2, "little"))       # mono
        handle.write(rate.to_bytes(4, "little"))
        handle.write((rate * 2).to_bytes(4, "little"))  # byte rate
        handle.write((2).to_bytes(2, "little"))       # block align
        handle.write((16).to_bytes(2, "little"))      # bits
        handle.write(b"data")
        handle.write((len(pcm) * 2).to_bytes(4, "little"))
        handle.write(pcm.tobytes())


def fetch_preview(candidate):
    """The public preview, in the best quality that is served."""
    preview = candidate["preview_lq"].replace("-lq.mp3", "-hq.mp3")

    try:
        return fetch(preview)
    except urllib.error.HTTPError:
        return fetch(candidate["preview_lq"])


def process(name, query, seconds, pitched, keywords, force=False):
    target = os.path.join(SAMPLE_DIR, name + ".wav")

    if os.path.exists(target) and not force:
        print("%-18s already there" % name)
        return None

    print("%-18s searching for %r ..." % (name, query))
    candidates = search_freesound(query)

    # Only the sounds whose title really is about the thing we are after.
    relevant = [
        item for item in candidates
        if any(word in item["title"].lower() for word in keywords)
    ]

    if not relevant:
        print("%-18s NOTHING RELEVANT (of %d results)" % (name, len(candidates)))
        return None

    shortlist = choose(relevant)
    best = None

    for candidate in shortlist:
        page_html = fetch(candidate["page"]).decode("utf-8", "replace")

        if not verify_cc0(page_html):
            print("   skipped (not CC0): %s" % candidate["title"])
            continue

        try:
            data = fetch_preview(candidate)
        except urllib.error.HTTPError as error:
            print("   skipped (no preview): %s" % candidate["title"])
            continue

        samples, rate = decode_mp3(data)
        samples = resample(samples, rate, TARGET_RATE)

        air = realness(samples, TARGET_RATE)
        steadiness, spread = pitch_stability(samples, TARGET_RATE) if pitched else (0.0, 0.0)

        # Real recording first, then a steady voice for the pitched ones, then a
        # recording long enough to give the sampler something to work with.
        air_score = min(1.0, air / 0.004)
        duration_fit = min(1.0, candidate["duration"] / max(0.5, seconds))
        rate_score = min(1.0, candidate["samplerate"] / 48000.0)

        score = air_score * 0.45 + steadiness * 0.25 + duration_fit * 0.15 + rate_score * 0.15

        print("   %-42s air=%.5f steady=%.2f spread=%.1f st  score=%.3f" % (
            candidate["title"][:42], air, steadiness, spread, score))

        if best is None or score > best["score"]:
            best = {
                "score": score,
                "candidate": candidate,
                "samples": samples,
                "air": air,
                "steadiness": steadiness,
            }

    if best is None:
        print("%-18s REFUSED: nothing usable" % name)
        return None

    candidate = best["candidate"]
    print("%-18s picked: %s by %s" % (name, candidate["title"], candidate["author"]))

    samples = cut_excerpt(best["samples"], seconds, TARGET_RATE)
    peak = float(np.max(np.abs(samples))) if samples.size else 0.0

    if peak < 1.0e-6:
        print("%-18s REFUSED: the recording is silent" % name)
        return None

    samples = samples * (PEAK / peak)
    write_wav(target, samples, TARGET_RATE)

    print("%-18s written: %.2fs, peak %.2f" % (name, samples.size / TARGET_RATE, peak))

    return {
        "file": name + ".wav",
        "title": candidate["title"],
        "author": candidate["author"],
        "id": candidate["id"],
        "page": candidate["page"],
        "license": "CC0 1.0 (public domain dedication)",
        "source_duration": candidate["duration"],
        "excerpt_duration": round(samples.size / TARGET_RATE, 3),
        "samplerate": TARGET_RATE,
        "channels": 1,
        "bits": 16,
        "realness_hf": round(best["air"], 6),
        "pitch_steadiness": round(best["steadiness"], 3),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", help="just this one sound")
    parser.add_argument("--force", action="store_true", help="fetch even if the file is there")
    args = parser.parse_args()

    os.makedirs(SAMPLE_DIR, exist_ok=True)

    results = {}
    if os.path.exists(JSON_PATH):
        with open(JSON_PATH, "r", encoding="utf-8") as handle:
            results = {item["file"]: item for item in json.load(handle)}

    for name, query, seconds, pitched, keywords in RECIPES:
        if args.only and name != args.only:
            continue

        item = process(name, query, seconds, pitched, keywords, force=args.force)

        if item:
            results[item["file"]] = item

    with open(JSON_PATH, "w", encoding="utf-8") as handle:
        json.dump(list(results.values()), handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    print("\nwrote %s" % JSON_PATH)


if __name__ == "__main__":
    main()
