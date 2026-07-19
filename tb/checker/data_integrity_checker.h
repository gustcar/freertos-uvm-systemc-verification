// ============================================================
// data_integrity_checker.h — Inline checker for shared data
// consistency (temp/humidity coherent across tasks)
// ============================================================

#ifndef DATA_INTEGRITY_CHECKER_H
#define DATA_INTEGRITY_CHECKER_H

#include <systemc>
#include <uvm>

class data_integrity_checker : public uvm::uvm_component {
public:
    UVM_COMPONENT_UTILS(data_integrity_checker)

    unsigned int violations;
    unsigned int checks;

    data_integrity_checker(uvm::uvm_component_name name)
        : uvm::uvm_component(name), violations(0), checks(0) {}

    void check_temp_range(float temp) {
        if (temp < -10.0f || temp > 60.0f) {
            violations++;
            UVM_ERROR(
                "CHECKER",
                "Temperature out of range: " +
                std::to_string(temp)
            );
        }
        checks++;
    }

    void check_humidity_range(float humidity) {
        if (humidity < 0.0f || humidity > 100.0f) {
            violations++;
            UVM_ERROR(
                "CHECKER",
                "Humidity out of range: " +
                std::to_string(humidity)
            );
        }
        checks++;
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Suppress unused parameter warning
        UVM_INFO(
            "CHECKER",
            "Data Integrity Checker Report: " +
            std::to_string(checks) + " checks, " +
            std::to_string(violations) + " violations.",
            uvm::UVM_LOW
        );
    }
};

#endif // DATA_INTEGRITY_CHECKER_H