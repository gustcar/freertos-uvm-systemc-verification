// ============================================================
// comm_driver.h — Drives UART commands via global variables
// ============================================================

#ifndef COMM_DRIVER_H
#define COMM_DRIVER_H

#include <systemc>
#include <uvm>
#include "comm_seq_item.h"

extern unsigned int dvr_command_type;
extern float dvr_command_value;

class comm_driver : public uvm::uvm_driver<comm_seq_item> {
public:
    UVM_COMPONENT_UTILS(comm_driver);

    comm_driver(uvm::uvm_component_name name) : uvm::uvm_driver<comm_seq_item>(name) {}

    void run_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        comm_seq_item* item = nullptr;

        while (true) {
            this->seq_item_port->get_next_item(*item);

            dvr_command_type = item->command_type;
            dvr_command_value = item->command_value;

            UVM_INFO("COMM_DRIVER", "Injecting: " + 
               item->convert2string(), uvm::UVM_MEDIUM);

            this->seq_item_port->item_done();
        }
    }
};

inline unsigned int dvr_command_type = 0;
inline float dvr_command_value = 0.0f;

#endif // COMM_DRIVER_H