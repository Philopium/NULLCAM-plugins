#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <SpampiAssets.h>

#include <algorithm>
#include <cmath>

namespace
{
// The recordings that travel inside the binary, in the order the factory
// presets expect them (Preset::monkey is 0, Preset::ambulance is 1, ...).
struct BundledFile
{
    const char* displayName;
    const char* data;
    int size;
};

const BundledFile& bundledFile (int index)
{
    static const BundledFile files[] = {
        { "MONKEY HOWL", SpampiAssets::monkey_howl_wav,      SpampiAssets::monkey_howl_wavSize },
        { "AMBULANCE",   SpampiAssets::ambulance_siren_wav,  SpampiAssets::ambulance_siren_wavSize },
        { "BABY CRY",    SpampiAssets::baby_cry_wav,         SpampiAssets::baby_cry_wavSize },
        { "CAT MEOW",    SpampiAssets::cat_meow_wav,         SpampiAssets::cat_meow_wavSize },
        { "DOG BARK",    SpampiAssets::dog_bark_wav,         SpampiAssets::dog_bark_wavSize },
        { "TOILET",      SpampiAssets::toilet_flush_wav,     SpampiAssets::toilet_flush_wavSize },
        { "TRAIN",       SpampiAssets::train_whistle_wav,    SpampiAssets::train_whistle_wavSize },
        { "EVIL LAUGH",  SpampiAssets::evil_laugh_wav,       SpampiAssets::evil_laugh_wavSize },
        { "ELEPHANT",    SpampiAssets::elephant_trumpet_wav, SpampiAssets::elephant_trumpet_wavSize },
        { "COW",         SpampiAssets::cow_moo_wav,          SpampiAssets::cow_moo_wavSize },
        { "ROOSTER",     SpampiAssets::rooster_crow_wav,     SpampiAssets::rooster_crow_wavSize },
        { "SPAM",        SpampiAssets::can_opening_wav,      SpampiAssets::can_opening_wavSize }
    };

    return files[juce::jlimit (0, (int) std::size (files) - 1, index)];
}

constexpr int kNumBundled = 12;
constexpr int kMaxPitchHistory = 256;

const char* const kAudioWildcards = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";

// Anything longer than this is not a sampler voice, and measuring all of it
// would make the plugin slow to open.
constexpr double kMaxMeasureSeconds = 2.0;
} // namespace

//==============================================================================
SpampiModeAudioProcessor::SpampiModeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& slot : sampleSlots)
        slot.store (nullptr, std::memory_order_relaxed);

    loadBundledSamples();
    refreshUserSamples();
    publishSamples();

    // Open on the monkey, so the plugin makes its noise the moment it is added.
    applyPreset (Preset::monkey);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpampiModeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* id, const char* name,
                          juce::NormalisableRange<float> range, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    // The recording, and where its pitch is measured from.
    add (ids::sampleIndex, "SAMPLE", juce::NormalisableRange<float> (0.0f, 63.0f, 1.0f), 0.0f);
    add (ids::root, "ROOT", juce::NormalisableRange<float> (21.0f, 108.0f, 1.0f), 48.0f);

    // How the playing bends the recording.
    add (ids::follow, "FOLLOW", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f);
    add (ids::interval, "INTERVAL", juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f);
    add (ids::spread, "SPREAD", juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f);
    add (ids::glide, "GLIDE", juce::NormalisableRange<float> (0.0f, 1.0f), 0.12f);
    add (ids::drift, "DRIFT", juce::NormalisableRange<float> (0.0f, 1.0f), 0.10f);
    add (ids::machine, "MACHINE", juce::NormalisableRange<float> (0.0f, 1.0f), 0.20f);

    // How it is triggered, and how it sounds.
    add (ids::trigger, "TRIGGER", juce::NormalisableRange<float> (0.0f, 3.0f, 1.0f), 0.0f);
    add (ids::sens, "SENS", juce::NormalisableRange<float> (0.0f, 1.0f), 0.55f);
    add (ids::tone, "TONE", juce::NormalisableRange<float> (0.0f, 1.0f), 0.85f);
    add (ids::mix, "MIX", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f);
    add (ids::level, "LEVEL", juce::NormalisableRange<float> (0.0f, 2.0f), 0.90f);

    return layout;
}

