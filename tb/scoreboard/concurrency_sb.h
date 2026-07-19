// ============================================================
// concurrency_sb.h — Scoreboard verifying shared data integrity
// across tasks. Integrates data integrity, timing and deadlock
// checkers. Compares expected vs actual sensor/actuator states
// to detect race-induced inconsistencies.
// ============================================================

#ifndef CONCURRENCY_SB_H
#define CONCURRENCY_SB_H

#include <systemc>
#include <uvm>
#include "../checker/data_integrity_checker.h"
#include "../checker/timing_checker.h"
#include "../checker/deadlock_detector.h"
#include "../agents/sensor_agent/sensor_seq_item.h"
#include "../agents/actuator_agent/actuator_seq_item.h"

class concurrency_sb : public uvm::uvm_scoreboard {
public:
    UVM_COMPONENT_UTILS(concurrency_sb)

    data_integrity_checker* data_integrity_chk;
    timing_checker*         timing_chk;
    deadlock_detector*      deadlock_chk;

    uvm::uvm_analysis_imp<sensor_seq_item*, concurrency_sb>*   sensor_analysis_export;
    uvm::uvm_analysis_imp<actuator_seq_item*, concurrency_sb>* actuator_analysis_export;

    unsigned int mismatches;

    concurrency_sb(uvm::uvm_component_name name = "concurrency_sb")
        : uvm::uvm_scoreboard(name),
          data_integrity_chk(nullptr),
          timing_chk(nullptr),
          deadlock_chk(nullptr),
          sensor_analysis_export(nullptr),
          actuator_analysis_export(nullptr),
          mismatches(0) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::build_phase(phase);
        data_integrity_chk = data_integrity_checker::type_id::create("data_integrity_chk", this);
        timing_chk         = timing_checker::type_id::create("timing_chk", this);
        deadlock_chk       = deadlock_detector::type_id::create("deadlock_chk", this);

        sensor_analysis_export   = new uvm::uvm_analysis_imp<sensor_seq_item*, concurrency_sb>("sensor_analysis_export", this);
        actuator_analysis_export = new uvm::uvm_analysis_imp<actuator_seq_item*, concurrency_sb>("actuator_analysis_export", this);
    }

    void write(sensor_seq_item* item) {
        // Check data integrity for sensor readings
        data_integrity_chk->check_temp_range(item->temperature);
        data_integrity_chk->check_humidity_range(item->humidity);
    }

    void write(actuator_seq_item* item) {
        if(item->type == actuator_seq_item::PWM) {
            if(item->pwm_duty_cycle > 100) {
                mismatches++;
                UVM_ERROR("SB", "Invalid PWM duty: " + std::to_string(item->pwm_duty_cycle) + "%");
            }
        } else { // GPIO
            if(!item->gpio_state) {
                UVM_INFO("SB", "GPIO OFF: pin=" + std::to_string(item->gpio_pin), uvm::UVM_HIGH);
            }
        }
    }

    void report_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::report_phase(phase);
        UVM_INFO("SB", "Scoreboard: " + std::to_string(mismatches) + " mismatches", uvm::UVM_LOW);
    }
    
};

#endif // CONCURRENCY_SB_H