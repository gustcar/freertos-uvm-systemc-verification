// ============================================================
// logger_heartbeat_item.h — Transaction item carrying a liveness
// heartbeat from the DUT's logger_task (which otherwise produces no
// observable output) to the scoreboard's deadlock detector.
// ============================================================

#ifndef LOGGER_HEARTBEAT_ITEM_H
#define LOGGER_HEARTBEAT_ITEM_H

#include <uvm>

class logger_heartbeat_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(logger_heartbeat_item);

    unsigned int task_id;
    unsigned int sequence_id;
    // When true, this is a task-completion signal (the task's thread returned),
    // not a liveness heartbeat — the scoreboard forwards it to the deadlock
    // detector's mark_finished() so the task is excluded from stall checks.
    bool finished;

    logger_heartbeat_item(std::string name = "logger_heartbeat_item")
        : uvm::uvm_sequence_item(name),
          task_id(0), sequence_id(0), finished(false) {}

    void do_copy(const uvm::uvm_object& rhs) override {
        uvm::uvm_sequence_item::do_copy(rhs);
        const logger_heartbeat_item& item = dynamic_cast<const logger_heartbeat_item&>(rhs);
        task_id = item.task_id;
        sequence_id = item.sequence_id;
        finished = item.finished;
    }
};

#endif // LOGGER_HEARTBEAT_ITEM_H
