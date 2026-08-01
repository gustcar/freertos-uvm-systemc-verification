// ============================================================
// comm_target_seq.h — Sequence driving CMD_TARGET commands to
// stress target_temp read/write concurrency between comm_task
// and control_task (Group A: vulnerable to torn/stale reads).
// ============================================================

#ifndef COMM_TARGET_SEQ_H
#define COMM_TARGET_SEQ_H

#include <systemc>
#include <uvm>
#include <random>
#include "../agents/comm_agent/comm_seq_item.h"

extern "C" {
#include "config.h"
#include "types.h"
}

class comm_target_seq : public uvm::uvm_sequence<comm_seq_item> {
public:
    UVM_OBJECT_UTILS(comm_target_seq);

    unsigned int num_items;
    unsigned int seed;

    comm_target_seq(const std::string& name = "comm_target_seq")
        : uvm::uvm_sequence<comm_seq_item>(name),
          num_items(200),
          seed(789) {}

    virtual void body() override {
        std::mt19937 gen(seed);
        // Values clustered near TEMP_TARGET_DEFAULT so control_task's
        // fan_duty decision is sensitive to whichever target_temp value
        // it happens to read — same principle as race_condition_seq.
        std::uniform_real_distribution<float> dist_target(
            TEMP_TARGET_DEFAULT - 3.0f,
            TEMP_TARGET_DEFAULT + 3.0f
        );
        std::uniform_int_distribution<int> dist_delay(0, 3);

        for (unsigned int i = 0; i < num_items; ++i) {
            comm_seq_item* item = comm_seq_item::type_id::create("target_cmd");
            start_item(item);
            item->command_type = CMD_TARGET;
            item->command_value = dist_target(gen);
            finish_item(item);
            sc_core::wait(dist_delay(gen), sc_core::SC_NS);
        }
    }
};

#endif // COMM_TARGET_SEQ_H