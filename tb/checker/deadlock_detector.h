// ============================================================
// deadlock_detector.h — Watchdog-based deadlock detector
// Tracks heartbeats per task. Flags tasks that stop producing
// events within a configurable timeout window.
// ============================================================

#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <systemc>
#include <uvm>
#include <string>
#include <map>
#include <set>

class deadlock_detector : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(deadlock_detector)

    unsigned int timeout_ms;
    unsigned int total_heartbeats;

    // last seen time (ms) per task
    std::map<unsigned int, unsigned int> last_heartbeat_ms;
    // how many heartbeats each task produced
    std::map<unsigned int, unsigned int> heartbeat_count;
    // tasks already flagged as stalled (sticky until reset)
    std::set<unsigned int> stalled_tasks;

    deadlock_detector(uvm::uvm_component_name name = "deadlock_detector")
        : uvm::uvm_component(name),
          timeout_ms(5000),
          total_heartbeats(0) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_component::build_phase(phase);
        // Allow per-test configuration
        int t = 0;
        if (uvm::uvm_config_db<int>::get(this, "", "deadlock_timeout_ms", t) && t > 0) {
            timeout_ms = static_cast<unsigned int>(t);
        }
    }

    // Called by scoreboard on every sensor (or other) transaction from a task
    void heartbeat(unsigned int task_id, unsigned int timestamp_ns) {
        unsigned int ts_ms = timestamp_ns / 1000000u;
        last_heartbeat_ms[task_id] = ts_ms;
        heartbeat_count[task_id]++;
        total_heartbeats++;

        // Task recovered after being flagged
        if (stalled_tasks.count(task_id)) {
            stalled_tasks.erase(task_id);
            UVM_INFO("DEADLOCK",
                     "Task " + std::to_string(task_id) + " resumed activity",
                     uvm::UVM_MEDIUM);
        }
    }

    // Called after heartbeat (or from a periodic check if you add one)
    void check(unsigned int current_ns) {
        unsigned int current_ms = current_ns / 1000000u;

        for (const auto& kv : last_heartbeat_ms) {
            unsigned int task_id = kv.first;
            unsigned int last_ms = kv.second;

            // Safe delta (ignore inverted timestamps)
            if (current_ms < last_ms)
                continue;

            unsigned int idle_ms = current_ms - last_ms;

            if (idle_ms > timeout_ms && stalled_tasks.count(task_id) == 0) {
                stalled_tasks.insert(task_id);
                UVM_ERROR("DEADLOCK",
                    "Task " + std::to_string(task_id) +
                    " stalled for " + std::to_string(idle_ms) +
                    " ms (timeout=" + std::to_string(timeout_ms) +
                    " ms) — possible deadlock.");
            }
        }
    }

    bool has_deadlock() const {
        return !stalled_tasks.empty();
    }

    void reset() {
        last_heartbeat_ms.clear();
        heartbeat_count.clear();
        stalled_tasks.clear();
        total_heartbeats = 0;
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;

        unsigned int now_ms =
            static_cast<unsigned int>(sc_core::sc_time_stamp().to_seconds() * 1000.0);

        UVM_INFO("DEADLOCK_DETECTOR",
                 "Heartbeats total=" + std::to_string(total_heartbeats) +
                 ", tracked tasks=" + std::to_string(last_heartbeat_ms.size()) +
                 ", sim end=" + std::to_string(now_ms) + " ms, timeout=" +
                 std::to_string(timeout_ms) + " ms",
                 uvm::UVM_LOW);

        // Per-task trailing idle (how long each task had been silent at sim end).
        // Useful for picking a meaningful timeout: it must exceed the largest
        // idle a healthy task legitimately shows (sparse tasks like comm log few).
        for (const auto& kv : last_heartbeat_ms) {
            unsigned int task_id = kv.first;
            unsigned int idle_ms = (now_ms >= kv.second) ? (now_ms - kv.second) : 0u;
            UVM_INFO("DEADLOCK_DETECTOR",
                     "  task " + std::to_string(task_id) +
                     ": " + std::to_string(heartbeat_count[task_id]) +
                     " heartbeats, trailing idle=" + std::to_string(idle_ms) + " ms",
                     uvm::UVM_LOW);
        }

        if (!stalled_tasks.empty()) {
            std::string list;
            for (unsigned int id : stalled_tasks) {
                if (!list.empty()) list += ",";
                list += std::to_string(id);
            }
            UVM_ERROR("DEADLOCK_DETECTOR",
                      "Deadlock/stall detected on task(s): " + list);
        } else {
            UVM_INFO("DEADLOCK_DETECTOR",
                     "No deadlock detected.",
                     uvm::UVM_LOW);
        }
    }
};

#endif // DEADLOCK_DETECTOR_H