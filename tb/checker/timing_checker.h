// ============================================================
// timing_checker.h — Alarm latency checker (measures time
// from threshold crossing to alarm_state change)
// ============================================================

#ifndef TIMING_CHECKER_H
#define TIMING_CHECKER_H

#include <systemc>
#include <uvm>

class timing_checker : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(timing_checker)

    unsigned int max_latency_ms;
    unsigned int samples;
    unsigned int last_timestamp_ns;

    timing_checker(uvm::uvm_component_name name)
        : uvm::uvm_component(name),
          max_latency_ms(0),
          samples(0),
          last_timestamp_ns(0) {}

    void record_timestamp(unsigned int timestamp_ns) {
        if(last_timestamp_ns > 0) {
            unsigned int delta_ns = timestamp_ns - last_timestamp_ns;
            unsigned int latency_ms = delta_ns / 1000000; // ns -> ms
            record_latency(latency_ms);
        }
        last_timestamp_ns = timestamp_ns;
    }

    void record_latency(unsigned int latency_ms) {
        samples++;
        if (latency_ms > max_latency_ms) {
            max_latency_ms = latency_ms;
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Suppress unused parameter warning
        UVM_INFO(
            "TIMING",
            "Alarm Latency Report: max=" + std::to_string(max_latency_ms) +
            " ms, samples=" + std::to_string(samples),
            uvm::UVM_LOW
        );
    }
};

#endif // TIMING_CHECKER_H