//==============================================================================
// The factory voices. Every one of them is a real recording and a handful of
// silly machine settings: the point is that the guitar plays the animal.
namespace
{
struct PresetValues
{
    int sample;
    float follow, interval, spread, glide, drift, machine;
    int trigger;
    float sens, tone, mix, level;
};

const PresetValues& presetValues (SpampiModeAudioProcessor::Preset preset)
{
    static const PresetValues table[] = {
        // sample, follow, interval, spread, glide, drift, machine, trig, sens, tone, mix, level
        // trig: 0 = SHOT (one hit per note, cut at the peak), 1 = LOOP, 2 = GATE, 3 = TRIG
        { 0,  1.0f,  0.0f, 0.35f, 0.08f, 0.15f, 0.25f, 0, 0.60f, 0.90f, 1.0f, 0.85f },  // MONKEY
        { 1,  1.0f,  7.0f, 0.00f, 0.30f, 0.35f, 0.15f, 0, 0.55f, 0.85f, 1.0f, 0.80f },  // WEE-OO
        { 2,  1.0f,  5.0f, 0.50f, 0.04f, 0.05f, 0.35f, 2, 0.62f, 0.88f, 1.0f, 0.82f },  // BABY
        { 3,  1.0f,  0.0f, 1.00f, 0.00f, 0.00f, 0.50f, 2, 0.60f, 0.92f, 1.0f, 0.88f },  // MEOW STACK
        { 4,  1.0f, -5.0f, 0.50f, 0.02f, 0.00f, 0.40f, 0, 0.58f, 0.80f, 1.0f, 0.90f },  // GOOD BOY
        { 5,  0.0f,  0.0f, 1.00f, 0.00f, 0.60f, 0.60f, 1, 0.50f, 0.70f, 1.0f, 0.75f },  // FLUSH
        { 6,  1.0f, -12.0f, 0.00f, 0.40f, 0.20f, 0.25f, 1, 0.55f, 0.75f, 1.0f, 0.85f },  // NIGHT TRAIN
        { 7,  0.4f,  3.0f, 0.70f, 0.03f, 0.10f, 0.70f, 2, 0.58f, 0.86f, 1.0f, 0.80f },  // EVIL LAUGH
        { 8,  1.0f,  0.0f, 0.10f, 0.10f, 0.05f, 0.30f, 0, 0.52f, 0.95f, 1.0f, 0.85f },  // TRUMPET
        { 9,  1.0f, -7.0f, 0.60f, 0.05f, 0.10f, 0.20f, 2, 0.60f, 0.82f, 1.0f, 0.90f },  // MOO
        { 10, 1.0f,  2.0f, 0.40f, 0.02f, 0.00f, 0.35f, 2, 0.62f, 0.90f, 1.0f, 0.86f },  // ROOSTER
        { 11, 0.0f, -12.0f, 1.00f, 0.00f, 0.30f, 0.80f, 0, 0.50f, 0.60f, 1.0f, 0.72f }   // SPAM
    };

    return table[juce::jlimit (0, (int) std::size (table) - 1, (int) preset)];
}
} // namespace

juce::String SpampiModeAudioProcessor::getFactoryPresetName (Preset preset)
{
    static const char* names[] = {
        "MONKEY", "WEE-OO", "BABY", "MEOW STACK", "GOOD BOY", "FLUSH",
        "NIGHT TRAIN", "EVIL LAUGH", "TRUMPET", "MOO", "ROOSTER", "SPAM"
    };

    return names[juce::jlimit (0, kNumPresets - 1, (int) preset)];
}

void SpampiModeAudioProcessor::applyPreset (Preset preset)
{
    const auto& v = presetValues (preset);

    suppressAutoRoot = false;

    setParam (ids::sampleIndex, (float) v.sample);
    setParam (ids::follow, v.follow);
    setParam (ids::interval, v.interval);
    setParam (ids::spread, v.spread);
    setParam (ids::glide, v.glide);
    setParam (ids::drift, v.drift);
    setParam (ids::machine, v.machine);
    setParam (ids::trigger, (float) v.trigger);
    setParam (ids::sens, v.sens);
    setParam (ids::tone, v.tone);
    setParam (ids::mix, v.mix);
    setParam (ids::level, v.level);

    // The root belongs to the recording, not to the preset.
    applySampleIndex();
}

