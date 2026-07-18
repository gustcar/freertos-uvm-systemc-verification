// ============================================================
// comm_seq_item.h — Sequence item for UART command transactions
// ============================================================

#ifndef COMM_SEQ_ITEM_H
#define COMM_SEQ_ITEM_H

#include <uvm>

class comm_seq_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(comm_seq_item);
    
    unsigned int command_type; // CMD_NONE=0, CMD_TARGET=1, CMD_ENABLE=2, CMD_DISABLE=3, CMD_RESET=4
    float command_value; // Value associated with the command

    comm_seq_item(std::string name = "comm_seq_item")
        : uvm::uvm_sequence_item(name),
          command_type(0),
          command_value(0.0f) {}
        
    void do_copy(const uvm::uvm_object& rhs) override {
        const comm_seq_item& item = dynamic_cast<const comm_seq_item&>(rhs);
        command_type = item.command_type;
        command_value = item.command_value;
    }

    bool do_compare(const uvm::uvm_object& rhs, const uvm::uvm_comparer* comparer = nullptr) const override {
        (void)comparer; // Unused parameter
        const comm_seq_item& item = dynamic_cast<const comm_seq_item&>(rhs);
        return (command_type == item.command_type) && (command_value == item.command_value);
    }

    void do_print(const uvm::uvm_printer& printer) const override {
        printer.print_field_int("command_type", command_type, 32, uvm::UVM_DEC);
        printer.print_real("command_value", command_value);
    }

    std::string convert2string() const override {
        std::ostringstream message;
        message << "comm_seq_item: { command_type: " << command_type
                << ", command_value: " << command_value << " }";
        return message.str();
    }
};
#endif // COMM_SEQ_ITEM_H