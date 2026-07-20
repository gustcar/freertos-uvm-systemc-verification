// ============================================================
// base_seq.h — Base sequence for sensor stimulus generation
// Generates randomized temperature and humidity values with
// small delays to simulate normal operating conditions.
// ============================================================

#ifndef BASE_SEQ_H
#define BASE_SEQ_H

#include <systemc>
#include <uvm>
#include <random>
#include "../agents/sensor_agent/sensor_seq_item.h"

class base_seq : public uvm::uvm_sequence<sensor_seq_item> {
public:
    UVM_OBJECT_UTILS(base_seq);

    unsigned int num_items;
    unsigned int seed;
    float temperature_min, temperature_max;
    float humidity_min, humidity_max;

    base_seq(const std::string& name = "base_seq")
        : uvm::uvm_sequence<sensor_seq_item>(name),
          num_items(10), seed(42),
          temperature_min(20.0f), temperature_max(40.0f),
          humidity_min(30.0f), humidity_max(70.0f) {}

    virtual void body() override {
        init_randomizer();

        for (unsigned int i = 0; i < num_items; ++i) {
            sensor_seq_item* item = sensor_seq_item::type_id::create("item");
            start_item(item);
            randomize_item(item);            
            finish_item(item);
            sc_core::wait(10, sc_core::SC_NS); // Simulate a small delay
        }
    }

protected:
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist_temp;
    std::uniform_real_distribution<float> dist_humid;

    void init_randomizer() {
        std::random_device rd;
        gen.seed(seed ? seed : rd());
        dist_temp = std::uniform_real_distribution<float>(temperature_min, temperature_max);
        dist_humid = std::uniform_real_distribution<float>(humidity_min, humidity_max);
    }

    void randomize_item(sensor_seq_item* item) {
        if(item) {
            item->temperature = dist_temp(gen);
            item->humidity = dist_humid(gen);
        }
    }
};

#endif // BASE_SEQ_H