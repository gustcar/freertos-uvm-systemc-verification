// ============================================================
// comm_monitor.h — Monitors UART command transactions
// ============================================================

#ifndef COMM_MONITOR_H
#define COMM_MONITOR_H

#include <systemc>
#include <uvm>
#include "comm_seq_item.h"

class comm_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(comm_monitor);

    uvm::uvm_analysis_port<comm_seq_item*> analysis_port;

    comm_monitor(uvm::uvm_component_name name = "comm_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {}

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;  // Suppress unused parameter warning

        UVM_INFO("COMM_MONITOR", "Comm monitor started", uvm::UVM_LOW);
        // Passive observer — no objection needed
    }

    void sample_and_send(unsigned int command_type, float command_value) {
        comm_seq_item* seq_item = new comm_seq_item("comm_sample");
        seq_item->command_type = command_type;
        seq_item->command_value = command_value;

        UVM_INFO("COMM_MONITOR", "Sampled command values: command_type=" + std::to_string(seq_item->command_type) +
                 ", command_value=" + std::to_string(seq_item->command_value), uvm::UVM_LOW);

        analysis_port.write(seq_item);
    }
};

#endif // COMM_MONITOR_H