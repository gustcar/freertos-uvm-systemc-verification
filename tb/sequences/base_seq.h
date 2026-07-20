// ============================================================
// base_seq.h — Base sequence for sensor stimulus generation
// Generates randomized temperature and humidity values with
// small delays to simulate normal operating conditions.
// ============================================================

#ifndef BASE_SEQ_H
#define BASE_SEQ_H

#include <systemc>
#include <uvm>
#include "../agents/sensor_agent/sensor_seq_item.h"
#include "../agents/sensor_agent/sensor_sequencer.h"

class base_seq : public uvm::uvm_sequence<sensor_seq_item> {
public:
    UVM_OBJECT_UTILS(base_seq);

    unsigned int num_items;

    base_seq(std::string name = "base_seq")
        : uvm::uvm_sequence<sensor_seq_item>(name), num_items(10) {}

    virtual void body() override {
        for (unsigned int i = 0; i < num_items; ++i) {
            sensor_seq_item* item = sensor_seq_item::type_id::create("item");
            start_item(item);

            item->temperature = 20.0f + (rand() % 200) / 10.0f; // 20 - 40 C
            item->humidity = 30.0f + (rand() % 400) / 10.0f; // 30 - 70 %
            
            finish_item(item);
            sc_core::wait(10, sc_core::SC_NS); // Simulate a small delay
        }
    }
};

#endif // BASE_SEQ_H