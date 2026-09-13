#include "AudioClock.h"
#include <algorithm>
#include <cmath>

void AudioClock::Start(double startSeconds) {
    time_ = std::max(0.0, startSeconds);
    if (duration_ > 0.0) {
        time_ = std::min(time_, duration_);
    }
    state_ = State::Playing;
}

void AudioClock::Stop() {
    state_ = State::Stopped;
    time_ = 0.0;
}

void AudioClock::Pause() {
    if (state_ == State::Playing) {
        state_ = State::Paused;
    }
}

void AudioClock::Resume() {
    if (state_ == State::Paused) {
        state_ = State::Playing;
    }
}

void AudioClock::Tick(double dtSeconds) {
    if (state_ != State::Playing) return;
    if (dtSeconds <= 0.0) return;

    time_ += dtSeconds * playbackRate_;

    // ---------- 循环 ----------
    if (loopEnabled_) {
        double effectiveEnd = (loopEnd_ > loopBegin_) ? loopEnd_ : duration_;
        if (effectiveEnd > loopBegin_ && time_ >= effectiveEnd) {
            double loopLen = effectiveEnd - loopBegin_;
            time_ = loopBegin_ + std::fmod(time_ - loopBegin_, loopLen);
        }
        return;
    }

    // ---------- 到达终点 ----------
    if (duration_ > 0.0 && time_ >= duration_) {
        time_ = duration_;
        state_ = State::Stopped;
        if (endCallback_) {
            endCallback_();
        }
    }
}

int64_t AudioClock::GetTimeSamples(int sampleRate) const {
    if (sampleRate <= 0) return 0;
    return static_cast<int64_t>(std::llround(time_ * static_cast<double>(sampleRate)));
}

void AudioClock::Seek(double seconds) {
    time_ = std::max(0.0, seconds);
    if (duration_ > 0.0) {
        time_ = std::min(time_, duration_);
    }
}

void AudioClock::SetPlaybackRate(double rate) {
    if (rate <= 0.0) return;
    playbackRate_ = rate;
}

void AudioClock::SetLoop(bool enabled, double loopBegin, double loopEnd) {
    loopEnabled_ = enabled;
    if (enabled) {
        loopBegin_ = std::max(0.0, loopBegin);
        loopEnd_ = std::max(0.0, loopEnd);
    } else {
        loopBegin_ = 0.0;
        loopEnd_ = 0.0;
    }
}