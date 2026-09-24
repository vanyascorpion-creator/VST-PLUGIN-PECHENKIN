#pragma once

#include <JuceHeader.h>
#include <atomic>

class MySynthAudioProcessor : public juce::AudioProcessor
{
public:
    MySynthAudioProcessor();
    ~MySynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index,
        const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data,
        int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    class SynthSound : public juce::SynthesiserSound
    {
    public:
        bool appliesToNote(int) override
        {
            return true;
        }

        bool appliesToChannel(int) override
        {
            return true;
        }
    };

    class SynthVoice : public juce::SynthesiserVoice
    {
    public:
        bool canPlaySound(juce::SynthesiserSound* sound) override
        {
            return dynamic_cast<SynthSound*>(sound) != nullptr;
        }

        void setParameterPointers(
            std::atomic<float>* newOscillator1Level,
            std::atomic<float>* newOscillator2Level,
            std::atomic<float>* newDetune,
            std::atomic<float>* newWaveform1,
            std::atomic<float>* newWaveform2,
            std::atomic<float>* newAttack,
            std::atomic<float>* newDecay,
            std::atomic<float>* newSustain,
            std::atomic<float>* newRelease,

            // Параметры фильтра:
            std::atomic<float>* newCutoff,
            std::atomic<float>* newResonance);

        void startNote(int midiNoteNumber,
            float velocity,
            juce::SynthesiserSound*,
            int) override;

        void stopNote(float,
            bool allowTailOff) override;

        void pitchWheelMoved(int) override
        {
        }

        void controllerMoved(int, int) override
        {
        }

        void renderNextBlock(
            juce::AudioBuffer<float>& outputBuffer,
            int startSample,
            int numSamples) override;

        void prepare(double newSampleRate);

    private:
        double sampleRate = 44100.0;

        double phase1 = 0.0;
        double phase2 = 0.0;

        double phaseIncrement1 = 0.0;
        double phaseIncrement2 = 0.0;

        float level = 0.0f;
        bool isPlaying = false;

        std::atomic<float>* oscillator1Level = nullptr;
        std::atomic<float>* oscillator2Level = nullptr;
        std::atomic<float>* detune = nullptr;
        std::atomic<float>* waveform1 = nullptr;
        std::atomic<float>* waveform2 = nullptr;
        std::atomic<float>* attack = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* sustain = nullptr;
        std::atomic<float>* release = nullptr;
        // Указатели на параметры фильтра.
// Значения меняются из интерфейса и читаются аудиопотоком.
        std::atomic<float>* cutoff = nullptr;
        std::atomic<float>* resonance = nullptr;

        juce::ADSR adsr;
        juce::ADSR::Parameters adsrParameters;
        // Встроенный JUCE low-pass-фильтр для отдельного голоса.
        juce::IIRFilter filter;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    juce::Synthesiser synthesiser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MySynthAudioProcessor)
};