// ============================================================================
// sensor_seq_item.h - Sequence item representing sensor transaction data
// ============================================================================

#ifndef SENSOR_SEQ_ITEM_H
#define SENSOR_SEQ_ITEM_H

#include <uvm>

class sensor_seq_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(sensor_seq_item);

    float temperature;
    float humidity;

    sensor_seq_item(std::string name = "sensor_seq_item")
        : uvm::uvm_sequence_item(name), temperature(25.0f), humidity(50.0f) {}

    virtual void do_copy(const uvm::uvm_object& rhs) override {
        const sensor_seq_item& aux = dynamic_cast<const sensor_seq_item&>(rhs);
            temperature = aux.temperature;
            humidity = aux.humidity;
    }

    virtual bool do_compare(const uvm::uvm_object& rhs, const uvm::uvm_comparer* comparer = nullptr) const override {
        (void)comparer; // Unused parameter
        const sensor_seq_item& aux = dynamic_cast<const sensor_seq_item&>(rhs);
        return (temperature == aux.temperature) && (humidity == aux.humidity);
    }

    virtual void do_print(const uvm::uvm_printer& printer) const override {
        printer.print_real("temperature", temperature);
        printer.print_real("humidity", humidity);
    }
};

#endif