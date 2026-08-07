// ============================================================
// sensor_monitor.h — Monitors sensor transactions from DUT
// Captures temperature/hunidity readings with RTOS metadata
// and forwards to scoreboard + coverage
// ============================================================

#ifndef SENSOR_MONITOR_H
#define SENSOR_MONITOR_H

#include <systemc>
#include <uvm>
#include "sensor_seq_item.h"

class sensor_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(sensor_monitor);

    uvm::uvm_analysis_port<sensor_seq_item*> analysis_port;

    sensor_monitor(uvm::uvm_component_name name = "sensor_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {}

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;  // Suppress unused parameter warning

        UVM_INFO("MON", "Sensor monitor started", uvm::UVM_LOW);
        // Passive observer — no objection needed
    }

    void sample_and_send(float temperature, float humidity, unsigned int timestamp_ns = 0, int task_priority = 0, unsigned int task_id =0, unsigned int sim_time_ns = 0) {
        sensor_seq_item* seq_item = sensor_seq_item::type_id::create("sensor_sample");
        seq_item->temperature = temperature;
        seq_item->humidity = humidity;
        seq_item->timestamp_ns = timestamp_ns;
        seq_item->task_priority = task_priority;
        seq_item->task_id = task_id;
        seq_item->sim_time_ns = sim_time_ns;

        UVM_INFO(
            "MON",
            "Sampled sensor values: temperature=" + std::to_string(seq_item->temperature) +
            ", humidity=" + std::to_string(seq_item->humidity) +
            ", timestamp=" + std::to_string(seq_item->timestamp_ns),
            uvm::UVM_LOW
        );

        analysis_port.write(seq_item);
    }
};

#endif // SENSOR_MONITOR_H