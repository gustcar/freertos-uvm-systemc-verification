// ============================================================
// base_test.h — Minimal UVM test
// ============================================================

#ifndef BASE_TEST_H
#define BASE_TEST_H

#include <uvm>

class base_test : public uvm::uvm_test {
public:
    UVM_COMPONENT_UTILS(base_test);

    explicit base_test(const char* name = "base_test")
        : uvm::uvm_test(name) {}

    void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this);
        uvm::uvm_report_info("BASE_TEST", "Test running...");
        sc_core::wait(10, sc_core::SC_NS);
        phase.drop_objection(this);
    }
};

#endif