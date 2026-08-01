#ifndef TB_HAL_BRIDGE_H
#define TB_HAL_BRIDGE_H

#include <mutex>
#include <deque>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "../agents/sensor_agent/sensor_monitor.h"
#include "../agents/actuator_agent/actuator_monitor.h"
#include "../agents/comm_agent/comm_monitor.h"

extern "C" {
#include "config.h"
#include "types.h"
}

// Values driven by the TB sequencers/drivers (defined in sensor_driver.h / comm_driver.h)
extern float drv_temperature;
extern float drv_humidity;
extern unsigned int dvr_command_type;
extern float dvr_command_value;

enum class hal_event_type { SENSOR, PWM, GPIO, COMM };

struct hal_event {
    hal_event_type type;
    unsigned int sequence_id = 0;
    float temperature = 0.0f, humidity = 0.0f;
    unsigned int pwm_channel = 0, pwm_duty = 0;
    unsigned int gpio_pin = 0; bool gpio_state = false;
    unsigned int command_type = 0; float command_value = 0.0f;
    unsigned int task_id = 0;
};

// Thread-safe hand-off: DUT pthreads (producers) -> SystemC thread (consumer)
class hal_bridge {
public:
    static void push(hal_event ev) {
        std::lock_guard<std::mutex> lk(get_mutex());
        ev.sequence_id = next_sequence_id()++;
        queue().push_back(ev);
    }

    static void set_monitors(sensor_monitor* s, actuator_monitor* a, comm_monitor* c) {
        sensor_mon() = s; actuator_mon() = a; comm_mon() = c;
    }

    // Called ONLY from the SystemC thread (dut_bridge run_phase)
    static void drain_to_monitors(unsigned int sim_time_ns) {
        (void)sim_time_ns;
        std::deque<hal_event> local;
        {
            std::lock_guard<std::mutex> lk(get_mutex());
            std::swap(local, queue());
        }
        for (auto& ev : local) {
            switch (ev.type) {
                case hal_event_type::SENSOR:
                    if (sensor_mon())
                        sensor_mon()->sample_and_send(
                            ev.temperature,
                            ev.humidity,
                            ev.sequence_id,
                            PRIORITY_SENSOR,
                            ev.task_id
                        );
                    break;
                case hal_event_type::PWM:
                    if (actuator_mon()) actuator_mon()->observe_pwm(
                        ev.pwm_channel,
                        ev.pwm_duty,
                        ev.task_id,
                        ev.sequence_id
                    );
                    break;
                case hal_event_type::GPIO:
                    if (actuator_mon()) actuator_mon()->observe_gpio(ev.gpio_pin, ev.gpio_state, ev.task_id, ev.sequence_id);
                    break;
                case hal_event_type::COMM:
                    if (comm_mon()) comm_mon()->sample_and_send(ev.command_type, ev.command_value, ev.task_id, ev.sequence_id);
                    break;
            }
        }
    }

    static std::atomic<int>& active_dut_threads() {
        static std::atomic<int> n{0};
        return n;
    }

private:
    static std::mutex& get_mutex() {
        static std::mutex m;
        return m;
    }
    static std::deque<hal_event>& queue() {
        static std::deque<hal_event> q;
        return q;
    }
    static unsigned int& next_sequence_id() {
        static unsigned int id = 0;
        return id;
    }
    static sensor_monitor*& sensor_mon() {
        static sensor_monitor* p = nullptr;
        return p;
    }
    static actuator_monitor*& actuator_mon() {
        static actuator_monitor* p = nullptr;
        return p;
    }
    static comm_monitor*& comm_mon() {
        static comm_monitor* p = nullptr;
        return p;
    }
};

// ---- HAL callback bridge functions (C linkage — match hal.h typedefs exactly) ----
// Called from DUT pthreads. MUST NOT touch UVM/SystemC directly — only push to the queue.

extern "C" inline float hal_bridge_adc_read(uint8_t channel) {
    static thread_local float last_temp = 0.0f;   // only sensor_task calls the ADC
    if (channel == ADC_CH_TEMP) {
        last_temp = drv_temperature;
        return last_temp;
    }
    float humidity = drv_humidity;
    hal_event ev;
    ev.type = hal_event_type::SENSOR;
    ev.temperature = last_temp;
    ev.humidity = humidity;
    ev.task_id = PRIORITY_SENSOR;
    hal_bridge::push(ev);
    return humidity;
}

extern "C" inline void hal_bridge_pwm_set(uint8_t channel, uint8_t duty_percent) {
    hal_event ev;
    ev.type = hal_event_type::PWM;
    ev.pwm_channel = channel;
    ev.pwm_duty = duty_percent;
    ev.task_id = PRIORITY_CONTROL;
    hal_bridge::push(ev);
}

extern "C" inline void hal_bridge_gpio_write(uint8_t pin, bool value) {
    hal_event ev;
    ev.type = hal_event_type::GPIO;
    ev.gpio_pin = pin;
    ev.gpio_state = value;
    ev.task_id = (pin == GPIO_PIN_LED_ALARM) ? PRIORITY_ALARM : PRIORITY_CONTROL;
    hal_bridge::push(ev);
}

extern "C" inline bool hal_bridge_gpio_read(uint8_t pin) {
    (void)pin;
    return false;   // no DUT task currently reads GPIO back
}

extern "C" inline int hal_bridge_uart_rx(uint8_t* buf, uint16_t len) {
    if (dvr_command_type == CMD_NONE || len < sizeof(command_t)) return 0;

    command_t cmd;
    cmd.type = static_cast<command_type_t>(dvr_command_type);
    cmd.value = dvr_command_value;
    std::memcpy(buf, &cmd, sizeof(command_t));

    hal_event ev;
    ev.type = hal_event_type::COMM;
    ev.command_type = dvr_command_type;
    ev.command_value = dvr_command_value;
    ev.task_id = PRIORITY_COMM;
    hal_bridge::push(ev);
    
    dvr_command_type = CMD_NONE;   // consume — avoid re-injecting the same command every poll
    return static_cast<int>(sizeof(command_t));
}

extern "C" inline int hal_bridge_log_write(const char* data, uint16_t len) {
    std::fwrite(data, 1, len, stdout);   // pass-through; logger activity is out of TB scope
    return static_cast<int>(len);
}

#endif // TB_HAL_BRIDGE_H