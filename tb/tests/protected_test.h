#ifndef PROTECTED_TEST_H
#define PROTECTED_TEST_H

#include <systemc>
#include <uvm>
#include "../tests/base_test.h"
#include "../sequences/race_condition_seq.h"
#include "../env/env.h"

class protected_test : public base_test {
public:
    UVM_COMPONENT_UTILS(protected_test)

    race_condition_seq* race_cond_seq;

    protected_test(uvm::uvm_component_name name = "protected_test")
        : base_test(name), race_cond_seq(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        base_test::build_phase(phase);

        uvm::uvm_config_db<int>::set(this, "*", "group_selection", 1); // 1 = group B
        uvm::uvm_config_db<int>::set(this, "*", "iterations", 1000);
        uvm::uvm_config_db<int>::set(this, "*", "randomize_delays", 1);
        uvm::uvm_config_db<int>::set(this, "*", "randomize_priorities", 1);
    }

    void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this, "protected_test starting");

        UVM_INFO("PT", "Protected test (group B)", uvm::UVM_LOW);

        race_cond_seq = race_condition_seq::type_id::create("race_cond_test", this);
        race_cond_seq->start(m_env->sensor_agt->sequencer);

        phase.drop_objection(this, "protected_test completed");
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        if (m_env->scoreboard->mismatches == 0) {
            UVM_INFO(
                "PT",
                "No mismatches detected - mutex protection validated (Expected for group B)",
                uvm::UVM_LOW
            );

        } else {
            UVM_ERROR(
                "PT",
                "Mismatches detected despite mutex protection: " +
               std::to_string(m_env->scoreboard->mismatches) +
               " - investigate possible deadlock or lock omission"
            );
        }
    }
};

#endif // PROTECTED_TEST_H