#include "PerformancePanel.h"
#include <imgui.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

void PerformancePanel::Draw() {
    if (!frameTimes_) return;

    if (ImGui::CollapsingHeader("Performance")) {
        if (!frameTimes_->empty()) {
            float sum = std::accumulate(frameTimes_->begin(), frameTimes_->end(), 0.0f);
            float avg = sum / frameTimes_->size();

            float sq_sum = 0.0f;
            for (float t : *frameTimes_) sq_sum += (t - avg) * (t - avg);
            float stddev = std::sqrt(sq_sum / frameTimes_->size());

            std::vector<float> sorted(frameTimes_->begin(), frameTimes_->end());
            std::sort(sorted.begin(), sorted.end());

            size_t p99_index = (size_t)(sorted.size() * 0.99);
            if (p99_index >= sorted.size()) p99_index = sorted.size() - 1;
            float p99 = sorted[p99_index];

            size_t worst_count = std::max<size_t>(1, sorted.size() / 100);
            float worst_sum = 0.0f;
            for (size_t i = sorted.size() - worst_count; i < sorted.size(); ++i) worst_sum += sorted[i];
            float worst1pct = worst_sum / worst_count;

            ImGui::Text("Avg: %.3f ms", avg);
            ImGui::Text("P99: %.3f ms", p99);
            ImGui::Text("Worst 1%% avg: %.3f ms", worst1pct);
            ImGui::Text("StdDev: %.3f ms", stddev);
        } else {
            ImGui::Text("Collecting frame data...");
        }
    }
}