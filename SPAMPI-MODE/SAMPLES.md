# The factory recordings

Every sound bundled with SPAMPI-MODE is a **real recording** taken from
[Freesound](https://freesound.org/), under the **CC0 1.0** licence (a public
domain dedication: free to use, free to redistribute, no attribution needed -
but every author is credited here anyway).

CC0 was chosen on purpose: it is the only Creative Commons licence that lets
the recordings travel inside a plugin bundle without asking anybody first.

| Plugin voice | File | Original recording | Author | Licence |
| --- | --- | --- | --- | --- |
| MONKEY HOWL | `monkey_howl.wav` | [screaming monkey or bird.wav](https://freesound.org/people/sethlind/sounds/332726/) | sethlind | CC0 1.0 |
| AMBULANCE | `ambulance_siren.wav` | [siren ambulance european distant echo nice with steps to clean.wav](https://freesound.org/people/kyles/sounds/451068/) | kyles | CC0 1.0 |
| BABY CRY | `baby_cry.wav` | [baby cry.mp3](https://freesound.org/people/mariiao2/sounds/240372/) | mariiao2 | CC0 1.0 |
| CAT MEOW | `cat_meow.wav` | [SND_CatHighMeows.wav](https://freesound.org/people/megrez7274/sounds/508686/) | megrez7274 | CC0 1.0 |
| DOG BARK | `dog_bark.wav` | [Barking Inez 2](https://freesound.org/people/wateenfotos/sounds/749351/) | wateenfotos | CC0 1.0 |
| TOILET | `toilet_flush.wav` | [Flush WC Toilete 1](https://freesound.org/people/frenkfurth/sounds/776528/) | frenkfurth | CC0 1.0 |
| TRAIN | `train_whistle.wav` | [whistle for train Jakarta INDONESIA 940316.flac](https://freesound.org/people/TRP/sounds/567326/) | TRP | CC0 1.0 |
| EVIL LAUGH | `evil_laugh.wav` | [Joker Laugh 1](https://freesound.org/people/BrainClaim/sounds/427574/) | BrainClaim | CC0 1.0 |
| ELEPHANT | `elephant_trumpet.wav` | [G12-17-Elephant Trumpeting.wav](https://freesound.org/people/craigsmith/sounds/437949/) | craigsmith | CC0 1.0 |
| COW | `cow_moo.wav` | [Cow Moo](https://freesound.org/people/invertedturtle/sounds/546481/) | invertedturtle | CC0 1.0 |
| ROOSTER | `rooster_crow.wav` | [Rooster crow](https://freesound.org/people/arelitron/sounds/582624/) | arelitron | CC0 1.0 |
| SPAM | `can_opening.wav` | [Soda can - Open](https://freesound.org/people/Khrinx/sounds/333697/) | Khrinx | CC0 1.0 |

## What was done to the recordings

The Freesound **preview** of every sound was downloaded (the public preview
file, which is the same recording in a compressed format), then:

- decoded to mono, because a sampler that plays a stereo file in the middle of
  a mix is a recipe for surprises;
- resampled to 48 kHz with a windowed sinc filter;
- cut to the loudest 3 to 4 seconds, so the plugin starts on the voice and
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
Windows  %APPDATA%\NULLCAM\SPAMPI-MODE\samples
macOS    ~/Library/Application Support/NULLCAM/SPAMPI-MODE/samples
```

The plugin reads WAV, AIFF, FLAC, Ogg Vorbis and MP3. Files you bring are
yours: the plugin never uploads anything and never writes to the originals.
