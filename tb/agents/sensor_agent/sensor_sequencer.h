// ============================================================
// sensor_sequencer.h — Arbitrates sequence items between
// sequences and the sensor driver
// ============================================================

#ifndef SENSOR_SEQUENCER_H
#define SENSOR_SEQUENCER_H

#include <systemc>
#include <uvm>
#include "sensor_seq_item.h"

class sensor_sequencer : public uvm::uvm_sequencer<sensor_seq_item> {
public:
    UVM_COMPONENT_UTILS(sensor_sequencer);

    sensor_sequencer(uvm::uvm_component_name name = "sensor_sequencer")
        : uvm::uvm_sequencer<sensor_seq_item>(name) {}
};

#endif // SENSOR_SEQUENCER_H