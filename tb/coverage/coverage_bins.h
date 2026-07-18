// ============================================================
// coverage_bins.h — Functional coverage bins for concurrency
// verification (priority x delay x task combinations)
// ============================================================

#ifndef COVERAGE_BINS_H
#define COVERAGE_BINS_H

#include <uvm>
#include <map>
#include <string>

class coverage_bins : public uvm::uvm_subscriber<unsigned int> {
public:
    UVM_COMPONENT_UTILS(coverage_bins);

    std::map<unsigned int, unsigned int> bins; // Map to hold coverage bins

    coverage_bins(uvm::uvm_component_name name = "coverage_bins")
        : uvm::uvm_subscriber<unsigned int>(name) {}

    void write(const unsigned int& bin_value) override {
        bins[bin_value]++;
    }

    void report_phase(uvm::uvm_phase& phase) override {
        (void)phase; // Suppress unused parameter warning
        UVM_INFO("COVERAGE_BINS", "Functional coverage bins:", uvm::UVM_LOW);
        for (const auto& bin : bins) {
            UVM_INFO(
                "COVERAGE_BINS",
                "Bin[" + std::to_string(bin.first) + "] hit " +
                std::to_string(bin.second) + " times: ",
                uvm::UVM_LOW
            );
        }
    }
};
#endif // COVERAGE_BINS_H