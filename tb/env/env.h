// ============================================================
// env.h — Minimal verification environment
// ============================================================

#ifndef ENV_H
#define ENV_H

#include <systemc>
#include <uvm>

class env : public uvm::uvm_env {
public:
    UVM_COMPONENT_UTILS(env);

    env(uvm::uvm_component_name name = "env") : uvm::uvm_env(name) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_env::build_phase(phase);
        UVM_INFO("ENV", "Environment built", uvm::UVM_NONE);
    }
};

#endif