juce::String SpampiModeAudioProcessor::matchFactoryPreset() const
{
    for (int i = 0; i < kNumPresets; ++i)
    {
        const auto preset = (Preset) i;
        const auto& v = presetValues (preset);

        const auto value = [this] (const char* id)
        {
            auto* parameter = apvts.getParameter (id);
            return parameter != nullptr ? parameter->convertFrom0to1 (parameter->getValue()) : 0.0f;
        };

        const bool matches =
            std::abs (value (ids::sampleIndex) - (float) v.sample) < 0.01f
            && std::abs (value (ids::follow) - v.follow) < 0.002f
            && std::abs (value (ids::interval) - v.interval) < 0.01f
            && std::abs (value (ids::spread) - v.spread) < 0.002f
            && std::abs (value (ids::glide) - v.glide) < 0.002f
            && std::abs (value (ids::drift) - v.drift) < 0.002f
            && std::abs (value (ids::machine) - v.machine) < 0.002f
            && std::abs (value (ids::trigger) - (float) v.trigger) < 0.01f
            && std::abs (value (ids::sens) - v.sens) < 0.002f
            && std::abs (value (ids::tone) - v.tone) < 0.002f
            && std::abs (value (ids::mix) - v.mix) < 0.002f
            && std::abs (value (ids::level) - v.level) < 0.002f;

        if (matches)
            return getFactoryPresetName (preset);
    }

    return {};
}

//==============================================================================
// Samples
namespace
{
bool isSupportedAudioFile (const juce::File& file)
{
    const auto extension = file.getFileExtension().toLowerCase();

    return extension == ".wav" || extension == ".aif" || extension == ".aiff"
        || extension == ".flac" || extension == ".ogg" || extension == ".mp3";
}
} // namespace

void SpampiModeAudioProcessor::loadBundledSamples()
{
    for (int i = 0; i < kNumBundled; ++i)
    {
        const auto& file = bundledFile (i);
        loadSampleFromMemory (file.data, file.size, file.displayName);
    }
}

bool SpampiModeAudioProcessor::decodeToMono (juce::AudioFormatReader* reader,
                                             std::vector<float>& out, double& rate)
{
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels == 0)
        return false;

    const auto seconds = juce::jlimit (0.05, 60.0, (double) reader->lengthInSamples / reader->sampleRate);
    const auto total = (int) juce::jmin ((juce::int64) (reader->sampleRate * seconds),
                                        reader->lengthInSamples);

    if (total <= 0)
        return false;

    juce::AudioBuffer<float> buffer ((int) reader->numChannels, total);
    reader->read (&buffer, 0, total, 0, true, true);

    out.assign ((std::size_t) total, 0.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* source = buffer.getReadPointer (ch);

        for (int i = 0; i < total; ++i)
            out[(std::size_t) i] += source[i];
    }

    if (buffer.getNumChannels() > 1)
    {
        const auto gain = 1.0f / (float) buffer.getNumChannels();

        for (auto& sample : out)
            sample *= gain;
    }

    rate = reader->sampleRate;
    return true;
}

int SpampiModeAudioProcessor::measuredMidiOf (const SampleEntry& entry) const
{
    if (entry.rawData.size() < 4096)
        return -1;

    const auto limit = (std::size_t) juce::jmin ((double) entry.rawData.size(),
                                                 entry.rawRate * kMaxMeasureSeconds);

    spampi::PitchDetector detector;
    detector.prepare (entry.rawRate);

    std::vector<float> notes;

    for (std::size_t i = 0; i < limit; ++i)
        if (detector.push (entry.rawData[i]))
            if (detector.getConfidence() > 0.45f && detector.getFrequency() > 40.0f)
                notes.push_back (spampi::PitchDetector::midiFromFrequency (detector.getFrequency()));

    if (notes.size() >= 8)
    {
        std::sort (notes.begin(), notes.end());
        auto midi = (int) std::lround (notes[notes.size() / 2]);

        // A recording measured an octave high still plays: fold it into the range
        // a guitar can reach, so the root is a note and not a number.
        while (midi > 96) midi -= 12;
        while (midi < 28) midi += 12;

        return juce::jlimit (21, 108, midi);
    }

    // Nothing pitched in there: a howl, a bark, a flush. Rather than inherit the
    // root the previous recording left behind, estimate how bright the sound is
    // from its zero crossings and fold that into the range a guitar can play.
    const auto block = (std::size_t) juce::jmax (256, (int) (entry.rawRate * 0.04));
    std::vector<float> estimates;

    for (std::size_t start = 0; start + block <= limit; start += block)
    {
        auto energy = 0.0;
        auto crossings = 0;

        for (std::size_t i = start; i < start + block; ++i)
        {
            const auto value = entry.rawData[i];
            energy += (double) value * (double) value;

            if (i > start && ((value >= 0.0f) != (entry.rawData[i - 1] >= 0.0f)))
                ++crossings;
        }

        if (std::sqrt (energy / (double) block) < 0.02)
            continue;

        const auto hz = (double) crossings * entry.rawRate / (2.0 * (double) block);
        estimates.push_back (spampi::PitchDetector::midiFromFrequency ((float) hz));
    }

    if (estimates.empty())
        return -1;

    std::sort (estimates.begin(), estimates.end());
    auto midi = (int) std::lround (estimates[estimates.size() / 2]);

    while (midi > 76) midi -= 12;
    while (midi < 40) midi += 12;

    return juce::jlimit (21, 108, midi);
}

