// ============================================================
// mutex_wait_monitor.h — Passive monitor observing mutex wait/
// holder events reported by Group B's DUT threads via HAL.
// ============================================================

#ifndef MUTEX_WAIT_MONITOR_H
#define MUTEX_WAIT_MONITOR_H

#include <systemc>
#include <uvm>
#include "mutex_wait_item.h"

class mutex_wait_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(mutex_wait_monitor);

    uvm::uvm_analysis_port<mutex_wait_item*> analysis_port;

    mutex_wait_monitor(uvm::uvm_component_name name = "mutex_wait_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {}

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;
        UVM_INFO("MUTEX_WAIT_MONITOR", "Mutex wait monitor started", uvm::UVM_LOW);
    }

    void sample_and_send(unsigned int task_id, unsigned int mutex_id, unsigned int wait_ms, unsigned int holder_task_id) {
        mutex_wait_item* item = mutex_wait_item::type_id::create("mutex_wait_sample");
        item->task_id = task_id;
        item->mutex_id = mutex_id;
        item->wait_ms = wait_ms;
        item->holder_task_id = holder_task_id;

        analysis_port.write(item);
    }
};

#endif // MUTEX_WAIT_MONITOR_H