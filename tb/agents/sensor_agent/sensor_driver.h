// ============================================================
// sensor_driver.h — Drives sensor values via global variables
// that HAL callbacks read. Stimulates the DUT's ADC input.
// ============================================================

#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <systemc>
#include <uvm>
#include "sensor_seq_item.h"

// Globals the HAL adc callback reads
extern float drv_temperature;
extern float drv_humidity;

class sensor_driver : public uvm::uvm_driver<sensor_seq_item> {
public:
    UVM_COMPONENT_UTILS(sensor_driver);

    sensor_driver(uvm::uvm_component_name name) : uvm::uvm_driver<sensor_seq_item>(name) {}

    virtual void run_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Unused parameter
        
        sensor_seq_item* seq_item = nullptr;

        while (true) {
            this->seq_item_port->get_next_item(seq_item);

            drv_temperature = seq_item->temperature;
            drv_humidity = seq_item->humidity;
            this->seq_item_port->item_done();
        }
    }

};

inline float drv_temperature = 25.0f;
inline float drv_humidity = 50.0f;

#endif // SENSOR_DRIVER_H