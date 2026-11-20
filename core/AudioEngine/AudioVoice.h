#ifndef AUDIOENGINE_AUDIOVOICE_H_
#define AUDIOENGINE_AUDIOVOICE_H_
#include "Audio.h"

class AudioStereoMirror : public AudioStream
{
  public:
    AudioStereoMirror()
      : AudioStream(2, inputQueueArray)
    {
    }

    void setStereo(bool stereo) { this->stereo = stereo; }
    bool isStereo() const { return stereo; }
    virtual void update() override;

  private:
    audio_block_t* inputQueueArray[2];
    bool stereo = false;
};

class AudioVoice
{
  public:
    enum EnvelopeGeneratorType
    {
        kFilter,
        kWavetablePosition,
        kGranularLength = kWavetablePosition,
        kGranularPosition,
        kPanning,
        kFinetune,
        kEnvelopeGeneratorCount
    };

    enum VoiceChannels
    {
        kLeft,
        kRight,
        kChannelsCount
    };

    AudioVoice();
    void setStereo(bool stereo) { stereoMirror.setStereo(stereo); }
    bool isStereo() const { return stereoMirror.isStereo(); }
    AudioStereoMirror* getStereoMirror() { return &stereoMirror; }
    AudioPlayMemory* getMemoryPlayer(VoiceChannels channel);
    AudioBitDepth* getBitDepth(VoiceChannels channel);
    AudioFilterStateVariable* getFilter(VoiceChannels channel);
    AudioAmplifier* getAmplifier(VoiceChannels channel);
    AudioAmplifier* getPanningAmplifier(VoiceChannels channel);
    AudioAnalyzeRMS* getRMS(VoiceChannels channel);
    envelopeGenerator* getEnvelopeGenerator(EnvelopeGeneratorType type);
#ifdef MOD_SAMPLE_INSTRUMENT_FX
    // One processor per voice with paired L/R channels (index 0 = left, 1 = right).
    AudioSampleRateReduction* getSampleRateReduction() { return &sampleRateReduction; }
    AudioFilterDJ* getDjFilter() { return &djFilter; }
#endif

  private:
    AudioPlayMemory memoryPlayer[VoiceChannels::kChannelsCount];
    AudioBitDepth bitDepth[VoiceChannels::kChannelsCount];
    AudioFilterStateVariable filter[VoiceChannels::kChannelsCount];
#ifdef MOD_SAMPLE_INSTRUMENT_FX
    // Declared (= constructed = updated) in signal order: filter -> SRR -> DJ -> amplifier.
    AudioSampleRateReduction sampleRateReduction{ VoiceChannels::kChannelsCount };
    AudioFilterDJ djFilter{ VoiceChannels::kChannelsCount };
#endif
    AudioAmplifier amplifier[VoiceChannels::kChannelsCount];
    AudioStereoMirror stereoMirror;
    AudioAmplifier panningAmplifier[VoiceChannels::kChannelsCount];
    AudioAnalyzeRMS rms[VoiceChannels::kChannelsCount];

    envelopeGenerator envelope[kEnvelopeGeneratorCount];
    std::vector<AudioConnection*> connections;
};

#endif /* AUDIOENGINE_AUDIOVOICE_H_ */
