// ============================================================
// race_condition_seq.h — Sequence with extreme values near
// thresholds and minimal delays to expose race conditions
// in shared data access (Group A: vulnerable)
// ============================================================

#ifndef RACE_CONDITION_SEQ_H
#define RACE_CONDITION_SEQ_H

#include <systemc>
#include <uvm>
#include <random>
#include "base_seq.h"

class race_condition_seq : public base_seq {
public:
    UVM_OBJECT_UTILS(race_condition_seq);

    race_condition_seq(const std::string& name = "race_condition_seq")
        : base_seq(name) {
            num_items = 50;
            seed = 123; // Fixed seed for reproducibility 
    }

    void body() override {
        init_randomizer();

        std::uniform_real_distribution<float> dist_temp_near(34.0f, 37.0f);
        std::uniform_int_distribution<int> dist_delay(0, 2); // 0 to 2 ns

        for (unsigned int i = 0; i < num_items; ++i) {
            sensor_seq_item* item = sensor_seq_item::type_id::create("item");
            start_item(item);

            // Generate extreme values near thresholds
            if(i % 3 == 0) {
                item->temperature = dist_temp_near(gen);
            } else {
                item->temperature = dist_temp(gen);
            }
            item->humidity = dist_humid(gen);
            
            finish_item(item);
            sc_core::wait(dist_delay(gen), sc_core::SC_NS); // Minimal delay to increase race condition likelihood
        }
    } 
};

#endif // RACE_CONDITION_SEQ_H