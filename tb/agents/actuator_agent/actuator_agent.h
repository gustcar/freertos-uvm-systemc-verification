// ============================================================
// actuator_agent.h — Passive agent (monitor only, no driver/sequencer)
// ============================================================

#ifndef ACTUATOR_AGENT_H
#define ACTUATOR_AGENT_H

#include <systemc>
#include <uvm>
#include "actuator_monitor.h"

class actuator_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(actuator_agent);

    actuator_monitor* monitor;

    actuator_agent(uvm::uvm_component_name name = "actuator_agent")
        : uvm::uvm_agent(name), monitor(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase;  // Suppress unused parameter warning
        monitor = actuator_monitor::type_id::create("monitor", this);
        if (!monitor) {
            UVM_FATAL("ACTUATOR_AGENT", "Failed to create actuator_monitor");
        }
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase;  // Suppress unused parameter warning
        // No connections needed for a passive agent
    }
};

#endif // ACTUATOR_AGENT_H