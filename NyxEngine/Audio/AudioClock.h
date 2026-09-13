#pragma once
#include <cstdint>
#include <functional>

// ============ AudioClock ============
//
// 音频时间轴的管理者。不负责播放声音——只负责"现在到几秒了"。
//
// 设计：
//   - 时钟由外部驱动（Tick(dt)），不主动读系统时间
//     这样时钟和渲染天然同步，不会漂移
//   - 支持播放/暂停/停止/跳转/变速/循环
//   - 提供秒、采样两种时间单位
//   - 可选结束回调
//
// 使用：
//   AudioClock clock;
//   clock.SetDuration(180.0);     // 3 分钟
//   clock.Start();
//   while (running) {
//       float dt = ...;
//       clock.Tick(dt);
//       double t = clock.GetTimeSeconds();
//       // 用 t 查询谱面、驱动渲染
//   }
//
// 未来扩展（不在此类中）：
//   - AudioSystem：实际的音频播放（WASAPI / miniaudio）
//   - 反向同步：AudioSystem 用播放位置修正 AudioClock 的漂移

class AudioClock {
public:
    enum class State {
        Stopped,   // 未播放，时间为 0
        Playing,   // 正在推进
        Paused     // 暂停，保留时间
    };

    // ---------- 生命周期 ----------
    void Start(double startSeconds = 0.0);
    void Stop();
    void Pause();
    void Resume();

    // 每帧调用，推进时钟
    // dtSeconds <= 0 时不做任何事
    void Tick(double dtSeconds);

    // ---------- 状态查询 ----------
    State GetState() const { return state_; }
    bool IsPlaying() const { return state_ == State::Playing; }
    bool IsPaused() const { return state_ == State::Paused; }
    bool IsStopped() const { return state_ == State::Stopped; }

    // ---------- 时间查询 ----------
    double GetTimeSeconds() const { return time_; }

    // 返回当前时间对应的采样索引（四舍五入到最近采样）
    int64_t GetTimeSamples(int sampleRate) const;

    // 当前帧对应的音频时间。
    // 现在等同于 GetTimeSeconds()。将来接入音频后端后，
    // 如果需要区分"帧的时间点"和"渲染时的实际时间"，会在这里实现。
    double GetFrameTime() const { return time_; }

    // ---------- 时长 ----------
    void SetDuration(double seconds) { duration_ = seconds; }
    double GetDurationSeconds() const { return duration_; }

    // ---------- 跳转 ----------
    // 跳转到指定秒数。会被夹在 [0, duration]
    void Seek(double seconds);

    // ---------- 变速 ----------
    // rate <= 0 会被忽略（不允许时钟停止或倒退）
    void SetPlaybackRate(double rate);
    double GetPlaybackRate() const { return playbackRate_; }

    // ---------- 循环 ----------
    // enabled = false 时清除循环
    // enabled = true 时：
    //   - 若 loopEnd > loopBegin：循环区间为 [loopBegin, loopEnd]
    //   - 否则：循环区间为 [loopBegin, duration]
    void SetLoop(bool enabled, double loopBegin = 0.0, double loopEnd = 0.0);
    bool IsLoopEnabled() const { return loopEnabled_; }
    double GetLoopBegin() const { return loopBegin_; }
    double GetLoopEnd() const { return loopEnd_; }

    // ---------- 回调 ----------
    // 非循环模式下，到达 duration 时调用一次
    // 注意：回调在 Tick 内同步执行，不要在里面做重活
    using EndCallback = std::function<void()>;
    void SetEndCallback(EndCallback cb) { endCallback_ = std::move(cb); }

private:
    State state_ = State::Stopped;
    double time_ = 0.0;
    double duration_ = 0.0;
    double playbackRate_ = 1.0;

    bool loopEnabled_ = false;
    double loopBegin_ = 0.0;
    double loopEnd_ = 0.0;

    EndCallback endCallback_;
};