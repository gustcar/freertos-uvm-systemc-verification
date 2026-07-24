// ============================================================================
// sensor_seq_item.h - Sequence item representing sensor transaction data
// Fields: temperature, humidity, RTOS metadata (timestamp, task priority,
// task id) for concurrency coverage tracking.
// ============================================================================

#ifndef SENSOR_SEQ_ITEM_H
#define SENSOR_SEQ_ITEM_H

#include <uvm>

class sensor_seq_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(sensor_seq_item);

    float temperature;
    float humidity;
    unsigned int timestamp_ns;
    int task_priority;
    unsigned int task_id;

    sensor_seq_item(std::string name = "sensor_seq_item")
        : uvm::uvm_sequence_item(name),
          temperature(25.0f),
          humidity(50.0f),
          timestamp_ns(0),
          task_priority(0),
          task_id(0) {}

    virtual void do_copy(const uvm::uvm_object& rhs) override {
        uvm::uvm_sequence_item::do_copy(rhs);
        const sensor_seq_item& item = dynamic_cast<const sensor_seq_item&>(rhs);
            temperature = item.temperature;
            humidity = item.humidity;
            timestamp_ns = item.timestamp_ns;
            task_priority = item.task_priority;
            task_id = item.task_id;
    }

    virtual bool do_compare(const uvm::uvm_object& rhs, const uvm::uvm_comparer* comparer = nullptr) const override {
        (void)comparer; // Unused parameter
        const sensor_seq_item& item = dynamic_cast<const sensor_seq_item&>(rhs);
        return (temperature == item.temperature) &&
               (humidity == item.humidity) &&
               (timestamp_ns == item.timestamp_ns) &&
               (task_priority == item.task_priority) &&
               (task_id == item.task_id);
    }

    virtual void do_print(const uvm::uvm_printer& printer) const override {
        printer.print_real("temperature", temperature);
        printer.print_real("humidity", humidity);
        printer.print_field_int("timestamp_ns", timestamp_ns);
        printer.print_field_int("task_priority", task_priority);
        printer.print_field_int("task_id", task_id);
    }
};

#endif