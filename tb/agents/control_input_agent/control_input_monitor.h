// ============================================================
// control_input_monitor.h — Passive monitor observing the control-input
// triples reported by the DUT's control task via HAL. Forwards them to
// the scoreboard for torn-read coherence checking.
// ============================================================

#ifndef CONTROL_INPUT_MONITOR_H
#define CONTROL_INPUT_MONITOR_H

#include <systemc>
#include <uvm>
#include "control_input_item.h"

class control_input_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(control_input_monitor);

    uvm::uvm_analysis_port<control_input_item*> analysis_port;

    control_input_monitor(uvm::uvm_component_name name = "control_input_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {}

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;
        UVM_INFO("CONTROL_INPUT_MONITOR", "Control input monitor started", uvm::UVM_LOW);
    }

    void sample_and_send(float temperature, float humidity, float target,
                         unsigned int sequence_id, unsigned int task_id) {
        control_input_item* item = control_input_item::type_id::create("control_input_sample");
        item->temperature = temperature;
        item->humidity    = humidity;
        item->target      = target;
        item->sequence_id = sequence_id;
        item->task_id     = task_id;

        analysis_port.write(item);
    }
};

#endif // CONTROL_INPUT_MONITOR_H
