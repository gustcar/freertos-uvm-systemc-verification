// ============================================================
// comm_sequencer.h — Arbitrates sequence items
// ============================================================

#ifndef COMM_SEQUENCER_H
#define COMM_SEQUENCER_H

#include <systemc>
#include <uvm>
#include "comm_seq_item.h"

class comm_sequencer : public uvm::uvm_sequencer<comm_seq_item> {
public:
    UVM_COMPONENT_UTILS(comm_sequencer);

    comm_sequencer(uvm::uvm_component_name name = "comm_sequencer")
        : uvm::uvm_sequencer<comm_seq_item>(name) {}
};

#endif // COMM_SEQUENCER_H