// ============================================================
// logger_agent.h — Passive agent (monitor only, no driver/sequencer)
// Wraps logger_monitor for consistency with the other agents.
// ============================================================

#ifndef LOGGER_AGENT_H
#define LOGGER_AGENT_H

#include <systemc>
#include <uvm>
#include "logger_monitor.h"

class logger_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(logger_agent);

    logger_monitor* monitor;

    logger_agent(uvm::uvm_component_name name = "logger_agent")
        : uvm::uvm_agent(name), monitor(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        monitor = logger_monitor::type_id::create("monitor", this);
        if (!monitor) {
            UVM_FATAL("LOGGER_AGENT", "Failed to create logger_monitor");
        }
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase;
        // No connections needed for a passive agent
    }
};

#endif // LOGGER_AGENT_H
