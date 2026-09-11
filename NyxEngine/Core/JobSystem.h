#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

// ============ JobSystem ============
//
// 简单的线程池，用于执行异步任务。
//
// 用法：
//   JobSystem jobs;
//   jobs.Initialize();   // 默认 CPU 核心数 - 1 个 worker
//
//   jobs.Submit([]() {
//       // 在后台线程执行
//   });
//
//   jobs.WaitAll();      // 等待所有任务完成
//   jobs.Shutdown();
//
// 设计：
//   - 实例化（不是单例），由 NyxEngine 持有
//   - 任务用 std::function<void()>，初期简单优先
//   - WaitAll 阻塞直到队列空 + 活跃任务为 0
//   - 线程安全
//   - 无异常、无 RTTI
//
// 限制：
//   - 不支持任务依赖（DAG）
//   - 不支持优先级
//   - 不支持取消单个任务
//   - 不支持任务返回值（用 std::promise 自己包装）

class JobSystem {
public:
    JobSystem() = default;
    ~JobSystem();

    // 禁止拷贝
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // 初始化线程池
    // threadCount = 0 时自动使用 (CPU 核心数 - 1)
    void Initialize(uint32_t threadCount = 0);

    // 关闭并等待所有 worker 退出
    void Shutdown();

    // 提交任务
    void Submit(std::function<void()> job);

    // 等待所有已提交任务完成（阻塞当前线程）
    void WaitAll();

    // ---------- 查询 ----------
    uint32_t GetWorkerCount() const { return workerCount_; }
    size_t GetPendingJobCount();
    size_t GetActiveJobCount() const { return activeJobCount_.load(); }

    bool IsInitialized() const { return initialized_; }

private:
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    mutable std::mutex mutex_;
    std::condition_variable jobAvailable_;
    std::condition_variable allDone_;

    std::atomic<bool> shutdown_{false};
    std::atomic<size_t> activeJobCount_{0};

    uint32_t workerCount_ = 0;
    bool initialized_ = false;
};