bool SpampiModeAudioProcessor::loadSampleFromMemory (const char* data, int size,
                                                     const juce::String& displayName)
{
    if (data == nullptr || size <= 0)
        return false;

    const juce::SpinLock::ScopedLockType lock (sampleLock);

    if ((int) samples.size() >= kMaxSamples)
        return false;

    auto stream = std::make_unique<juce::MemoryInputStream> (data, (std::size_t) size, false);

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (std::move (stream)));

    std::vector<float> mono;
    double rate = 48000.0;

    if (! decodeToMono (reader.get(), mono, rate))
        return false;

    auto entry = std::make_unique<SampleEntry>();
    entry->displayName = displayName;
    entry->path.clear();
    entry->isUser = false;
    entry->rawData = std::move (mono);
    entry->rawRate = rate;
    entry->measuredMidi = measuredMidiOf (*entry);
    entry->sample.data = spampi::resampleMono (entry->rawData, entry->rawRate, hostSampleRate);
    entry->sample.baseFrequency = entry->measuredMidi >= 0
        ? (double) spampi::PitchDetector::frequencyFromMidi ((float) entry->measuredMidi)
        : 0.0;
    entry->sample.name = displayName.toStdString();

    if (entry->sample.data.empty())
        return false;

    // Playback starts at the recording's own loudest moment, so the note attack
    // lands on the sound a person recognises instead of the room before it.
    entry->sample.startSample = spampi::peakStartOf (entry->sample.data, hostSampleRate);

    samples.push_back (std::move (entry));
    return true;
}

bool SpampiModeAudioProcessor::loadSampleFromFile (const juce::File& file,
                                                   const juce::String& displayName)
{
    if (! file.existsAsFile() || (int) samples.size() >= kMaxSamples)
        return false;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    std::vector<float> mono;
    double rate = 48000.0;

    if (! decodeToMono (reader.get(), mono, rate))
        return false;

    const juce::SpinLock::ScopedLockType lock (sampleLock);

    auto entry = std::make_unique<SampleEntry>();
    entry->displayName = displayName;
    entry->path = file.getFullPathName();
    entry->isUser = true;
    entry->rawData = std::move (mono);
    entry->rawRate = rate;
    entry->measuredMidi = measuredMidiOf (*entry);
    entry->sample.data = spampi::resampleMono (entry->rawData, entry->rawRate, hostSampleRate);
    entry->sample.baseFrequency = entry->measuredMidi >= 0
        ? (double) spampi::PitchDetector::frequencyFromMidi ((float) entry->measuredMidi)
        : 0.0;
    entry->sample.name = displayName.toStdString();

    if (entry->sample.data.empty())
        return false;

    samples.push_back (std::move (entry));
    return true;
}

void SpampiModeAudioProcessor::publishSamples()
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    const auto count = (int) samples.size();

    for (int i = 0; i < kMaxSamples; ++i)
        sampleSlots[(std::size_t) i].store (i < count ? &samples[(std::size_t) i]->sample : nullptr,
                                            std::memory_order_release);

    sampleCount.store (count, std::memory_order_release);
}

