// ============================================================
// control_input_agent.h — Passive agent (monitor only, no driver/sequencer)
// Wraps control_input_monitor for consistency with the other agents.
// ============================================================

#ifndef CONTROL_INPUT_AGENT_H
#define CONTROL_INPUT_AGENT_H

#include <systemc>
#include <uvm>
#include "control_input_monitor.h"

class control_input_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(control_input_agent);

    control_input_monitor* monitor;

    control_input_agent(uvm::uvm_component_name name = "control_input_agent")
        : uvm::uvm_agent(name), monitor(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        monitor = control_input_monitor::type_id::create("monitor", this);
        if (!monitor) {
            UVM_FATAL("CONTROL_INPUT_AGENT", "Failed to create control_input_monitor");
        }
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        // No connections needed for a passive agent
    }
};

#endif // CONTROL_INPUT_AGENT_H
