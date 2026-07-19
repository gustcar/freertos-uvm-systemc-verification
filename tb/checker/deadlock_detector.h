// ============================================================
// deadlock_detector.h — Watchdog-based deadlock detector
// Fires if no task heartbeat arrives within timeout window
// ============================================================

#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <systemc>
#include <uvm>

class deadlock_detector : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(deadlock_detector)

    unsigned int timeout_ms;
    unsigned int last_seen_ms;
    bool deadlock_detected;

    deadlock_detector(uvm::uvm_component_name name = "deadlock_detector")
        : uvm::uvm_component(name),
        timeout_ms(5000),
        last_seen_ms(0),
        deadlock_detected(false) {}

    void heartbeat(unsigned int current_ms) {
        last_seen_ms = current_ms;
    }

    void check_deadlock(unsigned int current_ms) {
        if((current_ms - last_seen_ms) > timeout_ms && !deadlock_detected) {
            deadlock_detected = true;
            UVM_ERROR(
                "DEADLOCK",
                "Task stalled for " +
                std::to_string(current_ms - last_seen_ms) +
                "ms - possible deadlock detected."
            );
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Suppress unused parameter warning
        if(deadlock_detected) {
            UVM_ERROR("DEADLOCK_DETECTOR", "Deadlock detected during simulation.");
        } else {
            UVM_INFO("DEADLOCK_DETECTOR", "No deadlock detected during simulation.", uvm::UVM_LOW);
        }
    }
};

#endif // DEADLOCK_DETECTOR_H