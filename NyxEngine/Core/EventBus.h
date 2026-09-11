#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <utility>

// ============ EventBus ============
//
// 类型安全的事件总线。
//
// 用法：
//   struct TimeChangedEvent { float timeOfDay; };
//
//   // 订阅
//   auto handle = EventBus::Subscribe<TimeChangedEvent>([](const TimeChangedEvent& e) {
//       NYX_LOG_INFO("time changed to %f", e.timeOfDay);
//   });
//
//   // 发出
//   EventBus::Emit(TimeChangedEvent{0.5f});
//
//   // 取消订阅
//   EventBus::Unsubscribe(handle);
//
// 设计：
//   - 同步派发（emit 时立即调用所有订阅者）
//   - emit 零分配
//   - emit 时对订阅列表做快照，回调内可以安全地订阅/取消订阅
//   - 线程安全
//   - 无 RTTI，无异常
//
// 限制：
//   - 不支持异步派发（用 JobSystem 自己做）
//   - 不支持返回值

class EventBus {
public:
    using Handle = uint32_t;
    static constexpr Handle kInvalidHandle = 0;

    // 订阅事件。返回句柄用于取消订阅
    template<typename T, typename F>
    static Handle Subscribe(F&& callback);

    // 取消订阅。句柄无效时安全忽略
    static void Unsubscribe(Handle handle);

    // 发出事件。同步调用所有订阅者
    template<typename T>
    static void Emit(const T& event);

    // 清空所有订阅（退出时调用）
    static void Clear();

    // 当前订阅总数（调试用）
    static size_t GetSubscriptionCount();

private:
    struct Entry {
        Handle handle;
        std::function<void(const void*)> callback;
    };

    using EntryList = std::vector<Entry>;

    static Handle SubscribeImpl(const void* typeId, std::function<void(const void*)> cb);
    static void EmitImpl(const void* typeId, const void* event);

    static std::unordered_map<const void*, std::shared_ptr<EntryList>> s_subscriptions;
    static std::mutex s_mutex;
    static Handle s_nextHandle;
};

// ============ 类型 ID ============
//
// 用模板静态 inline 变量的地址作为类型 ID。
// C++17 保证 inline 变量在所有 TU 中地址唯一。

template<typename T>
struct EventTypeId {
    inline static const char value = 0;
};

// ============ 模板实现 ============

template<typename T, typename F>
EventBus::Handle EventBus::Subscribe(F&& callback) {
    return SubscribeImpl(
        &EventTypeId<T>::value,
        [cb = std::forward<F>(callback)](const void* event) {
            cb(*static_cast<const T*>(event));
        }
    );
}

template<typename T>
void EventBus::Emit(const T& event) {
    EmitImpl(&EventTypeId<T>::value, &event);
}