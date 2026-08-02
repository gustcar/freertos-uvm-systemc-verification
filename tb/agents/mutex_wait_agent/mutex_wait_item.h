// ============================================================
// mutex_wait_item.h — Transaction item carrying mutex wait/holder
// data from hal_bridge to the scoreboard's priority_inversion_checker.
// ============================================================

#ifndef MUTEX_WAIT_ITEM_H
#define MUTEX_WAIT_ITEM_H

#include <uvm>

class mutex_wait_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(mutex_wait_item);

    unsigned int task_id;
    unsigned int mutex_id;
    unsigned int wait_ms;
    unsigned int holder_task_id;

    mutex_wait_item(std::string name = "mutex_wait_item")
        : uvm::uvm_sequence_item(name),
          task_id(0), mutex_id(0), wait_ms(0), holder_task_id(0) {}

    void do_copy(const uvm::uvm_object& rhs) override {
        uvm::uvm_sequence_item::do_copy(rhs);
        const mutex_wait_item& item = dynamic_cast<const mutex_wait_item&>(rhs);
        task_id = item.task_id;
        mutex_id = item.mutex_id;
        wait_ms = item.wait_ms;
        holder_task_id = item.holder_task_id;
    }
};

#endif // MUTEX_WAIT_ITEM_H