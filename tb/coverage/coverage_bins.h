// ============================================================
// coverage_bins.h — Functional coverage bins for concurrency
// verification (priority x delay x task combinations)
// ============================================================

#ifndef COVERAGE_BINS_H
#define COVERAGE_BINS_H

#include <systemc>
#include <uvm>
#include <map>
#include <string>
#include "../agents/sensor_agent/sensor_seq_item.h"
#include "../agents/actuator_agent/actuator_seq_item.h"
#include "../agents/comm_agent/comm_seq_item.h"

struct cross_coverage_key {
    int temp_range;       // 0=low(<20), 1=normal(20-30), 2=high(>30)
    int priority;         // 0=low, 1=medium, 2=high
    unsigned int task_id;
    int delay_category;   // 0=no_delay, 1=short(1-5ms), 2=long(>5ms)

    bool operator<(const cross_coverage_key& other) const {
        if (temp_range != other.temp_range) return temp_range < other.temp_range;
        if (priority != other.priority)     return priority < other.priority;
        if (task_id != other.task_id)       return task_id < other.task_id;
        return delay_category < other.delay_category;
    }
};

class coverage_bins : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(coverage_bins);

    std::map<cross_coverage_key, unsigned int> cross_cov_map; // Map to hold coverage bins

    uvm::uvm_analysis_imp<sensor_seq_item*, coverage_bins> sensor_analysis_export;
    uvm::uvm_analysis_imp<actuator_seq_item*, coverage_bins> actuator_analysis_export;
    uvm::uvm_analysis_imp<comm_seq_item*, coverage_bins> comm_analysis_export;

    // Simple counters
    unsigned int sampled_sensors;
    unsigned int sampled_actuators;
    unsigned int sampled_comms;

    unsigned int critical_corner_hits;

    coverage_bins(uvm::uvm_component_name name = "coverage_bins")
        : uvm::uvm_component(name),
          sensor_analysis_export("sensor_analysis_export", this),
          actuator_analysis_export("actuator_analysis_export", this),
          comm_analysis_export("comm_analysis_export", this),
          sampled_sensors(0),
          sampled_actuators(0),
          sampled_comms(0),
          critical_corner_hits(0) {}

    void write(sensor_seq_item* item) {
        sampled_sensors++;

        // Determine temperature range bin
        int temp_range = 1; // default = normal
        if (item->temperature < 20.0f) temp_range = 0;      // low
        else if (item->temperature > 30.0f) temp_range = 2; // high

        // Determine delay category (simulated based on timestamp delta)
        static unsigned int last_ts = 0;
        unsigned int dt = (last_ts > 0) ? (item->timestamp_ns - last_ts) : 0;
        last_ts = item->timestamp_ns;
        int delay_cat = (dt > 5000) ? 2 : ((dt > 1000) ? 1 : 0);

        // Build cross-coverage key
        cross_coverage_key key;
        key.temp_range = temp_range;
        key.priority = item->task_priority;
        key.task_id = item->task_id;
        key.delay_category = delay_cat;

        // Increment cross bin
        cross_cov_map[key]++;

        // Track critical corner case: critical temp + low priority + long delay
        if (temp_range == 2 && item->task_priority <= 1 && delay_cat >= 1) {
            critical_corner_hits++;
        }

        UVM_INFO("COV_SENSOR", "Sample #" + std::to_string(sampled_sensors) +
                 ": temp=" + std::to_string(item->temperature) +
                 ", range=" + std::to_string(temp_range) +
                 ", pri=" + std::to_string(item->task_priority) +
                 ", task=" + std::to_string(item->task_id),
                 uvm::UVM_HIGH);
    }

    void write(actuator_seq_item* item) {
        (void)item;
        sampled_actuators++;
        UVM_INFO("COV_ACTUATOR", "Actuator sample #" + std::to_string(sampled_actuators),
                 uvm::UVM_HIGH);
    }

    void write(comm_seq_item* item) {
        sampled_comms++;
        UVM_INFO("COV_COMM", "Comm sample #" + std::to_string(sampled_comms) +
                 ": type=" + std::to_string(item->command_type) +
                 ", value=" + std::to_string(item->command_value),
                 uvm::UVM_HIGH);
    }

    void report_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_component::report_phase(phase);

        UVM_INFO("COV_REPORT", "=== Functional Coverage Report ===", uvm::UVM_LOW);
        UVM_INFO("COV_REPORT", "Total Sensors Sampled:   " + std::to_string(sampled_sensors), uvm::UVM_LOW);
        UVM_INFO("COV_REPORT", "Total Actuators Sampled: " + std::to_string(sampled_actuators), uvm::UVM_LOW);
        UVM_INFO("COV_REPORT", "Total Comms Sampled:     " + std::to_string(sampled_comms), uvm::UVM_LOW);
        UVM_INFO("COV_REPORT", "Unique Cross-Bins Hit:   " + std::to_string(cross_cov_map.size()), uvm::UVM_LOW);
        UVM_INFO("COV_REPORT", "Critical Corner Hits:    " + std::to_string(critical_corner_hits), uvm::UVM_LOW);

        // Detailed cross-coverage breakdown
        UVM_INFO("COV_REPORT", "--- Cross-Coverage Detail (temp_range × priority × task_id) ---", uvm::UVM_MEDIUM);
        for (const auto& kv : cross_cov_map) {
            std::string range_str = (kv.first.temp_range == 0) ? "LOW" :
                                    (kv.first.temp_range == 1) ? "NORM" : "HIGH";
            std::string pri_str = (kv.first.priority == 0) ? "LOW" :
                                  (kv.first.priority == 1) ? "MED" : "HI";
            UVM_INFO("COV_REPORT",
                     "[" + range_str + "]×[" + pri_str + "]×[task#" + std::to_string(kv.first.task_id) +
                     "]: " + std::to_string(kv.second) + " hits",
                     uvm::UVM_MEDIUM);
        }
    }
    
};
#endif // COVERAGE_BINS_H