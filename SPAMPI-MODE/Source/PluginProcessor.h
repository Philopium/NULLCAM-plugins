#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "SpampiEngine.h"

#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace ids
{
    inline constexpr const char* sampleIndex = "sampleIndex";
    inline constexpr const char* root       = "root";
    inline constexpr const char* follow     = "follow";
    inline constexpr const char* interval   = "interval";
    inline constexpr const char* spread     = "spread";
    inline constexpr const char* glide      = "glide";
    inline constexpr const char* drift      = "drift";
    inline constexpr const char* machine    = "machine";
    inline constexpr const char* trigger    = "trigger";
    inline constexpr const char* sens       = "sens";
    inline constexpr const char* tone       = "tone";
    inline constexpr const char* mix        = "mix";
    inline constexpr const char* level      = "level";
} // namespace ids

class SpampiModeAudioProcessor : public juce::AudioProcessor
{
public:
    SpampiModeAudioProcessor();
    ~SpampiModeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SPAMPI-MODE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The silly factory voices. Each one picks a recorded sample and its own
    // machine settings.
    enum class Preset
    {
        monkey = 0, ambulance, baby, cat, dog, toilet,
        train, laugh, elephant, cow, rooster, spam
    };

    static constexpr int kNumPresets = 12;

    void applyPreset (Preset preset);
    juce::String matchFactoryPreset() const;
    static juce::String getFactoryPresetName (Preset preset);

    // User presets are one XML file each in the user application data folder.
    static juce::File getUserPresetFolder();
    static juce::File getUserSampleFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    // Samples: the bundled recordings come first, then whatever the user has
    // dropped into the user sample folder.
    int getNumSamples() const noexcept { return (int) samples.size(); }
    juce::String getSampleName (int index) const;
    juce::StringArray getSampleNames() const;
    int getSampleIndexForPath (const juce::String& path) const;
    juce::String getSamplePath (int index) const;
    int getSampleIndexForName (const juce::String& name) const;

    // The note the recording was measured at, or -1 when it has no pitch.
    int getSampleMeasuredMidi (int index) const;

    // Copies a file into the user sample folder and adds it to the list.
    // Returns the new index, or -1 on failure.
    int importUserSample (const juce::File& source);
    void refreshUserSamples();

    // Called by the panel when the player picks a recording: the root snaps to
    // the pitch the recording was measured at, so the machine starts in tune.
    void sampleChosenByUser();

    // A decimated peek at the selected recording, for the waveform display.
    bool copySampleWaveform (float* dest, int count) const;

    // Where the selected recording is cut, as a 0..1 position, so the panel can
    // draw the peak the note attack lands on.
    float getSampleStartFraction() const;

    // Display only: the pitch history the panel draws as a trace.
    bool copyPitchHistory (float* dest, int count) const;

    // The root follows the recording: when the sample changes, the new root is
    // handed over to the message thread, which writes it into the parameter.
    bool consumePendingAutoRoot();

    juce::AudioProcessorValueTreeState apvts;

    const spampi::Snapshot& getSnapshot() const noexcept { return engine.getSnapshot(); }
    int getCurrentSampleIndex() const noexcept { return currentSampleIndex.load(); }

private:
    struct SampleEntry;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float value);

    void loadBundledSamples();
    bool loadSampleFromFile (const juce::File& file, const juce::String& displayName);
    bool loadSampleFromMemory (const char* data, int size, const juce::String& displayName);
    bool decodeToMono (juce::AudioFormatReader* reader, std::vector<float>& out, double& rate);
    void applySampleIndex();
    void publishSamples();
    int measuredMidiOf (const SampleEntry& entry) const;
    void resampleAllTo (double sampleRate);

    spampi::Engine engine;

    struct SampleEntry
    {
        spampi::Sample sample;
        juce::String path;          // empty for the bundled ones
        juce::String displayName;
        std::vector<float> rawData; // at rawRate, before the host rate conversion
        double rawRate = 48000.0;
        int measuredMidi = -1;
        bool isUser = false;
    };

    // The entries are never moved or deleted while the plugin runs: the audio
    // thread holds a raw pointer to the sample it is playing.
    std::vector<std::unique_ptr<SampleEntry>> samples;

    // The audio thread never touches the list itself, only these slots. The
    // message thread publishes a pointer per sample, so adding a recording
    // while the plugin is playing can never make the audio thread read a
    // vector that is being resized.
    static constexpr int kMaxSamples = 64;
    std::array<std::atomic<spampi::Sample*>, kMaxSamples> sampleSlots {};
    std::atomic<int> sampleCount { 0 };

    // Held while the list itself is changed: loading, importing, resampling.
    mutable juce::SpinLock sampleLock;

    std::atomic<int> requestedIndex { 0 };
    std::atomic<int> currentSampleIndex { -1 };
    std::atomic<float> pendingAutoRoot { -1.0f };

    // Message thread only: set while a preset or a session is being restored,
    // so the root a preset asked for is not overwritten by the measurement.
    bool suppressAutoRoot = false;

    // Audio thread only: the sample the engine is actually playing.
    int audioSampleIndex = -1;
    const spampi::Sample* audioSample = nullptr;

    double hostSampleRate = 48000.0;
    std::vector<float> monoInput, monoOutput;
    juce::StringArray loadedUserPaths;

    mutable juce::SpinLock scopeLock;
    std::array<float, 256> pitchHistory {};
    std::size_t historyPos = 0;
    int historyCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpampiModeAudioProcessor)
};
