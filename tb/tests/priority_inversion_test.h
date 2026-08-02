// ============================================================
// priority_inversion_test.h — Runs background sensor stimulus
// while Group B's real mutex contention is measured for
// anomalous high-priority (alarm_task) wait latency.
// ============================================================

#ifndef PRIORITY_INVERSION_TEST_H
#define PRIORITY_INVERSION_TEST_H

#include <systemc>
#include <uvm>
#include "base_test.h"
#include "../sequences/priority_inversion_seq.h"

class priority_inversion_test : public base_test {
public:
    UVM_COMPONENT_UTILS(priority_inversion_test)

    priority_inversion_seq* prio_seq;

    priority_inversion_test(uvm::uvm_component_name name = "priority_inversion_test")
        : base_test(name), prio_seq(nullptr) {}

    void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this, "priority_inversion_test starting");

        UVM_INFO("PIT", "Priority inversion test (group B — mutex wait measurement)", uvm::UVM_LOW);

        prio_seq = priority_inversion_seq::type_id::create("prio_seq", this);
        prio_seq->start(m_env->sensor_agt->sequencer);

        phase.drop_objection(this, "priority_inversion_test completed");
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        UVM_INFO(
            "PIT",
            "Confirmed inversions: " + std::to_string(m_env->scoreboard->prio_inv_chk->confirmed_inversions) +
            ", suspicious waits: " + std::to_string(m_env->scoreboard->prio_inv_chk->suspicious_waits),
            uvm::UVM_LOW
        );
    }
};

#endif // PRIORITY_INVERSION_TEST_H