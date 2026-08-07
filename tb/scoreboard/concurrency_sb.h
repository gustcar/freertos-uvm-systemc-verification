// ============================================================
// concurrency_sb.h — Scoreboard for concurrency verification
// Integrates data integrity, timing, and deadlock checkers.
// Reference model that recomputes expected actuator outputs
// from recent sensor snapshots to detect race-condition corruption.
// ============================================================

#ifndef CONCURRENCY_SB_H
#define CONCURRENCY_SB_H

#include <systemc>
#include <uvm>
#include <deque>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <unordered_set>
#include "../agents/sensor_agent/sensor_seq_item.h"
#include "../agents/actuator_agent/actuator_seq_item.h"
#include "../agents/comm_agent/comm_seq_item.h"
#include "../checker/data_integrity_checker.h"
#include "../checker/timing_checker.h"
#include "../checker/deadlock_detector.h"
#include "../agents/mutex_wait_agent/mutex_wait_item.h"
#include "../agents/control_input_agent/control_input_item.h"
#include "../agents/logger_agent/logger_heartbeat_item.h"
#include "../checker/priority_inversion_checker.h"

extern "C" {
#include "config.h"
#include "types.h"
}

// Reference model: recomputes what control_task should have
// produced from a given sensor reading, mirroring its logic.
struct sensor_snapshot {
    unsigned int sequence_id;
    float temperature;
    float humidity;
};


class concurrency_sb : public uvm::uvm_scoreboard {
public:
    UVM_COMPONENT_UTILS(concurrency_sb)

    uvm::uvm_analysis_imp<sensor_seq_item*, concurrency_sb>     sensor_analysis_export;
    uvm::uvm_analysis_imp<actuator_seq_item*, concurrency_sb> actuator_analysis_export;
    uvm::uvm_analysis_imp<comm_seq_item*, concurrency_sb>         comm_analysis_export;
    uvm::uvm_analysis_imp<mutex_wait_item*, concurrency_sb> mutex_wait_analysis_export;
    uvm::uvm_analysis_imp<control_input_item*, concurrency_sb> control_input_analysis_export;
    uvm::uvm_analysis_imp<logger_heartbeat_item*, concurrency_sb> logger_analysis_export;

    data_integrity_checker* data_integrity_chk;
    timing_checker*         timing_chk;
    deadlock_detector*      deadlock_chk;
    priority_inversion_checker* prio_inv_chk;

    unsigned int mismatches;
    unsigned int actuator_checks;
    unsigned int torn_reads;         // incoherent control-input triples (Group A races)
    unsigned int coherence_checks;   // control-input triples examined (post warm-up)

    concurrency_sb(uvm::uvm_component_name name = "concurrency_sb")
        : uvm::uvm_scoreboard(name),
          sensor_analysis_export("sensor_analysis_export", this),
          actuator_analysis_export("actuator_analysis_export", this),
          comm_analysis_export("comm_analysis_export", this),
          mutex_wait_analysis_export("mutex_wait_analisys_export", this),
          control_input_analysis_export("control_input_analysis_export", this),
          logger_analysis_export("logger_analysis_export", this),
          data_integrity_chk(nullptr),
          timing_chk(nullptr),
          deadlock_chk(nullptr),
          prio_inv_chk(nullptr),
          mismatches(0),
          actuator_checks(0),
          torn_reads(0),
          coherence_checks(0),
          modeled_target_temp(TEMP_TARGET_DEFAULT) {
        // Seed with the DUT's reset state (shared_data_reset: 0.0/0.0) so a
        // control read that lands before the first sensor write is treated as
        // coherent, not a torn read.
        seen_pairs.insert(pair_key(0.0f, 0.0f));
    }

