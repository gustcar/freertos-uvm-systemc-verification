// ============================================================
// base_test.h — Minimal UVM test
// ============================================================

#ifndef BASE_TEST_H
#define BASE_TEST_H

#include <uvm>
#include "../env/env.h"

class base_test : public uvm::uvm_test {
public:
    UVM_COMPONENT_UTILS(base_test);

    env* m_env;

    explicit base_test(const char* name = "base_test")
        : uvm::uvm_test(name), m_env(nullptr) {}

    virtual void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_test::build_phase(phase);
        m_env = env::type_id::create("m_env", this);
        UVM_INFO("BASE_TEST", "Environment created", uvm::UVM_NONE);
    }

    virtual void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this);
        UVM_INFO("BASE_TEST", "Test + env running", uvm::UVM_NONE);
        sc_core::wait(10, sc_core::SC_NS);
        phase.drop_objection(this);
    }
};

#endif