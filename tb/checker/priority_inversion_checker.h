// ============================================================
// priority_inversion_checker.h — Flags anomalously high mutex
// wait latency for the highest-priority task (alarm_task), and
// confirms classic priority inversion when the mutex holder at
// the moment of the wait had a lower priority.
//
// LIMITATION: holder_task_id is a best-effort snapshot captured
// by the DUT just before attempting pthread_mutex_lock() — there
// is a race window where the actual holder may differ by the time
// the lock is acquired. See hal.c for details.
// ============================================================

#ifndef PRIORITY_INVERSION_CHECKER_H
#define PRIORITY_INVERSION_CHECKER_H

#include <uvm>
#include <map>

extern "C" {
#include "config.h"
}

class priority_inversion_checker : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(priority_inversion_checker)

    unsigned int total_waits;
    unsigned int suspicious_waits;
    unsigned int confirmed_inversions;
    unsigned int wait_threshold_ms;

    std::map<unsigned int, unsigned int> max_wait_per_task;

    priority_inversion_checker(uvm::uvm_component_name name = "priority_inversion_checker")
        : uvm::uvm_component(name),
          total_waits(0),
          suspicious_waits(0),
          confirmed_inversions(0),
          wait_threshold_ms(50) {}

    void record_wait(unsigned int task_id, unsigned int mutex_id, unsigned int wait_ms, unsigned int holder_task_id) {
        total_waits++;
        if (max_wait_per_task[task_id] < wait_ms) {
            max_wait_per_task[task_id] = wait_ms;
        }

        bool holder_known = (holder_task_id != static_cast<unsigned int>(-1));
        bool holder_is_lower_priority = holder_known && (holder_task_id < task_id);

        if (task_id == PRIORITY_ALARM && wait_ms > wait_threshold_ms) {
            suspicious_waits++;
            if (holder_is_lower_priority) {
                confirmed_inversions++;
                UVM_ERROR(
                    "PRIO_INVERSION",
                    "CONFIRMED: alarm_task (prio " + std::to_string(task_id) +
                    ") waited " + std::to_string(wait_ms) + "ms for mutex_id=" +
                    std::to_string(mutex_id) + " held by task_id=" +
                    std::to_string(holder_task_id) + " (lower priority)."
                );
            } else {
                UVM_WARNING(
                    "PRIO_INVERSION",
                    "alarm_task waited " + std::to_string(wait_ms) + "ms for mutex_id=" +
                    std::to_string(mutex_id) + " (holder=" +
                    (holder_known ? std::to_string(holder_task_id) : std::string("unknown")) +
                    ", inconclusive — holder unknown or same/higher priority)."
                );
            }
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        UVM_INFO(
            "PRIO_INVERSION_REPORT",
            "Total mutex waits observed: " + std::to_string(total_waits) +
            ", suspicious (alarm_task > " + std::to_string(wait_threshold_ms) + "ms): " +
            std::to_string(suspicious_waits) +
            ", confirmed inversions: " + std::to_string(confirmed_inversions),
            uvm::UVM_LOW
        );
        for (const auto& kv : max_wait_per_task) {
            UVM_INFO(
                "PRIO_INVERSION_REPORT",
                "  task " + std::to_string(kv.first) + ": max wait = " +
                std::to_string(kv.second) + "ms",
                uvm::UVM_LOW
            );
        }
    }
};

#endif // PRIORITY_INVERSION_CHECKER_H