// ============================================================
// concurrency_sb.h — Scoreboard for concurrency verification
// Integrates data integrity, timing, and deadlock checkers.
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
    UVM_COMPONENT_UTILS(concurrency_sb)

    uvm::uvm_analysis_imp<sensor_seq_item*, concurrency_sb>     sensor_analysis_export;
    uvm::uvm_analysis_imp<actuator_seq_item*, concurrency_sb> actuator_analysis_export;
    uvm::uvm_analysis_imp<comm_seq_item*, concurrency_sb>         comm_analysis_export;

    data_integrity_checker* data_integrity_chk;
    timing_checker*         timing_chk;
    deadlock_detector*      deadlock_chk;

    unsigned int mismatches;

    concurrency_sb(uvm::uvm_component_name name = "concurrency_sb")
        : uvm::uvm_scoreboard(name),
          sensor_analysis_export("sensor_analysis_export", this),
          actuator_analysis_export("actuator_analysis_export", this),
          comm_analysis_export("comm_analysis_export", this),
          data_integrity_chk(nullptr),
          timing_chk(nullptr),
          deadlock_chk(nullptr),
          mismatches(0) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::build_phase(phase);
        data_integrity_chk = data_integrity_checker::type_id::create("data_integrity_chk", this);
        timing_chk         = timing_checker::type_id::create("timing_chk", this);
        deadlock_chk       = deadlock_detector::type_id::create("deadlock_chk", this);
    }

    void write(sensor_seq_item* item) {
        // 1. Data integrity
        data_integrity_chk->check_temp_range(item->temperature);
        data_integrity_chk->check_humidity_range(item->humidity);

        // 2. Timing: detect threshold crossing
        timing_chk->on_sensor(item->temperature, item->timestamp_ns);

        // 3. Deadlock: heartbeat + check
        deadlock_chk->heartbeat(item->task_id, item->timestamp_ns);
        deadlock_chk->check(item->timestamp_ns);
    }

    void write(actuator_seq_item* item) {
        // PWM range check
        if (item->type == actuator_seq_item::PWM) {
            if (item->pwm_duty_cycle > 100) {
                mismatches++;
                UVM_ERROR("SB_ACTUATOR",
                    "Invalid PWM duty cycle: " +
                    std::to_string(item->pwm_duty_cycle) + "%");
            }
            // Any PWM change counts as possible alarm reaction
            timing_chk->on_actuator_reaction();
        } else {
            // GPIO change (e.g. alarm LED / relay)
            if (!item->gpio_state) {
                UVM_INFO("SB_ACTUATOR",
                    "GPIO OFF: pin=" + std::to_string(item->gpio_pin),
                    uvm::UVM_HIGH);
            }
            timing_chk->on_actuator_reaction();
        }
    }

    void write(comm_seq_item* item) {
        if (item->command_type == 1) {
            UVM_INFO("SB_COMM",
                "UART target command: " +
                std::to_string(item->command_value) + "°C",
                uvm::UVM_LOW);
        } else {
            UVM_INFO("SB_COMM",
                "UART command type=" +
                std::to_string(item->command_type),
                uvm::UVM_LOW);
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::report_phase(phase);
        UVM_INFO("SB_REPORT",
                 "Final mismatches: " + std::to_string(mismatches),
                 uvm::UVM_LOW);
    }
};

#endif // CONCURRENCY_SB_H