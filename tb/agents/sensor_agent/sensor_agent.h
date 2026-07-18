// ============================================================
// sensor_agent.h — Complete sensor UVC (agent wrapper)
// ============================================================

#ifndef SENSOR_AGENT_H
#define SENSOR_AGENT_H

#include <systemc>
#include <uvm>
#include "sensor_sequencer.h"
#include "sensor_driver.h"
#include "sensor_monitor.h"

class sensor_agent : public uvm::uvm_agent{
public:
    UVM_COMPONENT_UTILS(sensor_agent);

    sensor_sequencer* sequencer;
    sensor_driver* driver;
    sensor_monitor* monitor;

    sensor_agent(uvm::uvm_component_name name = "sensor_agent")
        : uvm::uvm_agent(name),
          sequencer(nullptr),
          driver(nullptr),
          monitor(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        uvm::uvm_agent::build_phase(phase);

        if(get_is_active() == uvm::UVM_ACTIVE) {
            sequencer = sensor_sequencer::type_id::create("sequencer", this);
            driver = sensor_driver::type_id::create("driver", this);
        }
        monitor = sensor_monitor::type_id::create("monitor", this);
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        uvm::uvm_agent::connect_phase(phase);

        if(get_is_active() == uvm::UVM_ACTIVE) {
            driver->seq_item_port.connect(sequencer->seq_item_export);
        }
    }
};

#endif // SENSOR_AGENT_H