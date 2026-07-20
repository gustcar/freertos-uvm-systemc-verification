// ============================================================
// priority_inversion_seq.h — Sequence with alternating delays
// to stress the scheduler and create conditions for priority
// inversion between tasks accessing shared resources.
// ============================================================

#ifndef PRIORITY_INVERSION_SEQ_H
#define PRIORITY_INVERSION_SEQ_H

#include <systemc>
#include <uvm>
#include "base_seq.h"

class priority_inversion_seq : public base_seq {
public:
    UVM_OBJECT_UTILS(priority_inversion_seq);

    unsigned int delay_ms; // Delay in milliseconds to simulate task execution time

    priority_inversion_seq(const std::string& name = "priority_inversion_seq")
        : base_seq(name), delay_ms(5) {
            num_items = 30;
            seed = 456; // Fixed seed for reproducibility
    }

    void body() override {
        init_randomizer();

        for (unsigned int i = 0; i < num_items; ++i) {
            sensor_seq_item* item = sensor_seq_item::type_id::create("item");
            start_item(item);
            randomize_item(item);
            finish_item(item);
            if(i % 2 == 0) {
                sc_core::wait(delay_ms, sc_core::SC_NS); // Alternating delays to stress scheduler
            }
        }
    }
};

#endif // PRIORITY_INVERSION_SEQ_H