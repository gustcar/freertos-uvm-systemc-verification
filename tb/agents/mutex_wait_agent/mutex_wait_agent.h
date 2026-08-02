// ============================================================
// mutex_wait_agent.h — Passive agent (monitor only, no driver/sequencer)
// Wraps mutex_wait_monitor for consistency with the other agents.
// ============================================================

#ifndef MUTEX_WAIT_AGENT_H
#define MUTEX_WAIT_AGENT_H

#include <systemc>
#include <uvm>
#include "mutex_wait_monitor.h"

class mutex_wait_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(mutex_wait_agent);

    mutex_wait_monitor* monitor;

    mutex_wait_agent(uvm::uvm_component_name name = "mutex_wait_agent")
        : uvm::uvm_agent(name), monitor(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        monitor = mutex_wait_monitor::type_id::create("monitor", this);
        if (!monitor) {
            UVM_FATAL("MUTEX_WAIT_AGENT", "Failed to create mutex_wait_monitor");
        }
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        // No connections needed for a passive agent
    }
};

#endif // MUTEX_WAIT_AGENT_H