void SpampiModeAudioProcessor::applySampleIndex()
{
    publishSamples();

    const auto want = juce::jlimit (0, juce::jmax (0, getNumSamples() - 1),
                                    (int) std::lround (apvts.getRawParameterValue (ids::sampleIndex)->load()));

    requestedIndex.store (want, std::memory_order_relaxed);
    currentSampleIndex.store (want, std::memory_order_relaxed);

    if (! suppressAutoRoot)
    {
        const auto measured = getSampleMeasuredMidi (want);

        if (measured >= 0)
        {
            setParam (ids::root, (float) measured);
            pendingAutoRoot.store ((float) measured, std::memory_order_relaxed);
        }
    }

    suppressAutoRoot = false;
}

void SpampiModeAudioProcessor::sampleChosenByUser()
{
    suppressAutoRoot = false;
    applySampleIndex();
}

int SpampiModeAudioProcessor::getSampleIndexForPath (const juce::String& path) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    for (std::size_t i = 0; i < samples.size(); ++i)
        if (samples[i]->path == path)
            return (int) i;

    return -1;
}

juce::String SpampiModeAudioProcessor::getSampleName (int index) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    if (index < 0 || index >= (int) samples.size())
        return {};

    return samples[(std::size_t) index]->displayName;
}

juce::StringArray SpampiModeAudioProcessor::getSampleNames() const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    juce::StringArray names;

    for (const auto& entry : samples)
        names.add (entry->displayName);

    return names;
}

juce::String SpampiModeAudioProcessor::getSamplePath (int index) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    if (index < 0 || index >= (int) samples.size())
        return {};

    return samples[(std::size_t) index]->path;
}

int SpampiModeAudioProcessor::getSampleIndexForName (const juce::String& name) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    for (std::size_t i = 0; i < samples.size(); ++i)
        if (samples[i]->displayName.equalsIgnoreCase (name))
            return (int) i;

    return -1;
}

int SpampiModeAudioProcessor::getSampleMeasuredMidi (int index) const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    if (index < 0 || index >= (int) samples.size())
        return -1;

    return samples[(std::size_t) index]->measuredMidi;
}

juce::File SpampiModeAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("SPAMPI-MODE")
        .getChildFile ("presets");
}

juce::File SpampiModeAudioProcessor::getUserSampleFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("NULLCAM")
        .getChildFile ("SPAMPI-MODE")
        .getChildFile ("samples");
}

void SpampiModeAudioProcessor::refreshUserSamples()
{
    const auto folder = getUserSampleFolder();

    if (! folder.isDirectory())
        return;

    auto files = folder.findChildFiles (juce::File::findFiles, false, kAudioWildcards);
    files.sort();

    for (const auto& file : files)
    {
        if (getSampleIndexForPath (file.getFullPathName()) >= 0)
            continue;

        if ((int) samples.size() >= kMaxSamples)
            break;

        loadSampleFromFile (file, file.getFileNameWithoutExtension());
    }

    publishSamples();
}

int SpampiModeAudioProcessor::importUserSample (const juce::File& source)
{
    if (! source.existsAsFile())
        return -1;

    const auto folder = getUserSampleFolder();

    if (! folder.createDirectory().wasOk())
        return -1;

    const auto base = juce::File::createLegalFileName (source.getFileNameWithoutExtension());
    auto extension = source.getFileExtension().toLowerCase();

    if (extension.isEmpty())
        extension = ".wav";

    auto target = folder.getChildFile (base + extension);

    for (int copy = 2; target.existsAsFile() && copy < 1000; ++copy)
        target = folder.getChildFile (base + " (" + juce::String (copy) + ")" + extension);

    if (! source.copyFileTo (target))
        return -1;

    if (! loadSampleFromFile (target, target.getFileNameWithoutExtension()))
        return -1;

    publishSamples();

    const auto index = (int) samples.size() - 1;
    setParam (ids::sampleIndex, (float) index);
    sampleChosenByUser();

    return index;
}

void SpampiModeAudioProcessor::resampleAllTo (double sampleRate)
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);

    for (auto& entry : samples)
    {
        entry->sample.data = spampi::resampleMono (entry->rawData, entry->rawRate, sampleRate);
        entry->sample.baseFrequency = entry->measuredMidi >= 0
            ? (double) spampi::PitchDetector::frequencyFromMidi ((float) entry->measuredMidi)
            : 0.0;
        entry->sample.startSample = spampi::peakStartOf (entry->sample.data, sampleRate);
    }
}