    void build_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::build_phase(phase);
        data_integrity_chk = data_integrity_checker::type_id::create("data_integrity_chk", this);
        timing_chk         = timing_checker::type_id::create("timing_chk", this);
        deadlock_chk       = deadlock_detector::type_id::create("deadlock_chk", this);
        prio_inv_chk       = priority_inversion_checker::type_id::create("prio_inv_chk", this);
    }

    void write(sensor_seq_item* item) {
        // 1. Data integrity
        data_integrity_chk->check_temp_range(item->temperature);
        data_integrity_chk->check_humidity_range(item->humidity);

        // 2. Timing: detect threshold crossing (consistent sim-time stamp)
        timing_chk->on_sensor(item->temperature, item->sim_time_ns);

        // 3. Deadlock: heartbeat + check
        deadlock_chk->heartbeat(item->task_id, item->timestamp_ns);
        deadlock_chk->check(item->timestamp_ns);

        // 4. Reference model: remember this reading for later actuator comparison
        // item->timestamp_ns = sequence_id in hal_bridge.h
        push_snapshot(item->temperature, item->humidity, item->timestamp_ns);
    }

    void write(actuator_seq_item* item) {
        deadlock_chk->heartbeat(item->task_id, item->sequence_id);
        deadlock_chk->check(item->sequence_id);
        // PWM range check
        if (item->type == actuator_seq_item::PWM) {
            if (item->pwm_duty_cycle > 100) {
                mismatches++;
                UVM_ERROR(
                    "SB_ACTUATOR",
                    "Invalid PWM duty cycle: " +
                    std::to_string(item->pwm_duty_cycle) + "%"
                );
            }
            pending_fan_duty = item->pwm_duty_cycle;
            has_pending_fan = true;

            if(has_pending_fan && has_pending_pump) {
                check_actuator_pair(pending_fan_duty, pending_pump_on);
                has_pending_fan = false;
                has_pending_pump = false;
            }
            // timing_chk->on_actuator_reaction();
        } else {
            if (item->gpio_pin == GPIO_PIN_PUMP) {
                pending_pump_on = item->gpio_state;
                has_pending_pump = true;

                if (has_pending_fan && has_pending_pump) {
                    check_actuator_pair(pending_fan_duty, pending_pump_on);
                    has_pending_fan = false;
                    has_pending_pump = false;
                }
            } else if(item->gpio_pin == GPIO_PIN_LED_ALARM) {
                check_alarm_pair(item->gpio_state);
                timing_chk->on_actuator_reaction(item->sim_time_ns);
            }
        }
    }

    void write(comm_seq_item* item) {
        deadlock_chk->heartbeat(item->task_id, item->sequence_id);
        deadlock_chk->check(item->sequence_id);
        if (item->command_type == CMD_TARGET || item->command_type == CMD_RESET) {
            modeled_target_temp = (item->command_type == CMD_RESET) ? TEMP_TARGET_DEFAULT : item->command_value;
            UVM_INFO(
                "SB_COMM",
                "Modeled target_temp updated to " + std::to_string(modeled_target_temp) + "°C",
                uvm::UVM_LOW
            );
        } else {
            UVM_INFO(
                "SB_COMM",
                "UART command type=" + std::to_string(item->command_type),
                uvm::UVM_LOW
            );
        }
    }

    void write(mutex_wait_item* item) {
        prio_inv_chk->record_wait(
            item->task_id,
            item->mutex_id,
            item->wait_ms,
            item->holder_task_id
        );
    }

    void write(control_input_item* item) {
        deadlock_chk->heartbeat(item->task_id, item->sequence_id);
        check_control_coherence(item->temperature, item->humidity, item->target);
    }

    void write(logger_heartbeat_item* item) {
        // Liveness only — logger_task produces no data to check, but its
        // heartbeat completes deadlock-detector coverage to all 5 tasks.
        deadlock_chk->heartbeat(item->task_id, item->sequence_id);
        deadlock_chk->check(item->sequence_id);
    }

    void report_phase(uvm::uvm_phase& phase) override {
        uvm::uvm_scoreboard::report_phase(phase);
        UVM_INFO(
            "SB_REPORT",
            "Final mismatches: " + std::to_string(mismatches) +
            " (out of " + std::to_string(actuator_checks) + " actuator checks)",
            uvm::UVM_LOW
        );
        UVM_INFO(
            "SB_COHERENCE_REPORT",
            "Torn reads: " + std::to_string(torn_reads) +
            " (out of " + std::to_string(coherence_checks) + " control-input coherence checks, " +
            std::to_string(seen_pairs.size()) + " unique sensor pairs seen)",
            uvm::UVM_LOW
        );
    }

