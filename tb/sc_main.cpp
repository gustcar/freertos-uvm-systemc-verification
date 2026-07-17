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
#include <string>
#include "tests/base_test.h"

int sc_main(int argc, char* argv[]) {

    // sc_core::sc_report_handler::set_actions(
    //     sc_core::SC_INFO,
    //     sc_core::SC_DO_NOTHING
    // );

    UVM_INFO("SC_MAIN", "Running UVM-SystemC test", uvm::UVM_NONE);
    
    std::string test_name = "base_test";
    if(argc > 1) test_name = argv[1];

    uvm::run_test(test_name.c_str());

    return 0;
}