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
    unsigned int tick_ms;          // period of the silence-probe tick (see run_phase)
    unsigned int total_heartbeats;

    // last seen time (ms) per task
    std::map<unsigned int, unsigned int> last_heartbeat_ms;
    // how many heartbeats each task produced
    std::map<unsigned int, unsigned int> heartbeat_count;
    // tasks already flagged as stalled (sticky until reset)
    std::set<unsigned int> stalled_tasks;
    // tasks that completed their work and exited normally — excluded from
    // stall checks so a finished task's trailing silence is not mistaken for
    // a deadlock (fed by dut_bridge when each DUT thread returns)
    std::set<unsigned int> finished_tasks;

    deadlock_detector(uvm::uvm_component_name name = "deadlock_detector")
        : uvm::uvm_component(name),
          timeout_ms(5000),
          tick_ms(1),
          total_heartbeats(0) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_component::build_phase(phase);
        // Allow per-test configuration
        int t = 0;
        if (uvm::uvm_config_db<int>::get(this, "", "deadlock_timeout_ms", t) && t > 0) {
            timeout_ms = static_cast<unsigned int>(t);
        }
        int tk = 0;
        if (uvm::uvm_config_db<int>::get(this, "", "deadlock_tick_ms", tk) && tk > 0) {
            tick_ms = static_cast<unsigned int>(tk);
        }
    }

    // Periodic silence probe. check() otherwise runs only when the scoreboard
    // receives an event, so a task (or the whole DUT) that goes completely
    // silent would never be re-examined and its stall never flagged. This tick
    // runs check() on a fixed simulated-time cadence so silence is caught too.
    // No objection is raised, so it never extends the simulation — the process
    // is killed when the run phase ends.
    void run_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        for (;;) {
            sc_core::wait(tick_ms, sc_core::SC_MS);
            unsigned int now_ns =
                static_cast<unsigned int>(sc_core::sc_time_stamp().to_seconds() * 1e9);
            check(now_ns);
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

    // Called by the scoreboard when a DUT task completes normally (thread
    // returned). A finished task is expected to go silent, so it is removed
    // from stall consideration — this is what distinguishes completion from
    // a genuine mid-run deadlock.
    void mark_finished(unsigned int task_id) {
        finished_tasks.insert(task_id);
        // A completion is authoritative proof the task was alive, so clear any
        // earlier (false) stall flag for it.
        stalled_tasks.erase(task_id);
        UVM_INFO("DEADLOCK",
                 "Task " + std::to_string(task_id) + " finished (excluded from stall checks)",
                 uvm::UVM_MEDIUM);
    }

    // Called after heartbeat (or from a periodic check if you add one)
    void check(unsigned int current_ns) {
        unsigned int current_ms = current_ns / 1000000u;

        for (const auto& kv : last_heartbeat_ms) {
            unsigned int task_id = kv.first;
            unsigned int last_ms = kv.second;

            // Finished tasks are expected to be silent — never a deadlock.
            if (finished_tasks.count(task_id))
                continue;

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
        finished_tasks.clear();
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
            std::string state = finished_tasks.count(task_id) ? " (finished)" : "";
            UVM_INFO("DEADLOCK_DETECTOR",
                     "  task " + std::to_string(task_id) +
                     ": " + std::to_string(heartbeat_count[task_id]) +
                     " heartbeats, trailing idle=" + std::to_string(idle_ms) + " ms" + state,
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