private:
    // Window sized by SAMPLE COUNT, not simulated time — the DUT threads
    // run at real-CPU speed, so many sensor readings can share the same
    // 1ms drain batch. Using sequence_id (real production order) with a
    // small count-based window correctly bounds how "stale" a matching
    // reading is allowed to be.
    //
    // NOTE: this output-reconstruction check is a WEAK torn-read detector —
    // control_task's transfer function saturates (fan pins to 0/100, pump is
    // binary), so most torn reads produce the same output as a coherent read
    // and slip through regardless of window size. Direct torn-read detection
    // lives in check_control_coherence() via the control-input instrumentation.
    static constexpr unsigned int kWindowSize = 5;

    std::deque<sensor_snapshot> recent_snapshots;
    float modeled_target_temp;

    // Every (temperature, humidity) pair ever seen together in one real sensor
    // reading. A control input whose pair is absent here was torn across two
    // readings — the Group A race we want to expose.
    std::unordered_set<uint64_t> seen_pairs;

    // Skip the first few control inputs so seen_pairs can populate before we
    // start judging — avoids cold-start false positives from the initial
    // reset state (addresses the "cold-start" concern directly).
    static constexpr unsigned int kCoherenceWarmup = 20;
    unsigned int coherence_warmup_seen = 0;
    
    // CHANGED: pairing state for joint fan/pump comparison, replaces the
    // removed on_fan_observed/on_pump_observed split-check helpers.
    unsigned int pending_fan_duty = 0;
    bool pending_pump_on = false;
    bool has_pending_fan = false;
    bool has_pending_pump = false;

    void push_snapshot(float temperature, float humidity, unsigned int timestamp_ns) {
        recent_snapshots.push_back({timestamp_ns, temperature, humidity});
        //prune_old_snapshots(timestamp_ns);
        if(recent_snapshots.size() > kWindowSize) {
            recent_snapshots.pop_front();
        }
        // Record every (temperature, humidity) pair that ever co-occurred in a
        // single real sensor reading. check_control_coherence() consults this
        // set: a control input whose pair was never seen here is a torn read.
        seen_pairs.insert(pair_key(temperature, humidity));
    }

    // Encode a (temperature, humidity) pair as a 64-bit key by concatenating
    // the raw float bit patterns. Values propagate DUT->TB by pure copy (no
    // arithmetic), so exact bit equality is the right coherence test and the
    // key never collides for genuinely distinct pairs.
    static uint64_t pair_key(float temperature, float humidity) {
        uint32_t t_bits, h_bits;
        std::memcpy(&t_bits, &temperature, sizeof(t_bits));
        std::memcpy(&h_bits, &humidity, sizeof(h_bits));
        return (static_cast<uint64_t>(t_bits) << 32) | h_bits;
    }

    // Mirrors control_task.c fan control logic exactly.
    static uint8_t compute_expected_fan_duty(float temperature, float target) {
        float error = temperature - target;
        uint8_t duty;
        if (error > FAN_TEMP_TOLERANCE) {
            duty = FAN_SPEED_MAX;
        } else if (error < -FAN_TEMP_TOLERANCE) {
            duty = FAN_SPEED_MIN;
        } else {
            duty = (uint8_t)(fabsf(error) * 5.0f);
            if (duty > FAN_SPEED_MAX) duty = FAN_SPEED_MAX;
        }
        return duty;
    }

    // Mirrors control_task.c pump control logic exactly.
    static bool compute_expected_pump_state(float humidity) {
        return humidity < HUMIDITY_MIN_LIMIT;
    }

    void check_actuator_pair(unsigned int observed_fan_duty, bool observed_pump_on) {
        if (recent_snapshots.empty()) return;

        actuator_checks++;
        for (const auto& snap : recent_snapshots) {
            uint8_t expected_fan = compute_expected_fan_duty(snap.temperature, modeled_target_temp);
            bool expected_pump = compute_expected_pump_state(snap.humidity);

            if (expected_fan == observed_fan_duty && expected_pump == observed_pump_on) {
                return; // Match — a single real reading explains both outputs
            }
        }

        mismatches++;
        UVM_ERROR("SB_REFMODEL",
            "actuator pair (fan=" + std::to_string(observed_fan_duty) +
            ", pump=" + std::to_string(observed_pump_on) +
            ") does not match any recent sensor snapshot pair — "
            "possible torn read of temp+humidity in control_task.");
    }

    static bool compute_expected_alarm_led(float temperature, float humidity) {
        if(temperature >= TEMP_CRITICAL_LIMIT) return true;
        if(temperature >= TEMP_ALARM_LIMIT || humidity >= HUMIDITY_MAX_LIMIT || humidity <= HUMIDITY_MIN_LIMIT) return true;
        return false;
    }

    void check_alarm_pair(bool observed_led_state) {
        if(recent_snapshots.empty()) return;
        actuator_checks++;
        for(const auto& snap : recent_snapshots) {
            bool expected_led = compute_expected_alarm_led(snap.temperature, snap.humidity);
            if(expected_led == observed_led_state) return;
        }
        mismatches++;
        UVM_ERROR(
            "SB_REFMODEL_ALARM",
            "alarm LED=" + std::string(observed_led_state ? "true" : "false") +
            " does not match any recent sensor snapshot - possible torn read of temp+humidity in alarm_task."
        );
    }

    // Direct torn-read detector: verifies that the (temperature, humidity)
    // pair control_task actually consumed ever co-occurred in a single real
    // sensor reading. Unlike the output-reconstruction checks above, this is
    // immune to the control law's output saturation — it inspects the inputs
    // themselves. Group B (mutex-protected reads) can never produce an unseen
    // pair; Group A can, and that is exactly the race we surface.
    void check_control_coherence(float temperature, float humidity, float target) {
        (void)target;   // target coherence tracked separately (few comm cmds); not judged here yet

        // Warm-up: let seen_pairs fill before judging.
        if (coherence_warmup_seen < kCoherenceWarmup) {
            coherence_warmup_seen++;
            return;
        }

        coherence_checks++;
        if (seen_pairs.find(pair_key(temperature, humidity)) != seen_pairs.end()) {
            return;   // pair was a real sensor reading — coherent
        }

        torn_reads++;
        mismatches++;
        UVM_ERROR(
            "SB_COHERENCE",
            "control_task consumed an incoherent (temp=" + std::to_string(temperature) +
            ", humidity=" + std::to_string(humidity) + ") pair that never co-occurred in a "
            "single sensor reading — torn read of unprotected shared sensor_data (Group A race)."
        );
    }
};

#endif // CONCURRENCY_SB_H