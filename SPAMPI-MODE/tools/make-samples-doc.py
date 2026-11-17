#!/usr/bin/env python3
"""Write SAMPLES.md from the metadata the fetch script recorded.

    python tools/make-samples-doc.py
"""

import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
JSON_PATH = os.path.join(HERE, "samples.json")
DOC_PATH = os.path.normpath(os.path.join(HERE, "..", "SAMPLES.md"))

# The friendly name of each file, as the plugin shows it.
NAMES = {
    "monkey_howl.wav": "MONKEY HOWL",
    "ambulance_siren.wav": "AMBULANCE",
    "baby_cry.wav": "BABY CRY",
    "cat_meow.wav": "CAT MEOW",
    "dog_bark.wav": "DOG BARK",
    "toilet_flush.wav": "TOILET",
    "train_whistle.wav": "TRAIN",
    "evil_laugh.wav": "EVIL LAUGH",
    "elephant_trumpet.wav": "ELEPHANT",
    "cow_moo.wav": "COW",
    "rooster_crow.wav": "ROOSTER",
    "can_opening.wav": "SPAM",
}

HEADER = """# The factory recordings

Every sound bundled with SPAMPI-MODE is a **real recording** taken from
[Freesound](https://freesound.org/), under the **CC0 1.0** licence (a public
domain dedication: free to use, free to redistribute, no attribution needed -
but every author is credited here anyway).

CC0 was chosen on purpose: it is the only Creative Commons licence that lets
the recordings travel inside a plugin bundle without asking anybody first.

| Plugin voice | File | Original recording | Author | Licence |
| --- | --- | --- | --- | --- |
"""

FOOTER = """
## What was done to the recordings

The Freesound **preview** of every sound was downloaded (the public preview
file, which is the same recording in a compressed format), then:

- decoded to mono, because a sampler that plays a stereo file in the middle of
  a mix is a recipe for surprises;
- resampled to 48 kHz with a windowed sinc filter;
- cut to the loudest {seconds} seconds, so the plugin starts on the voice and
  not on a long silence;
- faded at both edges, so the loop does not click;
- normalised to a peak of 0.88, so all the voices start at the same level.

The result is 48 kHz, mono, 16 bit PCM WAV, which is what the plugin loads at
startup.

## Reproducing them

The whole thing is scripted, so the list can be checked and rebuilt:

```bash
cd SPAMPI-MODE
pip install numpy miniaudio
python tools/fetch-samples.py        # downloads and converts, writes tools/samples.json
python tools/make-samples-doc.py     # rewrites this file from samples.json
```

`tools/fetch-samples.py` searches Freesound with the CC0 filter, checks the
licence on the sound page of every candidate, and refuses any file that is not
public domain. The metadata of the last run is kept in `tools/samples.json`.

## Your own recordings

Any audio file can be dropped into the plugin from the panel (**LOAD
SAMPLE**) or copied into the user sample folder:

```
Windows  %APPDATA%\\NULLCAM\\SPAMPI-MODE\\samples
macOS    ~/Library/Application Support/NULLCAM/SPAMPI-MODE/samples
```

The plugin reads WAV, AIFF, FLAC, Ogg Vorbis and MP3. Files you bring are
yours: the plugin never uploads anything and never writes to the originals.
"""


def main():
    with open(JSON_PATH, "r", encoding="utf-8") as handle:
        samples = json.load(handle)

    samples.sort(key=lambda item: list(NAMES).index(item["file"])
                 if item["file"] in NAMES else 99)

    rows = []

    for item in samples:
        name = NAMES.get(item["file"], item["file"].replace(".wav", "").upper())
        title = item["title"].replace("|", "\\|")
        rows.append(
            "| %s | `%s` | [%s](%s) | %s | CC0 1.0 |"
            % (name, item["file"], title, item["page"], item["author"])
        )

    durations = ", ".join(
        "%s %.1f s" % (NAMES.get(item["file"], item["file"]), item["excerpt_duration"])
        for item in samples
    )

    text = HEADER + "\n".join(rows) + "\n" + FOOTER.format(seconds="3 to 4")

    with open(DOC_PATH, "w", encoding="utf-8") as handle:
        handle.write(text)

    print("wrote %s (%d recordings)" % (DOC_PATH, len(samples)))
    print("durations: %s" % durations)


if __name__ == "__main__":
    main()
