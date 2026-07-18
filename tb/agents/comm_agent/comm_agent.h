// ============================================================
// comm_agent.h — Complete comm UVC (agent wrapper)
// ============================================================

#ifndef COMM_AGENT_H
#define COMM_AGENT_H

#include <systemc>
#include <uvm>
#include "comm_seq_item.h"
#include "comm_sequencer.h"
#include "comm_driver.h"
#include "comm_monitor.h"

class comm_agent : public uvm::uvm_agent {
public:
    UVM_COMPONENT_UTILS(comm_agent);

    comm_sequencer* sequencer;
    comm_driver* driver;
    comm_monitor* monitor;

    comm_agent(uvm::uvm_component_name name = "comm_agent")
        : uvm::uvm_agent(name), sequencer(nullptr), driver(nullptr), monitor(nullptr) {}
    
    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        uvm::uvm_agent::build_phase(phase);

        if(get_is_active() == uvm::UVM_ACTIVE) {
            sequencer = comm_sequencer::type_id::create("sequencer", this);
            driver = comm_driver::type_id::create("driver", this);
        }
        monitor = comm_monitor::type_id::create("monitor", this);
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        uvm::uvm_agent::connect_phase(phase);
        if(get_is_active() == uvm::UVM_ACTIVE) {
            driver->seq_item_port.connect(sequencer->seq_item_export);
        }
    }
};

#endif // COMM_AGENT_H