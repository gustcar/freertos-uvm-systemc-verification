// ============================================================
// env.h — Minimal verification environment
// ============================================================

#ifndef ENV_H
#define ENV_H

#include <systemc>
#include <uvm>
#include "../agents/sensor_agent/sensor_agent.h"
#include "../agents/comm_agent/comm_agent.h"
#include "../agents/actuator_agent/actuator_agent.h"
#include "../scoreboard/concurrency_sb.h"
#include "../coverage/coverage_bins.h"
#include "../hal_bridge/dut_bridge.h"
#include "../agents/mutex_wait_agent/mutex_wait_agent.h"
#include "../agents/control_input_agent/control_input_agent.h"
#include "../agents/logger_agent/logger_agent.h"

class env : public uvm::uvm_env {
public:
    UVM_COMPONENT_UTILS(env);

    sensor_agent*       sensor_agt;
    comm_agent*         comm_agt;
    actuator_agent*     actuator_agt;
    mutex_wait_agent*   mutex_wait_agt;
    control_input_agent* control_input_agt;
    logger_agent*       logger_agt;
    concurrency_sb*     scoreboard;
    coverage_bins*      cov;
    dut_bridge*         bridge;

    env(uvm::uvm_component_name name = "env")
        : uvm::uvm_env(name),
          sensor_agt(nullptr),
          comm_agt(nullptr),
          actuator_agt(nullptr),
          mutex_wait_agt(nullptr),
          control_input_agt(nullptr),
          logger_agt(nullptr),
          scoreboard(nullptr),
          cov(nullptr),
          bridge(nullptr) {}

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_env::build_phase(phase);
        sensor_agt   = sensor_agent::type_id::create("sensor_agt", this);
        comm_agt     = comm_agent::type_id::create("comm_agt", this);
        actuator_agt = actuator_agent::type_id::create("actuator_agt", this);
        mutex_wait_agt = mutex_wait_agent::type_id::create("mutex_wait_agt", this);
        control_input_agt = control_input_agent::type_id::create("control_input_agt", this);
        logger_agt   = logger_agent::type_id::create("logger_agt", this);
        scoreboard   = concurrency_sb::type_id::create("scoreboard", this);
        cov          = coverage_bins::type_id::create("cov", this);
        bridge       = dut_bridge::type_id::create("bridge", this);
        UVM_INFO("ENV", "Environment built", uvm::UVM_NONE);
    }

    void connect_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_env::connect_phase(phase);
        // connect sensor monitor to scoreboard (sensor data integrity)
        sensor_agt->monitor->analysis_port.connect(scoreboard->sensor_analysis_export);
        // connect actuator monitor to scoreboard (PWM/GPIO validation)
        actuator_agt->monitor->analysis_port.connect(scoreboard->actuator_analysis_export);
        // connect comm monitor to scoreboard
        comm_agt->monitor->analysis_port.connect(scoreboard->comm_analysis_export);
        mutex_wait_agt->monitor->analysis_port.connect(scoreboard->mutex_wait_analysis_export);
        // connect control-input monitor to scoreboard (torn-read coherence)
        control_input_agt->monitor->analysis_port.connect(scoreboard->control_input_analysis_export);
        // connect logger monitor to scoreboard (deadlock heartbeat, task 5/5)
        logger_agt->monitor->analysis_port.connect(scoreboard->logger_analysis_export);
        // connect sensor monitor to coverage collector
        sensor_agt->monitor->analysis_port.connect(cov->sensor_analysis_export);
        // connect actuator monitor to coverage collector
        actuator_agt->monitor->analysis_port.connect(cov->actuator_analysis_export);
        comm_agt->monitor->analysis_port.connect(cov->comm_analysis_export);
        hal_bridge::set_monitors(
            sensor_agt->monitor,
            actuator_agt->monitor,
            comm_agt->monitor,
            mutex_wait_agt->monitor,
            control_input_agt->monitor,
            logger_agt->monitor
        );
    }
};

#endif