#include "JobSystem.h"
#include "Logger.h"
#include "Platform.h"

JobSystem::~JobSystem() {
    if (initialized_) {
        Shutdown();
    }
}

void JobSystem::Initialize(uint32_t threadCount) {
    if (initialized_) {
        NYX_LOG_WARN("JobSystem: already initialized");
        return;
    }

    // 默认线程数：CPU 核心数 - 1（保留主线程）
    if (threadCount == 0) {
        uint32_t cores = Platform::GetCPUCoreCount();
        threadCount = (cores > 1) ? (cores - 1) : 1;
    }

    shutdown_ = false;
    activeJobCount_ = 0;
    workerCount_ = threadCount;
    initialized_ = true;

    workers_.reserve(threadCount);
    for (uint32_t i = 0; i < threadCount; i++) {
        workers_.emplace_back([this]() { WorkerLoop(); });
    }

    NYX_LOG_INFO("JobSystem: initialized with %u workers", threadCount);
}

void JobSystem::Shutdown() {
    if (!initialized_) return;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    jobAvailable_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    // 清理队列
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!jobs_.empty()) {
            jobs_.pop();
        }
    }

    workerCount_ = 0;
    initialized_ = false;
    shutdown_ = false;
    activeJobCount_ = 0;

    NYX_LOG_INFO("JobSystem: shutdown");
}

void JobSystem::Submit(std::function<void()> job) {
    if (!initialized_) {
        NYX_LOG_ERROR("JobSystem: not initialized, rejecting job");
        return;
    }

    if (!job) {
        NYX_LOG_WARN("JobSystem: null job submitted, ignoring");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.push(std::move(job));
    }
    jobAvailable_.notify_one();
}

void JobSystem::WaitAll() {
    if (!initialized_) return;

    std::unique_lock<std::mutex> lock(mutex_);
    allDone_.wait(lock, [this]() {
        return jobs_.empty() && activeJobCount_.load() == 0;
    });
}

size_t JobSystem::GetPendingJobCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.size();
}

void JobSystem::WorkerLoop() {
    while (true) {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            jobAvailable_.wait(lock, [this]() {
                return shutdown_.load() || !jobs_.empty();
            });

            // 退出条件：已关闭 且 队列为空
            if (shutdown_.load() && jobs_.empty()) {
                return;
            }

            job = std::move(jobs_.front());
            jobs_.pop();
            activeJobCount_.fetch_add(1);
        }

        // 执行任务（不在锁内）
        job();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            activeJobCount_.fetch_sub(1);

            // 队列空 + 没有活跃任务 → 通知 WaitAll
            if (jobs_.empty() && activeJobCount_.load() == 0) {
                allDone_.notify_all();
            }
        }
    }
}