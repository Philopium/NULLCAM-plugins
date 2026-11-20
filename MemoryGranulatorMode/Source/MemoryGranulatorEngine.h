#pragma once

// MemoryGranulatorMode engine.
//
// The instrument is one loop, not a granulator followed by a reverb:
//
//   input --> [ circular memory ] --> [ grain cloud ] --> [ spatial field ]
//                 ^       |              ^     |               |
//                 |       +-- feedback --+     |               |
//                 +---------------------------+---------------+
//
// MIDI and audio pitch are separate control sources. The selected source is
// resolved into a musical control value, then a signed mapping matrix moves
// grain, harmonic and spatial parameters without ever averaging two pitch
// values blindly.
//
// This file contains no JUCE dependency so the complete engine can be tested
// without a host. The audio path owns fixed-size storage only: after prepare()
// there are no allocations, locks, strings or file operations.

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace memorygranulator
{

constexpr int kMaxGrains = 48;
constexpr int kMaxMidiNotes = 16;
constexpr int kWaveformPoints = 192;
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

enum class PitchSource   { midi = 0, audio, hybrid };
enum class NotePriority { latest = 0, lowest, highest, velocity };
enum class FollowMode   { follow = 0, harmonic, interval, resonant, loose, free };
enum class GrainMode    { uniform = 0, random, cluster, rhythmic, cloud, sparse, scan, pitchAware };
enum class Envelope     { hann = 0, exponential, triangle, plateau };
enum class MemoryMode   { live = 0, freeze, captureRelease, overdub, hold };
enum class FeedbackSource { input = 0, granular, reverb, combined, filtered };
enum class ModSource    { lfo = 0, envelope, input, velocity, aftertouch, pitch,
                           random, sampleHold, tempo, feedback };
enum class ModDestination { size = 0, density, position, rate, speed, pitch, spread,
                             reverbDecay, diffusion, feedback, filter, resonance };
enum class ScaleType    { chromatic = 0, major, minor, harmonicMinor, pentatonicMinor,
                           pentatonicMajor, dorian, mixolydian, lydian, wholeTone,
                           phrygian, blues };
enum class CurveType    { linear = 0, exponential, logarithmic };

struct Settings
{
    // ---- memory and evolution ----
    float memoryLength = 4.0f;           // seconds
    bool  captureEnable = true;
    bool  freeze = false;
    bool  captureArmed = false;
    bool  clearTrigger = false;
    bool  releaseTrigger = false;
    int   memoryMode = (int) MemoryMode::live;
    float recLevel = 1.0f;
    bool  stereoCapture = true;
    int   feedbackSource = (int) FeedbackSource::granular;
    float feedbackAmount = 0.0f;
    float feedbackStability = 0.65f;
    float feedbackFilter = 5000.0f;
    float feedbackDamping = 0.35f;
    float feedbackSaturation = 0.20f;
    float feedbackDiffusion = 0.25f;
    float feedbackPitch = 0.0f;          // semitones

    // ---- granular engine ----
    int   grainMode = (int) GrainMode::random;
    float grainSize = 0.12f;             // seconds
    float density = 8.0f;                // grains per second
    float overlap = 1.20f;
    float bufferPosition = 0.50f;
    float scanSpeed = 0.25f;             // memory positions per second
    float spray = 0.25f;
    float reverseProbability = 0.15f;
    int   grainEnvelope = (int) Envelope::hann;
    float grainGain = 0.55f;
    float stereoSpread = 0.70f;
    float stereoRandom = 0.35f;
    int   maxGrains = 24;
    bool  tempoSync = false;
    float hostRate = 1.0f;
    float hostBpm = 120.0f;              // supplied by the host transport

    // ---- pitch control ----
    int   pitchSource = (int) PitchSource::hybrid;
    int   midiChannel = 0;               // 0 = omni, 1..16 = channel
    int   releaseCC = 64;
    int   notePriority = (int) NotePriority::latest;
    int   followMode = (int) FollowMode::follow;
    float noteLow = 36.0f;
    float noteHigh = 96.0f;
    float trackRangeLow = 40.0f;          // Hz
    float trackRangeHigh = 1600.0f;       // Hz
    float transposition = 0.0f;          // semitones
    float pitchSensitivity = 1.0f;
    float confidenceThreshold = 0.55f;
    float pitchAttack = 20.0f;           // milliseconds
    float pitchRelease = 180.0f;         // milliseconds
    float pitchSmoothing = 40.0f;        // milliseconds
    float hybridBlend = 0.50f;

    // Signed matrix: polarity is the sign, magnitude is the range.
    float mapTranspose = 0.35f;
    float mapSpeed = 0.15f;
    float mapPosition = 0.20f;
    float mapDensity = 0.15f;
    float mapSize = 0.25f;
    float mapRate = 0.00f;
    float mapHarmonic = 0.00f;
    float mapResonant = 0.00f;
    float mapReverb = 0.00f;
    float mapStereo = 0.15f;
    float mapSmoothing = 80.0f;
    int   mapCurve = (int) CurveType::linear;

    // ---- harmonic organisation ----
    float fundamental = 0.45f;
    float octaveMix = 0.25f;
    int   harmonicLayers = 2;
    float intervalA = 7.0f;
    float intervalB = 12.0f;
    float harmonicSpread = 0.50f;
    bool  quantize = false;
    int   scale = (int) ScaleType::chromatic;
    float rootNote = 0.0f;
    float tuning = 0.0f;                 // cents
    float detune = 4.0f;                 // cents
    float harmLevel1 = 1.00f;
    float harmLevel2 = 0.55f;
    float harmLevel3 = 0.30f;
    float harmLevel4 = 0.15f;

    // ---- spatial engine ----
    float reverbMix = 0.35f;
    float reverbDecay = 2.80f;
    float roomSize = 0.55f;
    float diffusion = 0.65f;
    float damping = 0.30f;
    float preDelay = 0.015f;
    float width = 0.80f;
    float spaceModulation = 0.35f;
    float filterFreq = 8000.0f;
    float resonance = 0.20f;
    float reverbFeedback = 0.20f;
    bool  reverbFreeze = false;

    // ---- modulation matrix ----
    int   modSource = (int) ModSource::lfo;
    int   modDestination = (int) ModDestination::density;
    float modDepth = 0.0f;
    int   modSource2 = -1;
    int   modDestination2 = (int) ModDestination::size;
    float modDepth2 = 0.0f;
    float modRate = 0.40f;

    // ---- output ----
    float dryWet = 0.35f;
    float grainLevel = 0.80f;
    float reverbLevel = 0.70f;
    float inputGain = 1.0f;
    float outputGain = 0.80f;
};

enum class MidiKind { noteOn = 0, noteOff, controller, aftertouch, pitchWheel };

struct MidiEvent
{
    MidiKind kind = MidiKind::controller;
    int channel = 0;
    int note = 60;
    float value = 0.0f;                  // velocity, controller value or wheel
    int controller = 0;
};

struct Snapshot
{
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> grainLevel { 0.0f };
    std::atomic<float> reverbLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> feedbackLevel { 0.0f };
    std::atomic<float> trackedHz { 0.0f };
    std::atomic<float> confidence { 0.0f };
    std::atomic<float> controlPitch { 0.0f };
    std::atomic<float> playhead { 0.0f };
    std::atomic<float> modulationValue { 0.0f };
    std::atomic<float> velocity { 0.0f };
    std::atomic<float> aftertouch { 0.0f };
    std::atomic<int>   activeGrains { 0 };
    std::atomic<int>   activeNotes { 0 };
    std::atomic<int>   controlSource { 0 };
    std::atomic<float> grainPosition[kMaxGrains] {};
    std::atomic<float> grainLife[kMaxGrains] {};
    std::atomic<float> waveform[kWaveformPoints] {};
    std::atomic<int>   midiNotes[8] {};
};

class Engine
{
public:
    Engine();
    ~Engine();

    Engine (const Engine&) = delete;
    Engine& operator= (const Engine&) = delete;

    void prepare (double sampleRate);
    void reset() noexcept;
    void setSettings (const Settings& newSettings) noexcept { settings = newSettings; }
    void handleMidi (const MidiEvent& event) noexcept;

    void process (const float* const* input, float* const* output,
                  int numChannels, int numSamples) noexcept;

    const Snapshot& getSnapshot() const noexcept { return snapshot; }

private:
    struct Grain
    {
        bool active = false;
        double position = 0.0;
        double age = 0.0;
        double duration = 0.0;
        float pitchRatio = 1.0f;
        float gain = 1.0f;
        float pan = 0.0f;
        int direction = 1;
        int envelope = 0;
        int harmonic = 0;
    };

    struct MidiVoice
    {
        bool active = false;
        bool released = false;
        int note = 60;
        float velocity = 0.0f;
        std::uint32_t age = 0;
    };

    struct Smooth
    {
        float value = 0.0f;
        float coefficient = 0.01f;

        void setup (double sampleRate, double milliseconds) noexcept;
        float next (float target) noexcept;
        void snap (float target) noexcept { value = target; }
    };

    struct PeakFilter
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        void setParameters (double sampleRate, double frequency, double resonance) noexcept;
        float process (float x) noexcept;
        void reset() noexcept;
    };

    struct DCBlocker
    {
        float lastIn = 0.0f;
        float lastOut = 0.0f;

        float process (float x) noexcept;
        void reset() noexcept;
    };

    class PitchTracker;
    class Reverb;

    float random() noexcept;
    float smoothRandom() noexcept;
    float chooseEnvelope (int shape, float phase) const noexcept;
    float scaleFrequency (float frequency) const noexcept;
    float mapDepth (float depth, float position, float range) const noexcept;
    float modulationSource (int source) noexcept;
    float modulationDestinationValue (int destination, float modulation) const noexcept;
    float modulationContribution (int destination, float lane1, float lane2) const noexcept;
    int  chooseMidiNote() const noexcept;
    int  chooseHarmonicLayer(float pitchPosition) noexcept;
    float harmonicRatioForLayer (int layer, float pitchPosition) const noexcept;
    float harmonicLayerGain (int layer) const noexcept;
    float controlPitchRatio (float pitchPosition, int* selectedHarmonic, int forcedLayer = -1) noexcept;
    bool  hasMidiForGrain() const noexcept;
    float readMemory (double position, bool rightChannel) const noexcept;
    void spawnGrain (float pitchPosition, float mappedStereo, float currentDensity,
                     float currentSize, float currentPosition, float currentSpeed) noexcept;
    void updateSnapshot() noexcept;
    bool shouldCapture() const noexcept;
    float effectivePlaybackSpeed() const noexcept;

    Settings settings;
    Snapshot snapshot;
    double sampleRate = 48000.0;
    bool prepared = false;

    // ---- circular memory ----
    std::vector<float> memoryLeft;
    std::vector<float> memoryRight;
    std::size_t memoryCapacity = 0;
    std::size_t writePosition = 0;
    std::size_t logicalLength = 0;
    std::size_t capturedSamples = 0;
    std::size_t feedbackWritePosition = 0;
    float memoryLevel = 0.0f;

    // ---- grains and transport ----
    std::array<Grain, kMaxGrains> grains {};
    double playhead = 0.50;
    double triggerPhase = 0.0;
    double tempoPhase = 0.0;
    double lfoPhase = 0.0;
    double sampleHoldPhase = 0.0;
    float randomState = 0.0f;
    float sampleHoldValue = 0.0f;
    std::uint32_t randomSeed = 0x9e3779b9u;

    // ---- MIDI state ----
    std::array<MidiVoice, kMaxMidiNotes> voices {};
    std::uint32_t voiceAge = 0;
    float velocity = 0.0f;
    float aftertouch = 0.0f;
    float modWheel = 0.0f;
    float pitchWheel = 0.0f;
    bool pendingRelease = false;

    // ---- pitch state ----
    std::unique_ptr<PitchTracker> pitchTracker;
    float trackedHz = 0.0f;
    float trackedConfidence = 0.0f;
    float smoothedPitchHz = 0.0f;
    float smoothedAudioPitchHz = 0.0f;
    float pitchPosition = 0.5f;
    float controlPitchSemitones = 0.0f;
    Smooth pitchSmoother;
    Smooth attackSmoother;
    Smooth releaseSmoother;
    Smooth mapSmoothers[10];

    // ---- filtering and feedback ----
    float feedbackFilterStateL = 0.0f;
    float feedbackFilterStateR = 0.0f;
    float feedbackDampL = 0.0f;
    float feedbackDampR = 0.0f;
    std::vector<float> feedbackHistoryL;
    std::vector<float> feedbackHistoryR;
    std::size_t feedbackHistorySize = 0;
    std::size_t feedbackHistoryIndex = 0;
    float inputEnvelope = 0.0f;
    float feedbackEnvelope = 0.0f;
    DCBlocker feedbackDcL;
    DCBlocker feedbackDcR;
    float modulationValue = 0.0f;
    float modulationValue2 = 0.0f;

    // ---- spatial engine ----
    std::unique_ptr<Reverb> reverb;
    PeakFilter spatialFilterL;
    PeakFilter spatialFilterR;
    PeakFilter feedbackPeakFilterL;
    PeakFilter feedbackPeakFilterR;

    // ---- trigger edges ----
    bool previousClear = false;
    bool previousRelease = false;
    bool previousCaptureArmed = false;
    bool captureReleased = false;
};

} // namespace memorygranulator
