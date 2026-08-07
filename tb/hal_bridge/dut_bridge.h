#ifndef TB_DUT_BRIDGE_H
#define TB_DUT_BRIDGE_H

#include <systemc>
#include <uvm>
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include "hal_bridge.h"

extern "C" {
#include "config.h"
#include "types.h"
#include "hal.h"

#if defined(DUT_GROUP_A)
    void sensor_task(void);
    void control_task(void);
    void comm_task(void);
    void alarm_task(void);
    void logger_task(void);
    void shared_data_reset(void);
#elif defined(DUT_GROUP_B)
    void sensor_task_safe(void);
    void control_task_safe(void);
    void comm_task_safe(void);
    void alarm_task_safe(void);
    void logger_task_safe(void);
    void shared_data_reset(void);
    void shared_data_init_mutexes(void);
    void shared_data_destroy_mutexes(void);
#else
    #error "Define DUT_GROUP_A or DUT_GROUP_B when compiling the testbench"
#endif
}

// opt-in core-pinning flag for the priority inversion demonstration.
// OFF by default so it never affects race_condition_test / protected_test
// (mutex A vs B comparison). Only priority_inversion_test enables this,
// pinning logger_task/alarm_task/comm_task to the same core to reproduce
// the classic single-core-style CPU contention scenario.
namespace dut_bridge_detail {
    inline bool& pin_priority_inversion_tasks() {
        static bool enabled = false;
        return enabled;
    }
}

class dut_bridge : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(dut_bridge)

    dut_bridge(uvm::uvm_component_name name = "dut_bridge")
        : uvm::uvm_component(name) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_component::build_phase(phase);
        hal_init(MODE_UVM_SYSTEMC);
        hal_register_adc(hal_bridge_adc_read);
        hal_register_pwm(hal_bridge_pwm_set);
        hal_register_gpio(hal_bridge_gpio_write, hal_bridge_gpio_read);
        hal_register_uart(hal_bridge_uart_rx);
        hal_register_log(hal_bridge_log_write);
        hal_register_mutex_wait(hal_bridge_mutex_wait);
        hal_register_control_inputs(hal_bridge_control_inputs);
    }

    void run_phase(uvm::uvm_phase& phase) override {
        phase.raise_objection(this, "dut_bridge: DUT running");

        srand(42);
#if defined(DUT_GROUP_B)
        shared_data_init_mutexes();
#endif
        shared_data_reset();

        struct task_entry { const char* name; void (*fn)(void); };
#if defined(DUT_GROUP_A)
        task_entry tasks[5] = {
            {"alarm_task",   alarm_task},
            {"sensor_task",  sensor_task},
            {"control_task", control_task},
            {"comm_task",    comm_task},
            {"logger_task",  logger_task},
        };
#else
        task_entry tasks[5] = {
            {"alarm_task_safe",   alarm_task_safe},
            {"sensor_task_safe",  sensor_task_safe},
            {"control_task_safe", control_task_safe},
            {"comm_task_safe",    comm_task_safe},
            {"logger_task_safe",  logger_task_safe},
        };
#endif
        static void (*task_functions[5])(void);
        pthread_t threads[5];
        for (int i = 0; i < 5; ++i) task_functions[i] = tasks[i].fn;

        for (int i = 0; i < 5; ++i) {
            hal_bridge::active_dut_threads()++;
            pthread_create(
                &threads[i],
                nullptr,
                [](void* arg) -> void* {
                    int idx = static_cast<int>(reinterpret_cast<intptr_t>(arg));
                    task_functions[idx]();
                    hal_bridge::active_dut_threads()--;
                    return nullptr;
                },
                reinterpret_cast<void*>(static_cast<intptr_t>(i))
            );
            
            if(dut_bridge_detail::pin_priority_inversion_tasks() && (i == 0 || i == 3 || i == 4)) {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(0, &cpuset);
                int rc = pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    UVM_WARNING("DUT_BRIDGE", "setaffinity failed for " + std::string(tasks[i].name) +
                                ", errno=" + std::to_string(rc));
                } else {
                    UVM_INFO("DUT_BRIDGE", "Pinned " + std::string(tasks[i].name) + " to core 0", uvm::UVM_LOW);
                }
                //pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
            }
            UVM_INFO("DUT_BRIDGE", std::string("Spawned ") + tasks[i].name, uvm::UVM_LOW);
        }

        // Poll the HAL event queue while DUT threads are alive; forward to monitors
        // from inside the SystemC thread only.
        while (hal_bridge::active_dut_threads() > 0) {
            // Publish current simulated time so DUT threads can stamp events
            // with a consistent clock (read via hal_get_sim_time_ns). No gate /
            // blocking — purely informational, so no deadlock risk.
            hal_set_sim_time_ns(
                static_cast<uint64_t>(sc_core::sc_time_stamp().to_seconds() * 1e9)
            );
            hal_bridge::drain_to_monitors(
                static_cast<unsigned int>(sc_core::sc_time_stamp().to_seconds() * 1e9)
            );
            sc_core::wait(1, sc_core::SC_MS);
        }
        hal_set_sim_time_ns(
            static_cast<uint64_t>(sc_core::sc_time_stamp().to_seconds() * 1e9)
        );
        hal_bridge::drain_to_monitors(
            static_cast<unsigned int>(sc_core::sc_time_stamp().to_seconds() * 1e9)
        );

        for (int i = 0; i < 5; ++i) pthread_join(threads[i], nullptr);

#if defined(DUT_GROUP_B)
        shared_data_destroy_mutexes();
#endif

        UVM_INFO("DUT_BRIDGE", "All DUT threads completed", uvm::UVM_LOW);
        phase.drop_objection(this, "dut_bridge: DUT completed");
    }
};

#endif // TB_DUT_BRIDGE_H