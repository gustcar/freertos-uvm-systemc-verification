// ============================================================
// concurrency_sb.h — Scoreboard for concurrency verification
// Validates sensor data integrity, PWM/GPIO correctness, and
// tracks UART commands. Counts mismatches when violations occur.
// ============================================================

#ifndef CONCURRENCY_SB_H
#define CONCURRENCY_SB_H

#include <systemc>
#include <uvm>
#include "../agents/sensor_agent/sensor_seq_item.h"
#include "../agents/actuator_agent/actuator_seq_item.h"
#include "../agents/comm_agent/comm_seq_item.h"
#include "../checker/data_integrity_checker.h"
#include "../checker/timing_checker.h"
#include "../checker/deadlock_detector.h"

class concurrency_sb : public uvm::uvm_scoreboard {
public:
    UVM_COMPONENT_UTILS(concurrency_sb);

    uvm::uvm_analysis_imp<sensor_seq_item*, concurrency_sb>      sensor_analysis_export;
    uvm::uvm_analysis_imp<actuator_seq_item*, concurrency_sb>  actuator_analysis_export;
    uvm::uvm_analysis_imp<comm_seq_item*, concurrency_sb>          comm_analysis_export;

    data_integrity_checker* data_integrity_check;
    timing_checker*         timing_check;
    deadlock_detector*      deadlock_check;

    unsigned int mismatches;

    concurrency_sb(uvm::uvm_component_name name = "concurrency_sb")
        : uvm::uvm_scoreboard(name),
          sensor_analysis_export("sensor_analysis_export", this),
          actuator_analysis_export("actuator_analysis_export", this),
          comm_analysis_export("comm_analysis_export", this),
          data_integrity_check(nullptr),
          timing_check(nullptr),
          deadlock_check(nullptr),
          mismatches(0) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::build_phase(phase);
        data_integrity_check = data_integrity_checker::type_id::create("data_integrity_check", this);
        timing_check = timing_checker::type_id::create("timing_check", this);
        deadlock_check = deadlock_detector::type_id::create("deadlock_check", this);
    }

    void write(sensor_seq_item* item) {
        // Data integrity validation
        data_integrity_check->check_temp_range(item->temperature);
        data_integrity_check->check_humidity_range(item->humidity);

        // Track timing for concurrency analysis
        timing_check->record_timestamp(item->timestamp_ns);
    }

    void write(actuator_seq_item* item) {
        if (item->type == actuator_seq_item::PWM) {
            if (item->pwm_duty_cycle > 100) {
                mismatches++;
                UVM_ERROR(
                    "SB_ACTUATOR",
                    "Invalid PWM duty cycle: " + std::to_string(item->pwm_duty_cycle) + "%"
                );
            }
        } else {
            if (!item->gpio_state) {
                UVM_INFO(
                    "SB_ACTUATOR",
                    "GPIO OFF: pin=" + std::to_string(item->gpio_pin),
                    uvm::UVM_HIGH
                );
            }
        }
    }

    void write(comm_seq_item* item) {
        // Log command for traceability
        if (item->command_type == 1) {
            UVM_INFO(
                "SB_COMM",
                "UART target command: " + std::to_string(item->command_value) + "°C",
                uvm::UVM_LOW
            );
        } else {
            UVM_INFO(
                "SB_COMM",
                "UART command type=" + std::to_string(item->command_type),
                uvm::UVM_LOW
            );
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::report_phase(phase);
        UVM_INFO("SB_REPORT", "Final mismatches: " + std::to_string(mismatches), uvm::UVM_LOW);
    }
};

#endif // CONCURRENCY_SB_H