// ============================================================
// timing_checker.h — Alarm latency checker
// Measures time from temperature threshold crossing to
// first actuator reaction (GPIO/PWM).
// ============================================================

#ifndef TIMING_CHECKER_H
#define TIMING_CHECKER_H

#include <systemc>
#include <uvm>

class timing_checker : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(timing_checker)

    // Threshold that defines "alarm condition" (same region used by race seq)
    float alarm_threshold_c;
    bool  threshold_crossed;
    unsigned int cross_timestamp_ns;

    unsigned int max_latency_ms;
    unsigned int samples;
    unsigned int pending_crossings;  // crossed but not yet matched by actuator

    timing_checker(uvm::uvm_component_name name = "timing_checker")
        : uvm::uvm_component(name),
          alarm_threshold_c(35.0f),
          threshold_crossed(false),
          cross_timestamp_ns(0),
          max_latency_ms(0),
          samples(0),
          pending_crossings(0) {}

    // Called on every sensor sample
    void on_sensor(float temperature, unsigned int timestamp_ns) {
        if (!threshold_crossed && temperature >= alarm_threshold_c) {
            threshold_crossed  = true;
            cross_timestamp_ns = timestamp_ns;
            pending_crossings++;
            UVM_INFO("TIMING",
                     "Threshold crossed @ " + std::to_string(timestamp_ns) +
                     " ns (T=" + std::to_string(temperature) + "°C)",
                     uvm::UVM_MEDIUM);
        }
        // Reset when temperature falls back below threshold
        if (threshold_crossed && temperature < alarm_threshold_c - 1.0f) {
            threshold_crossed = false;
        }
    }

    // Called when actuator reacts (GPIO or PWM). Uses current sim time if no ts available.
    void on_actuator_reaction(unsigned int reaction_timestamp_ns = 0) {
        if (pending_crossings == 0) return;

        unsigned int ts = reaction_timestamp_ns;
        if (ts == 0) {
            // Fallback: SystemC simulation time in ns
            ts = static_cast<unsigned int>(sc_core::sc_time_stamp().to_seconds() * 1e9);
        }

        if (ts >= cross_timestamp_ns) {
            unsigned int latency_samples = (ts - cross_timestamp_ns);
            samples++;
            if (latency_samples > max_latency_ms)
                max_latency_ms = latency_samples;
            pending_crossings--;
            UVM_INFO("TIMING",
                     "Alarm latency = " + std::to_string(latency_samples) + " ns",
                     uvm::UVM_MEDIUM);
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        UVM_INFO("TIMING",
                 "Alarm Latency Report: max=" + std::to_string(max_latency_ms) +
                 " ns (sim-time), samples=" + std::to_string(samples) +
                 ", pending_unmatched=" + std::to_string(pending_crossings),
                 uvm::UVM_LOW);
    }
};

#endif // TIMING_CHECKER_H