// ============================================================
// actuator_monitor.h — Passive monitor observing PWM and GPIO
// outputs from the DUT via HAL callbacks
// ============================================================

#ifndef ACTUATOR_MONITOR_H
#define ACTUATOR_MONITOR_H

#include <systemc>
#include <uvm>
#include "actuator_seq_item.h"

class actuator_monitor : public uvm::uvm_monitor {
public:
    UVM_COMPONENT_UTILS(actuator_monitor);

    uvm::uvm_analysis_port<actuator_seq_item*> analysis_port;

    actuator_monitor(uvm::uvm_component_name name = "actuator_monitor")
        : uvm::uvm_monitor(name),
          analysis_port("analysis_port") {
            seq_item = new actuator_seq_item("pwm_observation");
          }

    void run_phase(uvm::uvm_phase& phase) {
        (void)phase;  // Suppress unused parameter warning

        UVM_INFO("ACTUATOR_MONITOR", "Actuator monitor started", uvm::UVM_LOW);
        // Passive observer — no objection needed
    }

    void observe_pwm(unsigned int channel, unsigned int duty_cycle) {
        // Create a new sequence item for PWM observation
        actuator_seq_item* seq_item = actuator_seq_item::type_id::create("pwm_observation");
        seq_item->type = actuator_seq_item::PWM;
        seq_item->pwm_channel = channel;
        seq_item->pwm_duty_cycle = duty_cycle;
        seq_item->gpio_pin = 0; // Not applicable for PWM
        seq_item->gpio_state = false; // Not applicable for PWM

        UVM_INFO("ACTUATOR_MONITOR", "Observed PWM: channel=" + std::to_string(channel) +
                 ", duty_cycle=" + std::to_string(duty_cycle) + " %", uvm::UVM_LOW);

        analysis_port.write(seq_item);
    }

    void observe_gpio(unsigned int pin, bool state) {
        // Create a new sequence item for GPIO observation
        actuator_seq_item* seq_item = new actuator_seq_item("gpio_observation");
        seq_item->type = actuator_seq_item::GPIO;
        seq_item->pwm_channel = 0; // Not applicable for GPIO
        seq_item->pwm_duty_cycle = 0; // Not applicable for GPIO
        seq_item->gpio_pin = pin;
        seq_item->gpio_state = state;

        UVM_INFO("ACTUATOR_MONITOR", "Observed GPIO: pin=" + std::to_string(pin) +
                 ", state=" + (state ? "true" : "false"), uvm::UVM_LOW);

        analysis_port.write(seq_item);
    }
};

#endif // ACTUATOR_MONITOR_H