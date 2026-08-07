// ============================================================
// control_input_item.h — Transaction item carrying the (temperature,
// humidity, target) triple that control_task actually consumed in one
// iteration, from hal_bridge to the scoreboard's coherence check.
// ============================================================

#ifndef CONTROL_INPUT_ITEM_H
#define CONTROL_INPUT_ITEM_H

#include <uvm>

class control_input_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(control_input_item);

    float temperature;
    float humidity;
    float target;
    unsigned int sequence_id;
    unsigned int task_id;

    control_input_item(std::string name = "control_input_item")
        : uvm::uvm_sequence_item(name),
          temperature(0.0f), humidity(0.0f), target(0.0f),
          sequence_id(0), task_id(0) {}

    void do_copy(const uvm::uvm_object& rhs) override {
        uvm::uvm_sequence_item::do_copy(rhs);
        const control_input_item& item = dynamic_cast<const control_input_item&>(rhs);
        temperature = item.temperature;
        humidity    = item.humidity;
        target      = item.target;
        sequence_id = item.sequence_id;
        task_id     = item.task_id;
    }
};

#endif // CONTROL_INPUT_ITEM_H
