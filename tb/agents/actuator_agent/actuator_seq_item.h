// ============================================================
// actuator_seq_item.h — Transaction item for actuator observations
// ============================================================

#ifndef ACTUATOR_SEQ_ITEM_H
#define ACTUATOR_SEQ_ITEM_H

#include <uvm>

class actuator_seq_item : public uvm::uvm_sequence_item {
public:
    UVM_OBJECT_UTILS(actuator_seq_item);

    enum actuator_type_t { PWM, GPIO } type;
    unsigned int pwm_channel;
    unsigned int pwm_duty_cycle;
    unsigned int gpio_pin;
    bool         gpio_state;
    unsigned int task_id;
    unsigned int sequence_id;

    actuator_seq_item(std::string name = "actuator_seq_item")
        : uvm::uvm_sequence_item(name),
          type(PWM),
          pwm_channel(0),
          pwm_duty_cycle(0),
          gpio_pin(0),
          gpio_state(false),
          task_id(0),
          sequence_id(0) {}
    
    std::string convert2string() const override {
        if(type == PWM) {
            return "PWM channel: " + std::to_string(pwm_channel) +
                   " pwm_duty_cycle: " + std::to_string(pwm_duty_cycle) + " %";
        } else {
            return "GPIO pin: " + std::to_string(gpio_pin) +
                   " gpio_state: " + (gpio_state ? "true" : "false");
        }
    }

    void do_copy(const uvm::uvm_object& rhs) override {
        const actuator_seq_item& item = dynamic_cast<const actuator_seq_item&>(rhs);
        type = item.type;
        pwm_channel = item.pwm_channel;
        pwm_duty_cycle = item.pwm_duty_cycle;
        gpio_pin = item.gpio_pin;
        gpio_state = item.gpio_state;
        task_id = item.task_id;
        sequence_id = item.sequence_id;
    }

    virtual bool do_compare(const uvm::uvm_object& rhs, const uvm::uvm_comparer* comparer = nullptr) const override {
        (void)comparer; // Unused parameter
        const actuator_seq_item& item = dynamic_cast<const actuator_seq_item&>(rhs);
        return (type == item.type) &&
               (pwm_channel == item.pwm_channel) &&
               (pwm_duty_cycle == item.pwm_duty_cycle) &&
               (gpio_pin == item.gpio_pin) &&
               (gpio_state == item.gpio_state);
    }

    virtual void do_print(const uvm::uvm_printer& printer) const override {
        printer.print_string("type", type == PWM ? "PWM" : "GPIO");
        printer.print_field_int("pwm_channel", pwm_channel, 32, uvm::UVM_DEC);
        printer.print_field_int("pwm_duty_cycle", pwm_duty_cycle, 32, uvm::UVM_DEC);
        printer.print_field_int("gpio_pin", gpio_pin, 32, uvm::UVM_DEC);
        printer.print_string("gpio_state", gpio_state ? "true" : "false");
    }
};

#endif // ACTUATOR_SEQ_ITEM_H