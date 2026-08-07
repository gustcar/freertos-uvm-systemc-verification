// ============================================================
// logger_monitor.h — Passive monitor observing logger_task liveness
// heartbeats reported by the DUT via HAL (hal_log_write). Forwards
// them to the scoreboard so the deadlock detector tracks all 5 tasks.
// ============================================================

#ifndef LOGGER_MONITOR_H
#define LOGGER_MONITOR_H

#include <systemc>
#include <uvm>
#include "logger_heartbeat_item.h"

class logger_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(logger_monitor);

    uvm::uvm_analysis_port<logger_heartbeat_item*> analysis_port;

    logger_monitor(uvm::uvm_component_name name = "logger_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {}

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;
        UVM_INFO("LOGGER_MONITOR", "Logger monitor started", uvm::UVM_LOW);
    }

    void sample_and_send(unsigned int task_id, unsigned int sequence_id) {
        logger_heartbeat_item* item = logger_heartbeat_item::type_id::create("logger_heartbeat");
        item->task_id = task_id;
        item->sequence_id = sequence_id;

        analysis_port.write(item);
    }
};

#endif // LOGGER_MONITOR_H
