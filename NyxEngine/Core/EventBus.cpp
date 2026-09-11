#include "EventBus.h"
#include <algorithm>

std::unordered_map<const void*, std::shared_ptr<EventBus::EntryList>> EventBus::s_subscriptions;
std::mutex EventBus::s_mutex;
EventBus::Handle EventBus::s_nextHandle = 1;

EventBus::Handle EventBus::SubscribeImpl(const void* typeId, std::function<void(const void*)> cb) {
    std::lock_guard<std::mutex> lock(s_mutex);

    Handle handle = s_nextHandle++;

    auto& sharedList = s_subscriptions[typeId];
    if (!sharedList) {
        sharedList = std::make_shared<EntryList>();
    }

    // Copy-on-write：拷贝一份，加新元素，替换
    // 这样正在遍历旧列表的 Emit 不受影响
    auto newList = std::make_shared<EntryList>(*sharedList);
    newList->push_back({handle, std::move(cb)});
    sharedList = std::move(newList);

    return handle;
}

void EventBus::Unsubscribe(Handle handle) {
    if (handle == kInvalidHandle) return;

    std::lock_guard<std::mutex> lock(s_mutex);

    for (auto& pair : s_subscriptions) {
        auto& sharedList = pair.second;
        if (!sharedList) continue;

        auto& list = *sharedList;
        auto it = std::find_if(list.begin(), list.end(),
            [handle](const Entry& e) { return e.handle == handle; });

        if (it != list.end()) {
            auto newList = std::make_shared<EntryList>();
            newList->reserve(list.size() - 1);
            for (auto& e : list) {
                if (e.handle != handle) newList->push_back(e);
            }
            sharedList = std::move(newList);
            return;
        }
    }
}

void EventBus::EmitImpl(const void* typeId, const void* event) {
    std::shared_ptr<EntryList> snapshot;

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_subscriptions.find(typeId);
        if (it == s_subscriptions.end()) return;
        snapshot = it->second;  // 拷贝 shared_ptr，遍历期间列表不会被修改
    }

    if (!snapshot) return;

    for (auto& entry : *snapshot) {
        entry.callback(event);
    }
}

void EventBus::Clear() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_subscriptions.clear();
    s_nextHandle = 1;
}

size_t EventBus::GetSubscriptionCount() {
    std::lock_guard<std::mutex> lock(s_mutex);
    size_t total = 0;
    for (auto& pair : s_subscriptions) {
        if (pair.second) total += pair.second->size();
    }
    return total;
}