bool SpampiModeAudioProcessor::copySampleWaveform (float* dest, int count) const
{
    if (dest == nullptr || count <= 0)
        return false;

    const juce::SpinLock::ScopedLockType lock (sampleLock);

    const auto index = currentSampleIndex.load (std::memory_order_relaxed);

    if (index < 0 || index >= (int) samples.size())
        return false;

    const auto& data = samples[(std::size_t) index]->sample.data;

    if (data.empty())
        return false;

    const auto bins = (std::size_t) count;
    const auto perBin = juce::jmax ((std::size_t) 1, data.size() / bins);

    for (int bin = 0; bin < count; ++bin)
    {
        const auto start = (std::size_t) bin * perBin;
        const auto end = juce::jmin (data.size(), start + perBin);

        auto peak = 0.0f;

        for (auto i = start; i < end; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));

        dest[bin] = peak;
    }

    return true;
}

float SpampiModeAudioProcessor::getSampleStartFraction() const
{
    const juce::SpinLock::ScopedTryLockType lock (sampleLock);

    if (! lock.isLocked())
        return 0.0f;

    const auto index = currentSampleIndex.load (std::memory_order_relaxed);

    if (index < 0 || index >= (int) samples.size())
        return 0.0f;

    const auto& sample = samples[(std::size_t) index]->sample;

    if (sample.data.empty())
        return 0.0f;

    return juce::jlimit (0.0f, 1.0f,
                         (float) sample.startSample / (float) sample.data.size());
}

//==============================================================================
bool SpampiModeAudioProcessor::copyPitchHistory (float* dest, int count) const
{
    if (dest == nullptr || count <= 0)
        return false;

    const juce::SpinLock::ScopedTryLockType lock (scopeLock);

    if (! lock.isLocked())
        return false;

    const auto n = juce::jmin (count, kMaxPitchHistory);
    const auto start = (int) historyPos;   // the next write is the oldest value

    for (int i = 0; i < n; ++i)
        dest[i] = pitchHistory[(std::size_t) ((start + i) % kMaxPitchHistory)];

    return true;
}

bool SpampiModeAudioProcessor::consumePendingAutoRoot()
{
    const auto value = pendingAutoRoot.exchange (-1.0f, std::memory_order_relaxed);
    return value >= 0.0f;
}

//==============================================================================
// User presets: one XML file each, exactly like the other two plugins, so a
// preset is a text file a person can read and keep.
namespace
{
juce::File presetFile (const juce::String& name)
{
    return SpampiModeAudioProcessor::getUserPresetFolder()
        .getChildFile (juce::File::createLegalFileName (name.trim()) + ".xml");
}
} // namespace

juce::StringArray SpampiModeAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;

    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}

bool SpampiModeAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty() || trimmed == "." || trimmed == "..")
        return false;

    if (! getUserPresetFolder().createDirectory().wasOk())
        return false;

    juce::XmlElement xml ("SPAMPI-MODE-PRESET");
    xml.setAttribute ("name", trimmed);
    xml.setAttribute ("version", 1);

    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
        {
            auto* child = xml.createNewChildElement ("PARAM");
            child->setAttribute ("id", ranged->paramID);
            child->setAttribute ("value", (double) ranged->convertFrom0to1 (ranged->getValue()));
        }

    // The recording is referenced by path as well as by index, so a user
    // sample survives being imported at a different position in the list.
    const auto index = (int) std::lround (apvts.getRawParameterValue (ids::sampleIndex)->load());
    xml.setAttribute ("samplePath", getSamplePath (index));
    xml.setAttribute ("sampleName", getSampleName (index));

    return presetFile (trimmed).replaceWithText (xml.toString());
}

bool SpampiModeAudioProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);

    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("SPAMPI-MODE-PRESET"))
        return false;

    // The preset says what the root is: do not let the measurement overwrite it.
    suppressAutoRoot = true;

    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
    {
        auto* parameter = apvts.getParameter (child->getStringAttribute ("id"));

        if (parameter == nullptr)
            continue;

        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    }

    // A user sample may have arrived at a different index: the path wins.
    const auto path = xml->getStringAttribute ("samplePath");

    if (path.isNotEmpty())
    {
        const auto index = getSampleIndexForPath (path);

        if (index >= 0)
            setParam (ids::sampleIndex, (float) index);
    }

    applySampleIndex();
    return true;
}

bool SpampiModeAudioProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

