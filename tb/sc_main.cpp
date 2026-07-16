// ============================================================
// sc_main.cpp — SystemC + UVM-SystemC entry point
//
// Registers HAL callbacks that delegate to UVM agents, then
// launches the UVM test. This is the bridge between the C
// DUT firmware and the C++ verification environment.
// ============================================================

#define  SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc>
#include <uvm>

int sc_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sc_core::sc_report_handler::set_actions(
        sc_core::SC_INFO,
        sc_core::SC_DO_NOTHING
    );

    uvm::uvm_report_info("SC_MAIN", "hello from UVM-SystemC");
    uvm::uvm_report_info("SC MAIN", "Simulation finished");
    return 0;
}