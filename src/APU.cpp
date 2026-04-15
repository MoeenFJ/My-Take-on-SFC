#include <bits/stdc++.h>
using namespace std;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int add24;

class APU
{
    //The Following is AI generated, for now.
public:
    // Step 1: Signal ready. Port 0 = $AA, Port 1 = $BB
    uint8 out_ports[4] = {0xAA, 0xBB, 0x00, 0x00};
    uint8 in_ports[4]  = {0x00, 0x00, 0x00, 0x00};
    
    // Memory to hold the actual uploaded audio program
    uint8 spc_ram[65536] = {0}; 

    // The IPL State Machine
    enum IplState {
        STATE_WAIT_CC,    // Step 2
        STATE_WAIT_ZERO,  // Step 7
        STATE_TRANSFER    // Step 8
    };
    
    IplState state = STATE_WAIT_CC;
    uint16 dest_addr = 0;
    uint8 expected_counter = 0;

    uint8 IORead(add24 address)
    {
        if (address >= 0x2140 && address <= 0x2143) {
            return out_ports[address - 0x2140];
        }
        return 0x00;
    }

    void IOWrite(add24 address, uint8 data)
    {
        if (address >= 0x2140 && address <= 0x2143) {
            int port = address - 0x2140;
            in_ports[port] = data;

            // The IPL heavily relies on Port 0 as the command/sync port
            if (port == 0) {
                switch (state) {
                    case STATE_WAIT_CC:
                        // Step 2: Loop until $CC is read from port 0.
                        // (This inherently ignores the $FF spam from your first screenshot)
                        if (data == 0xCC) {
                            process_new_block(data);
                        }
                        break;

                    case STATE_WAIT_ZERO:
                        // Step 7: Loop until read port 0 reads 0.
                        if (data == 0x00) {
                            expected_counter = 0; // Set 8-bit counter to 0
                            do_transfer_cycle(data); // Proceed to step 8
                        }
                        break;

                    case STATE_TRANSFER:
                        // Step 8: Transfer Loop
                        if (data == expected_counter) {
                            // "Wait until port 0 reads equal to the new counter, and repeat"
                            do_transfer_cycle(data);
                        } 
                        else {
                            // "If port 0 reads greater than the new counter..."
                            // Any out-of-sequence value triggers the end of the block.
                            // "...write back port 0 to acknowledge, transfer ends and return to step 4."
                            process_new_block(data);
                        }
                        break;
                }
            }
        }
    }

    void step()
    {
        // No step logic needed! The event-driven state machine in IOWrite
        // instantly satisfies the SNES CPU's wait loops.
    }

private:
    // Handles Steps 4, 5, and 6
    void process_new_block(uint8 port0_data) 
    {
        // Step 4: Read a 2 byte address from port 2 and 3
        dest_addr = in_ports[2] | (in_ports[3] << 8);
        
        // Step 5: Acknowledge (write value read from port 0 back to port 0)
        out_ports[0] = port0_data;
        
        // Step 6: Begin
        if (in_ports[1] == 0) {
            // "if 0 begin executing code at address"
            // For now, we just reset the state. A full emulator might boot the SPC700 CPU here.
            state = STATE_WAIT_CC; 
        } else {
            // "otherwise begin reading data (step 7)"
            state = STATE_WAIT_ZERO;
        }
    }

    // Handles the repetitive part of Step 8
    void do_transfer_cycle(uint8 port0_data) 
    {
        // "Read a byte from port 1 and write to the destination address."
        spc_ram[dest_addr] = in_ports[1];
        
        // "Write the value read from port 0 to port 0 to acknowledge receipt"
        out_ports[0] = port0_data;
        
        // "Increment the destination address, and increment the 8-bit counter"
        dest_addr++;
        expected_counter++; // This will naturally wrap 255 -> 0, mimicking 8-bit math
        
        state = STATE_TRANSFER;
    }
};