//==============================================================================
void SpampiModeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    engine.prepare (sampleRate);

    resampleAllTo (sampleRate);
    publishSamples();

    const auto size = juce::jmax (1, samplesPerBlock);
    monoInput.assign ((std::size_t) size, 0.0f);
    monoOutput.assign ((std::size_t) size, 0.0f);

    audioSample = nullptr;
    audioSampleIndex = -1;
}

void SpampiModeAudioProcessor::releaseResources()
{
    monoInput.clear();
    monoOutput.clear();
}

bool SpampiModeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input.isDisabled() || output.isDisabled())
        return false;

    // The guitar comes in, the animal goes out, and they are both mono or both
    // stereo: anything else and the dry signal would drift channels.
    return input == output && (input == juce::AudioChannelSet::mono()
                               || input == juce::AudioChannelSet::stereo());
}

void SpampiModeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || numChannels <= 0)
        return;

    // ---- pick the recording -------------------------------------------------
    const auto count = sampleCount.load (std::memory_order_acquire);

    auto want = (int) std::lround (apvts.getRawParameterValue (ids::sampleIndex)->load());

    if (count > 0)
        want = juce::jlimit (0, count - 1, want);

    auto* target = sampleSlots[(std::size_t) juce::jlimit (0, kMaxSamples - 1, want)]
                       .load (std::memory_order_acquire);

    // Comparing every block also picks up a slot that was empty when the
    // parameter changed and was filled a moment later.
    if (target != audioSample)
    {
        engine.setSample (target);
        audioSample = target;
        audioSampleIndex = want;
        currentSampleIndex.store (want, std::memory_order_relaxed);
    }

    // ---- the settings -------------------------------------------------------
    const auto param = [this] (const char* id)
    {
        return apvts.getRawParameterValue (id)->load();
    };

    spampi::Settings settings;
    settings.sampleIndex = audioSampleIndex;
    settings.root = param (ids::root);
    settings.follow = param (ids::follow);
    settings.interval = param (ids::interval);
    settings.spread = param (ids::spread);
    settings.glide = param (ids::glide);
    settings.drift = param (ids::drift);
    settings.machine = param (ids::machine);
    settings.triggerMode = (int) std::lround (param (ids::trigger));
    settings.sens = param (ids::sens);
    settings.tone = param (ids::tone);
    settings.mix = param (ids::mix);
    settings.level = param (ids::level);

    engine.setSettings (settings);

    // ---- mono in, mono out ---------------------------------------------------
    if ((int) monoInput.size() < numSamples)
    {
        monoInput.assign ((std::size_t) numSamples, 0.0f);
        monoOutput.assign ((std::size_t) numSamples, 0.0f);
    }

    const auto gain = 1.0f / (float) numChannels;

    std::fill (monoInput.begin(), monoInput.begin() + numSamples, 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* source = buffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            monoInput[(std::size_t) i] += source[i] * gain;
    }

    engine.process (monoInput.data(), monoOutput.data(), numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.copyFrom (ch, 0, monoOutput.data(), numSamples);

    // ---- the trace the panel draws -------------------------------------------
    {
        const juce::SpinLock::ScopedTryLockType lock (scopeLock);

        if (lock.isLocked())
        {
            const auto& snapshot = engine.getSnapshot();
            const auto note = snapshot.note.load (std::memory_order_relaxed);
            const auto cents = snapshot.cents.load (std::memory_order_relaxed);

            pitchHistory[historyPos] = note >= 0.0f ? note + cents / 100.0f : 0.0f;
            historyPos = (historyPos + 1) % kMaxPitchHistory;
            ++historyCounter;
        }
    }
}

//==============================================================================
void SpampiModeAudioProcessor::setParam (const char* id, float value)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void SpampiModeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            const auto index = (int) std::lround (apvts.getRawParameterValue (ids::sampleIndex)->load());
            xml->setAttribute ("samplePath", getSamplePath (index));
            xml->setAttribute ("sampleName", getSampleName (index));
            copyXmlToBinary (*xml, destData);
        }
}

void SpampiModeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            suppressAutoRoot = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            const auto path = xml->getStringAttribute ("samplePath");

            if (path.isNotEmpty())
            {
                const auto index = getSampleIndexForPath (path);

                if (index >= 0)
                    setParam (ids::sampleIndex, (float) index);
            }

            applySampleIndex();
        }
}

juce::AudioProcessorEditor* SpampiModeAudioProcessor::createEditor()
{
    return new SpampiModeAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpampiModeAudioProcessor();
}
