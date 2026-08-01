// ============================================================
// race_condition_test.h — Test case targeting Group A (vulnerable)
// Runs race_condition_seq with randomized delays and priorities
// to expose unprotected shared-data hazards. Expected outcome:
// mismatches in the scoreboard due to torn reads / inconsistent
// actuator states.
// ============================================================

#ifndef RACE_CONDITION_TEST_H
#define RACE_CONDITION_TEST_H

#include <systemc>
#include <uvm>
#include "base_test.h"
#include "../sequences/race_condition_seq.h"
#include "../sequences/comm_target_seq.h"
#include "../env/env.h"

class race_condition_test : public base_test {
public:
    UVM_COMPONENT_UTILS(race_condition_test)

    race_condition_seq* race_cond_seq;
    comm_target_seq*    comm_seq;

    race_condition_test(uvm::uvm_component_name name = "race_condition_test")
        : base_test(name), race_cond_seq(nullptr) {}
    
    void build_phase(uvm::uvm_phase& phase) override {
        base_test::build_phase(phase);

        // configure test to target group A (vulnerable, no mutex)
        uvm::uvm_config_db<int>::set(this, "*", "group_selection", 0); // 0 = group A
        uvm::uvm_config_db<int>::set(this, "*", "iterations", 1000);
        uvm::uvm_config_db<int>::set(this, "*", "randomize_delays", 1);
        uvm::uvm_config_db<int>::set(this, "*", "randomize_priorities", 1);
    }

    void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this, "race_condition_test starting");

        UVM_INFO("RCT", "Race condition test (group A)", uvm::UVM_LOW);

        race_cond_seq = race_condition_seq::type_id::create("race_cond_seq", this);
        comm_seq = comm_target_seq::type_id::create("comm_seq", this);

        sc_core::sc_spawn(
            sc_core::sc_bind(
                &comm_target_seq::start,
                comm_seq,
                m_env->comm_agt->sequencer,
                nullptr,
                -1,
                1
            ),
            "comm_target_seq_proc"
        );

        race_cond_seq->start(m_env->sensor_agt->sequencer);

        phase.drop_objection(this, "race_condition_test completed");
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        if(m_env->scoreboard->mismatches > 0) {
            UVM_INFO(
                "RCT",
                std::to_string(m_env->scoreboard->mismatches) +
                " mismatches found (Expected for group A)",
                uvm::UVM_LOW
            );
        } else {
            UVM_WARNING(
                "RCT",
                "No mismatches detected - race contition may need "
                "more iterations or tighter delays do happen."
            );
        }
    }
};

#endif // RACE_CONDITION_TEST_H