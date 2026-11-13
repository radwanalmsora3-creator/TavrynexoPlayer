#ifndef NOMINMAX
#define NOMINMAX
#endif
#define MINIAUDIO_IMPLEMENTATION
#include "core/AudioEngine.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace Tavrynexo::Core {

AudioEngine::AudioEngine(std::shared_ptr<ClockSync> clockSync)
    : m_clockSync(std::move(clockSync)) 
{}

AudioEngine::~AudioEngine() {
    stop();
    if (m_isInitialized) {
        ma_device_uninit(&m_device);
    }
}

bool AudioEngine::init() {
    if (m_isInitialized) return true;

    m_deviceConfig = ma_device_config_init(ma_device_type_playback);
    m_deviceConfig.playback.format   = ma_format_f32;
    m_deviceConfig.playback.channels = 2;
    m_deviceConfig.sampleRate        = 48000;
    m_deviceConfig.dataCallback      = AudioEngine::dataCallback;
    m_deviceConfig.pUserData         = this;

    if (ma_device_init(nullptr, &m_deviceConfig, &m_device) != MA_SUCCESS) {
        std::cerr << "[Tavrynexo::AudioEngine] Failed to initialize miniaudio playback device" << std::endl;
        return false;
    }

    m_isInitialized = true;
    return true;
}

void AudioEngine::start() {
    if (!m_isInitialized && !init()) return;
    ma_device_start(&m_device);
}

void AudioEngine::pause() {
    if (m_isInitialized) {
        ma_device_stop(&m_device);
    }
}

void AudioEngine::stop() {
    pause();
    clearBuffer();
}

void AudioEngine::clearBuffer() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_audioBuffer.clear();
    m_ptsMarks.clear();
}

void AudioEngine::pushSamples(const std::vector<float>& samples, double pts) {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    size_t currentSampleCount = m_audioBuffer.size();
    m_ptsMarks.emplace_back(currentSampleCount, pts);
    m_audioBuffer.insert(m_audioBuffer.end(), samples.begin(), samples.end());
}

void AudioEngine::setVolume(float volume) {
    m_volume.store(std::clamp(volume, 0.0f, 2.0f));
}

void AudioEngine::dataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
    auto* self = static_cast<AudioEngine*>(pDevice->pUserData);
    if (!self) return;

    float* out = static_cast<float*>(pOutput);
    size_t requestedSamples = frameCount * 2; // Stereo

    std::lock_guard<std::mutex> lock(self->m_bufferMutex);
    size_t availableSamples = self->m_audioBuffer.size();
    size_t samplesToRead = (std::min)(requestedSamples, availableSamples);

    float vol = self->m_isMuted.load() ? 0.0f : self->m_volume.load();

    for (size_t i = 0; i < samplesToRead; ++i) {
        out[i] = self->m_audioBuffer.front() * vol;
        self->m_audioBuffer.pop_front();
    }

    // إذا لم تكن هناك عينات كافية، املأ الفراغ بالصمت لمنع حدوث فرقعة صوتية (Audio Pop/Click)
    if (samplesToRead < requestedSamples) {
        std::memset(out + samplesToRead, 0, (requestedSamples - samplesToRead) * sizeof(float));
    }

    // تحديث ساعة المزامنة (Audio Clock Sync)
    while (!self->m_ptsMarks.empty() && self->m_ptsMarks.front().first < samplesToRead) {
        if (self->m_clockSync) {
            self->m_clockSync->updateAudioClock(self->m_ptsMarks.front().second);
        }
        self->m_ptsMarks.pop_front();
    }

    for (auto& mark : self->m_ptsMarks) {
        if (mark.first >= samplesToRead) {
            mark.first -= samplesToRead;
        }
    }
}

} // namespace Tavrynexo::Core
