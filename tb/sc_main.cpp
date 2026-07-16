// ============================================================
// sc_main.cpp — Entry point for UVM-SystemC testbench
//
// Main entry point that initializes SystemC simulation
// and runs the UVM verification environment.
//
// ============================================================

#include <systemc.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

// Forward declarations for UVM-SystemC (implemented in 5.2+)
class uvm_env;
class uvm_test;

SC_MODULE(tb_top) {
    void run_test() {

        std::cout << "[INFO] Testbench initialized successfully\n";
        std::cout << "  Testbench Framework Ready\n";
    }
    
    SC_HAS_PROCESS(tb_top);
    
    tb_top(sc_module_name name) : sc_module(name) {
        SC_THREAD(run_test);
    }
};

int sc_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "\n\nStarting UVM-SystemC Verification Environment...\n\n";
    
    tb_top top("testbench");
    top.run_test();
    
    std::cout << "\nSimulation finished.\n";
    return 0;
}