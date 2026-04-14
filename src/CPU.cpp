#include <bits/stdc++.h>
using namespace std;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int add24;

class FlagsRegister
{
    const uint8_t flagNMask = 0b10000000;
    const uint8_t flagVMask = 0b01000000;
    const uint8_t flagMMask = 0b00100000;
    const uint8_t flagXMask = 0b00010000;
    const uint8_t flagDMask = 0b00001000;
    const uint8_t flagIMask = 0b00000100;
    const uint8_t flagZMask = 0b00000010;
    const uint8_t flagCMask = 0b00000001;

public:
    bool N;
    bool V;
    bool M;
    bool X;
    bool D;
    bool I;
    bool Z;
    bool C;
    FlagsRegister()
    {
        N = V = M = X = D = I = Z = C = 0;
    }

    void SetMask(uint8_t mask)
    {
        N = (mask & flagNMask) ? 1 : N;
        V = (mask & flagVMask) ? 1 : V;
        M = (mask & flagMMask) ? 1 : M;
        X = (mask & flagXMask) ? 1 : X;
        D = (mask & flagDMask) ? 1 : D;
        I = (mask & flagIMask) ? 1 : I;
        Z = (mask & flagZMask) ? 1 : Z;
        C = (mask & flagCMask) ? 1 : C;
    }
    void ResetMask(uint8_t mask)
    {
        // cout << "Reset Mask : " << std::bitset<8>(mask) << endl;
        N = (mask & flagNMask) ? 0 : N;
        V = (mask & flagVMask) ? 0 : V;
        M = (mask & flagMMask) ? 0 : M;
        X = (mask & flagXMask) ? 0 : X;
        D = (mask & flagDMask) ? 0 : D;
        I = (mask & flagIMask) ? 0 : I;
        Z = (mask & flagZMask) ? 0 : Z;
        C = (mask & flagCMask) ? 0 : C;
    }

    operator uint8_t() const
    {
        return (N ? flagNMask : 0) | (V ? flagVMask : 0) | (M ? flagMMask : 0) | (X ? flagXMask : 0) | (D ? flagDMask : 0) | (I ? flagIMask : 0) | (Z ? flagZMask : 0) | (C ? flagCMask : 0);
    }

    FlagsRegister &operator=(const uint8_t &bits)
    {
        N = bits & flagNMask;
        V = bits & flagVMask;
        M = bits & flagMMask;
        X = bits & flagXMask;
        D = bits & flagDMask;
        I = bits & flagIMask;
        Z = bits & flagZMask;
        C = bits & flagCMask;
        return *this;
    }
};

class Register
{

public:
    uint8_t L;
    uint8_t H;

    const int &operator=(const int &bits)
    {
        this->H = bits >> 8;
        this->L = bits & 0x000000FF;
        return bits;
    }

    const int &operator=(const unsigned int &bits)
    {
        this->H = bits >> 8;
        this->L = bits & 0x000000FF;
        return bits;
    }
    const int &operator=(const uint16_t &bits)
    {
        this->H = bits >> 8;
        this->L = bits & 0x00FF;
        return bits;
    }

    const int &operator=(const uint8_t &bits)
    {
        this->H = 0;
        this->L = bits;
        return bits;
    }

    const bool operator[](const int &i)
    {
        return (i <= 7 ? (L & (1 << i)) : (H & (1 << (i - 8)))) >> i;
    }

    const uint16_t operator-=(const uint16_t &i)
    {
        *this = (uint16_t)*this - (uint16_t)i;

        return *this;
    }

    const uint16_t operator+=(const uint16_t &i)
    {
        *this = (uint16_t)*this + (uint16_t)i;

        return *this;
    }

    /*operator int() const
    {
        return (((int)this->H) << 8) + (int)this->L;
    }
    operator unsigned int() const
    {
        return (((unsigned int)this->H) << 8) + (unsigned int)this->L;
    }*/

    operator uint16_t() const
    {
        return (((uint16_t)this->H) << 8) + (uint16_t)this->L;
    }

    /*operator uint8_t() const
    {
        return this->L;
    }*/
};

struct CPURegisters
{
    Register C;   // accumulator 16 bit
    Register DBR; // data bank register 8 bit
    Register D;   // Direct 16 bit
    Register K;   // Program Bank 8 bit
    Register PC;  // PC 16 bit
    Register S;   // Stack 16 bit
    Register X;   // X 16 bit
    Register Y;   // Y 16 bit
};

using C65ReadByteFunc_t = uint8 (*)(add24);
using C65WriteByteFunc_t = void (*)(add24, uint8);
class CPU
{

    C65ReadByteFunc_t ReadByte;
    C65WriteByteFunc_t WriteByte;
    ofstream pcInstDump;

public:
    FlagsRegister flags; // Status Reg
    CPURegisters cregs;
    bool flagE;

    bool waitForInt = false;
    bool stop = false;
    add24 instAddress;

    add24 AddressingMode_16PTR(add24 lo) // ()
    {
        return (cregs.DBR << 16) | (ReadByte(lo + 1) << 8) | ReadByte(lo);
    }

    add24 AddressingMode_24PTR(add24 lo) // []
    {
        return (ReadByte(lo + 2) << 16) | (ReadByte(lo + 1) << 8) | ReadByte(lo);
    }

    add24 AddressingMode_STK(add24 lo)
    {
        return ReadByte(lo) + cregs.S;
    }

    add24 AddressingMode_Direct(add24 ll, bool old = true)
    {
        ll = ReadByte(ll);
        if (flagE && old && cregs.D.L == 0x00)
        {
            return (cregs.D.H << 8) | ll;
        }
        else
        {
            return cregs.D + ll;
        }
    }
    add24 AddressingMode_Absolute(add24 ll, bool jmp = false)
    {
        if (jmp)
        {
            return ReadWord(ll);
        }
        else
        {
            return (cregs.DBR << 16) | ReadWord(ll);
        }
    }

    add24 AddressingMode_Long(add24 address)
    {
        return (add24)ReadByte(address) | ((add24)ReadByte(address + 1) << 8) | ((add24)ReadByte(address + 2) << 16);
    }

    uint16 ReadWord(add24 address)
    {
        uint16 low = ReadByte(address);
        uint16 high = ReadByte(address + 1);
        return (high << 8) | low;
    }

    void WriteWord(add24 address, uint16 data)
    {
        WriteByte(address, data & 0x00FF);
        WriteByte(address + 1, data >> 8);
    }

    void WriteM(add24 abs, uint16 d)
    {
        if (flags.M)
            WriteByte(abs, d);
        else
            WriteWord(abs, d);
    }

    void WriteX(add24 abs, uint16 d)
    {
        if (flags.X)
            WriteByte(abs, d);
        else
            WriteWord(abs, d);
    }

    uint16 ReadM(add24 abs)
    {
        if (flags.M)
            return ReadByte(abs);
        else
            return ReadWord(abs);
    }

    uint16 ReadX(add24 abs)
    {
        if (flags.X)
            return ReadByte(abs);
        else
            return ReadWord(abs);
    }

    void Push(uint8 d)
    {
        WriteByte(cregs.S, d);
        cregs.S -= 1;
    }
    void PushWord(uint16 d)
    {
        cregs.S -= 1;
        WriteWord(cregs.S, d);
        cregs.S -= 1;
        // cin.get();
    }

    void UpdateC(uint16 val)
    {
        if (flags.M)
        {
            cregs.C.L = val;
        }
        else
        {
            cregs.C = val;
        }
    }

    void UpdateX(uint16 val)
    {
        if (flags.X)
        {
            cregs.X.L = val;
        }
        else
        {
            cregs.X = val;
        }
    }

    void UpdateY(uint16 val)
    {
        if (flags.X)
        {
            cregs.Y.L = val;
        }
        else
        {
            cregs.Y = val;
        }
    }

    uint8 Pop()
    {
        cregs.S += 1;
        return ReadByte(cregs.S);
    }
    uint16 PopWord()
    {
        cregs.S += 1;
        uint16 val = ReadWord(cregs.S);
        cregs.S += 1;
        return val;
    }

    CPU(C65ReadByteFunc_t readfunc, C65WriteByteFunc_t writefunc)
    {
        this->ReadByte = readfunc;
        this->WriteByte = writefunc;
        cregs.C = 0;
        cregs.DBR = 0;
        cregs.D = 0;
        cregs.K = 0;

        // Program Should Do this
        cregs.PC = 0; // FFFC
        cregs.S = 0;  // 0x00ff;

        cregs.X = 0;
        cregs.Y = 0;

        flagE = 1;
        flags.X = 1;
        flags.M = 1;
        flags.I = 1;

        pcInstDump = ofstream("dump.csv");
    }

    // Interrupts
    void reset()
    {
        uint16 rstVector = ReadWord(0x00FFFC);
        cregs.K = 0;
        cregs.PC = rstVector;
        cout << "rstVector : " << rstVector << endl;
        stop = false;
    }

    void invokeNMI()
    {
        if (waitForInt)
            waitForInt = false;
        if (!flagE)
            Push(cregs.K);  // 8 Bit
        PushWord(cregs.PC); // 16 Bit
        Push(flags);        // 8 Bit

        cout << "------------NMI------------------" << endl;
        uint16 nmi;
        cregs.K = 0;
        if (flagE)
            nmi = ReadWord(0xFFFA);
        else
            nmi = ReadWord(0xFFEA);
        cregs.PC = nmi;
    }

    void setOverflow()
    {
        flags.V = 1;
    }

    void invokeIRQ()
    {
        if (waitForInt)
            waitForInt = false;

        if (flags.I) // I == 1 => disable intrrupts.
            return;

        if (!flagE)
            Push(cregs.K);  // 8 Bit
        PushWord(cregs.PC); // 16 Bit
        Push(flags);        // 8 Bit

        cout << "------------IRQ------------------" << endl;

        uint16 irq;
        cregs.K = 0;
        if (flagE)
            irq = ReadWord(0xFFFE);
        else
            irq = ReadWord(0xFFEE);
        cregs.PC = irq;
    }

    void doADC(uint16 d)
    {
        unsigned int t;
        if (flags.D)
        {
            // --- BCD (Decimal) Addition ---
            if (flags.M)
            {
                // 8-bit BCD
                int op1 = cregs.C & 0xFF;
                int op2 = d & 0xFF;

                // 1. Calculate pure binary sum just for the V flag
                int binSum = op1 + op2 + flags.C;
                flags.V = (((op1 ^ binSum) & (op2 ^ binSum) & 0x80) != 0);

                // 2. Perform BCD Adjustments
                int al = (op1 & 0x0F) + (op2 & 0x0F) + flags.C;
                if (al > 0x09)
                    al += 0x06;

                int ah = (op1 & 0xF0) + (op2 & 0xF0) + (al > 0x0F ? 0x10 : 0);
                if (ah > 0x90)
                    ah += 0x60;

                t = (ah & 0xF0) | (al & 0x0F);
                flags.C = (ah > 0xFF);
            }
            else
            {
                // 16-bit BCD
                int op1 = cregs.C & 0xFFFF;
                int op2 = d & 0xFFFF;

                int binSum = op1 + op2 + flags.C;
                flags.V = (((op1 ^ binSum) & (op2 ^ binSum) & 0x8000) != 0);

                int al = (op1 & 0x000F) + (op2 & 0x000F) + flags.C;
                if (al > 0x0009)
                    al += 0x0006;

                int ah = (op1 & 0x00F0) + (op2 & 0x00F0) + (al > 0x000F ? 0x0010 : 0);
                if (ah > 0x0090)
                    ah += 0x0060;

                int bl = (op1 & 0x0F00) + (op2 & 0x0F00) + (ah > 0x00FF ? 0x0100 : 0);
                if (bl > 0x0900)
                    bl += 0x0600;

                int bh = (op1 & 0xF000) + (op2 & 0xF000) + (bl > 0x0FFF ? 0x1000 : 0);
                if (bh > 0x9000)
                    bh += 0x6000;

                t = (bh & 0xF000) | (bl & 0x0F00) | (ah & 0x00F0) | (al & 0x000F);
                flags.C = (bh > 0xFFFF);
            }
        }
        else
        {
            // --- Standard Binary Addition ---
            if (flags.M)
            {
                int op1 = cregs.C & 0xFF;
                int op2 = d & 0xFF;
                t = op1 + op2 + flags.C;
                flags.V = (((op1 ^ t) & (op2 ^ t) & 0x80) != 0);
                flags.C = (t > 0xFF);
            }
            else
            {
                int op1 = cregs.C & 0xFFFF;
                int op2 = d & 0xFFFF;
                t = op1 + op2 + flags.C;
                flags.V = (((op1 ^ t) & (op2 ^ t) & 0x8000) != 0);
                flags.C = (t > 0xFFFF);
            }
        }

        UpdateC(t);

        flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
        flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
    }

    void doSBC(uint16 d)
    {
        unsigned int t;

        if (flags.D)
        {
            // --- BCD (Decimal) Subtraction ---
            if (flags.M)
            {
                // 8-bit BCD
                int op1 = cregs.C & 0xFF;
                int op2 = d & 0xFF;
                int carryIn = flags.C ? 0 : 1; // 65816 Carry is inverted borrow

                // 1. Pure binary subtraction for V and C flags
                int binDiff = op1 - op2 - carryIn;
                // V flag: Checks if signs of operands differed, and if result sign differs from accumulator
                flags.V = (((op1 ^ op2) & (op1 ^ binDiff) & 0x80) != 0);
                flags.C = (binDiff >= 0); // Carry is 1 if no borrow occurred

                // 2. Perform BCD Adjustments (nibble by nibble)
                int al = (op1 & 0x0F) - (op2 & 0x0F) - carryIn;
                int ah = ((op1 >> 4) & 0x0F) - ((op2 >> 4) & 0x0F) - (al < 0 ? 1 : 0);

                // If a nibble underflows (borrows), we apply the BCD correction (-6)
                if (al < 0)
                    al -= 6;
                if (ah < 0)
                    ah -= 6;

                t = ((ah << 4) | (al & 0x0F)) & 0xFF;
            }
            else
            {
                // 16-bit BCD
                int op1 = cregs.C & 0xFFFF;
                int op2 = d & 0xFFFF;
                int carryIn = flags.C ? 0 : 1;

                int binDiff = op1 - op2 - carryIn;
                flags.V = (((op1 ^ op2) & (op1 ^ binDiff) & 0x8000) != 0);
                flags.C = (binDiff >= 0);

                // Cascaded nibble subtraction
                int al = (op1 & 0x000F) - (op2 & 0x000F) - carryIn;
                int ah = ((op1 >> 4) & 0x000F) - ((op2 >> 4) & 0x000F) - (al < 0 ? 1 : 0);
                int bl = ((op1 >> 8) & 0x000F) - ((op2 >> 8) & 0x000F) - (ah < 0 ? 1 : 0);
                int bh = ((op1 >> 12) & 0x000F) - ((op2 >> 12) & 0x000F) - (bl < 0 ? 1 : 0);

                // Apply corrections where borrows occurred
                if (al < 0)
                    al -= 6;
                if (ah < 0)
                    ah -= 6;
                if (bl < 0)
                    bl -= 6;
                if (bh < 0)
                    bh -= 6;

                t = ((bh << 12) | ((bl & 0x0F) << 8) | ((ah & 0x0F) << 4) | (al & 0x0F)) & 0xFFFF;
            }
        }
        else
        {
            // --- Standard Binary Subtraction ---
            if (flags.M)
            {
                int op1 = cregs.C & 0xFF;
                int op2 = d & 0xFF;
                int binDiff = op1 - op2 - (flags.C ? 0 : 1);

                t = binDiff & 0xFF;
                flags.V = (((op1 ^ op2) & (op1 ^ t) & 0x80) != 0);
                flags.C = (binDiff >= 0);
            }
            else
            {
                int op1 = cregs.C & 0xFFFF;
                int op2 = d & 0xFFFF;
                int binDiff = op1 - op2 - (flags.C ? 0 : 1);

                t = binDiff & 0xFFFF;
                flags.V = (((op1 ^ op2) & (op1 ^ t) & 0x8000) != 0);
                flags.C = (binDiff >= 0);
            }
        }

        UpdateC(t); // Updates the Accumulator (handles 8-bit B register preservation)

        // The N and Z flags evaluate the resulting accumulator value (Valid for BCD!)
        flags.N = (t & (flags.M ? 0x0080 : 0x8000)) != 0;
        flags.Z = (t & (flags.M ? 0x00FF : 0xFFFF)) == 0;
    }
    void printStatus()
    {
        // nvmxdizc
        cout << "--------------------------NEW INSTRUCTON---------------------------------------" << endl;
        cout << "-----------Flags----------" << endl;
        cout << "N V M X D I Z C    E" << endl;
        cout << flags.N << " " << flags.V << " " << flags.M << " " << flags.X << " " << flags.D << " " << flags.I << " " << flags.Z << " " << flags.C << "    " << flagE << endl;
        cout << "-----------Regs----------" << endl;
        cout << "Stack top : " << hex << (unsigned int)ReadByte(cregs.S + 1) << " " << (unsigned int)ReadByte(cregs.S + 2) << endl;
        cout << "C : " << std::hex << (unsigned int)cregs.C << endl;
        cout << "DBR : " << std::hex << (unsigned int)cregs.DBR << endl;
        cout << "D : " << std::hex << (unsigned int)cregs.D << endl;
        cout << "K : " << std::hex << (unsigned int)cregs.K << endl;
        cout << "S : " << std::hex << (unsigned int)cregs.S << endl;
        cout << "X : " << std::hex << (unsigned int)cregs.X << endl;
        cout << "Y : " << std::hex << (unsigned int)cregs.Y << endl;
        cout << "PC : " << std::hex << (unsigned int)cregs.PC << endl;
        add24 instAddress = ((add24)cregs.K << 16) + (add24)cregs.PC;

        cout << "InstAddress : " << std::hex << instAddress << endl;
        uint8_t inst = ReadByte(instAddress);
        cout << "OpCode : " << std::hex << (int)inst << endl;
        pcInstDump << hex << instAddress << "," << std::hex << (int)inst << endl;
        cout << "3 Bytes Ahead : " << std::hex << (int)ReadByte(instAddress + 1) << " " << std::hex << (int)ReadByte(instAddress + 2) << " " << std::hex << (int)ReadByte(instAddress + 3) << " " << endl;
    }
    string stringStatus()
    {
        stringstream ss;
        // nvmxdizc
        ss << "--------------------------NEW INSTRUCTON---------------------------------------" << endl;
        ss << "-----------Flags----------" << endl;
        ss << "N V M X D I Z C    E" << endl;
        ss << flags.N << " " << flags.V << " " << flags.M << " " << flags.X << " " << flags.D << " " << flags.I << " " << flags.Z << " " << flags.C << "    " << flagE << endl;
        ss << "-----------Regs----------" << endl;
        ss << "Stack top : " << hex << (unsigned int)ReadByte(cregs.S + 1) << " " << (unsigned int)ReadByte(cregs.S + 2) << endl;

        ss << "C : " << std::hex << (unsigned int)cregs.C << endl;
        ss << "DBR : " << std::hex << (unsigned int)cregs.DBR << endl;
        ss << "D : " << std::hex << (unsigned int)cregs.D << endl;
        ss << "K : " << std::hex << (unsigned int)cregs.K << endl;
        ss << "S : " << std::hex << (unsigned int)cregs.S << endl;
        ss << "X : " << std::hex << (unsigned int)cregs.X << endl;
        ss << "Y : " << std::hex << (unsigned int)cregs.Y << endl;
        ss << "PC : " << std::hex << (unsigned int)cregs.PC << endl;
        add24 instAddress = ((add24)cregs.K << 16) + (add24)cregs.PC;

        ss << "InstAddress : " << std::hex << instAddress << endl;
        uint8_t inst = ReadByte(instAddress);
        ss << "OpCode : " << std::hex << (int)inst << endl;
        pcInstDump << hex << instAddress << "," << std::hex << (int)inst << endl;
        ss << "3 Bytes Ahead : " << std::hex << (int)ReadByte(instAddress + 1) << " " << std::hex << (int)ReadByte(instAddress + 2) << " " << std::hex << (int)ReadByte(instAddress + 3) << " " << endl;

        return ss.str();
    }

    void cpuStep()
    {
        if (waitForInt || stop)
        {
            return;
        }
        if (flagE) // Emulation Constrains
        {
            cregs.S.H = 0x01;
            flags.X = 1;
            flags.M = 1;
        }
        if (flags.X)
        {
            cregs.X.H = 0;
            cregs.Y.H = 0;
        }

        bool opcodeNotFound = false;
        instAddress = ((add24)cregs.K << 16) + (add24)cregs.PC;
        uint8_t inst = ReadByte(instAddress);

        switch (inst)
        {

        case 0xea: // NOP
        {
            cregs.PC += 1;
            break;
        }
        case 0x42: // WDM
        {
            cregs.PC += 2;
            break;
        }

        // Needs more reading and changing, not gonna be use anyways
        case 0x00: // BRK
        {

            if (!flagE)
                Push(cregs.K);      // 8 Bit
            PushWord(cregs.PC + 2); // 16 Bit
            Push(flags);            // 8 Bit
            flags.I = 1;
            flags.D = 0;
            uint16 brk;
            cregs.K = 0;
            if (flagE)
                brk = ReadWord(0xFFFE); // IRQ with b flag
            else
                brk = ReadWord(0xFFE6);
            cregs.PC = brk;

            cout << "=-=-=-=-=-=-=-=-=-=-= BREAK POINT =-=-=-=-=-=-=-=-=-=-=" << endl;

            cin.get();
            break;
        }
        // Needs more reading and changing, not gonna be use anyways
        case 0x02: // COP
        {

            if (!flagE)
                Push(cregs.K);      // 8 Bit
            PushWord(cregs.PC + 2); // 16 Bit
            Push(flags);            // 8 Bit
            flags.I = 1;
            flags.D = 0;
            uint16 cop;
            cregs.K = 0;
            if (flagE)
                cop = ReadWord(0xFFF4);
            else
                cop = ReadWord(0xFFE4);
            cregs.PC = cop;
            break;
        }
        case 0xdb: // STP
        {
            stop = true;
            cregs.PC += 1;
            break;
        }
        case 0xcb: // WAI
        {
            waitForInt = true;
            cregs.PC += 1;
            break;
        }
#pragma region Flag_Manipulation
        case 0xfb: // XCE
        {
            bool e = flagE;
            flagE = flags.C;
            flags.C = e;

            cregs.PC += 1;
            break;
        }
        case 0x78: // SEI
        {
            flags.I = 1;
            cregs.PC += 1;
            break;
        }
        case 0x38: // SEC
        {
            flags.C = 1;
            cregs.PC += 1;
            break;
        }
        case 0xF8: // SED
        {
            flags.D = 1;
            cregs.PC += 1;
            break;
        }
        case 0xB8: // CLV
        {
            flags.V = 0;
            cregs.PC += 1;
            break;
        }
        case 0x18: // CLC
        {
            flags.C = 0;
            cregs.PC += 1;
            break;
        }
        case 0xD8: // CLD
        {
            flags.D = 0;
            cregs.PC += 1;
            break;
        }
        case 0x58: // CLI
        {
            flags.I = 0;
            cregs.PC += 1;
            break;
        }

        case 0xc2: // REP imm - Reset selected Flags
        {
            uint8 imm = ReadByte(instAddress + 1);
            flags.ResetMask(imm);
            cregs.PC += 2;
            break;
        }
        case 0xe2: // SEP imm - Set selected Flags
        {
            uint8 imm = ReadByte(instAddress + 1);
            flags.SetMask(imm);
            cregs.PC += 2;
            break;
        }
#pragma endregion

#pragma region Stack_Operations

        case 0x08: // PHP
        {

            Push(flags);
            // No Flags
            // cout << "PHP : " << hex << (uint16)flags << endl;
            // cin.get();
            cregs.PC += 1;

            break;
        }
        case 0x48: // PHA
        {
            if (flags.M)
                Push((uint8)cregs.C);
            else
                PushWord((uint16)cregs.C);
            cregs.PC += 1;
            break;
        }
        case 0xda: // PHX
        {
            if (flags.X)
                Push((uint8)cregs.X);
            else
                PushWord((uint16)cregs.X);
            cregs.PC += 1;
            break;
        }
        case 0x5a: // PHY
        {
            if (flags.X)
                Push((uint8)cregs.Y);
            else
                PushWord((uint16)cregs.Y);
            cregs.PC += 1;
            break;
        }
        case 0x0b: // PHD
        {

            PushWord((uint16)cregs.D);
            cregs.PC += 1;
            break;
        }
        case 0x8b: // PHB
        {
            // TODO : Make sure about 1 byte
            Push((uint16)cregs.DBR);
            cregs.PC += 1;
            break;
        }
        case 0x4b: // PHK
        {
            Push((uint16)cregs.K);
            cregs.PC += 1;
            break;
        }

        case 0x68: // PLA
        {
            uint16 d;
            if (flags.M)
                d = Pop();
            else
                d = PopWord();

            UpdateC(d);

            // cout << "PLA : " << hex << (uint16)cregs.C << endl;
            //  cin.get();
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0x7a: // PLY
        {
            uint16 d;
            if (flags.X)
                d = Pop();
            else
                d = PopWord();

            UpdateY(d);

            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xfa: // PLX
        {
            uint16 d;
            if (flags.X)
                d = Pop();
            else
                d = PopWord();

            UpdateX(d);

            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xab: // PLB
        {
            // DBR is always 8 bit
            cregs.DBR = Pop();
            flags.N = cregs.DBR & 0x0080;
            flags.Z = !(cregs.DBR & 0x00FF);
            cregs.PC += 1;
            break;
        }
        case 0x28: // PLP
        {
            uint8 d = Pop();
            flags = d;

            cregs.PC += 1;
            // cout << "new flags " << (uint16)flags << endl;

            break;
        }
        case 0x2B: // PLD
        {
            uint16 d = PopWord();
            cregs.D = d;
            flags.N = cregs.D & 0x8000;
            flags.Z = !cregs.D; // Since operation is 16 bit
            cregs.PC += 1;
            break;
        }

        case 0xf4: // PEA
        {
            uint16 imm = ReadWord(instAddress + 1);
            PushWord(imm);
            cregs.PC += 3;
            break;
        }
        case 0xd4: // PEI
        {
            add24 add = AddressingMode_Direct(instAddress + 1, false);
            PushWord(ReadWord(add)); // Or ReadM?
            cregs.PC += 2;
            break;
        }
        case 0x62: // PER
        {
            signed short imm = ReadWord(instAddress + 1);
            PushWord(instAddress + 3 + imm);
            cregs.PC += 3;
            break;
        }
#pragma endregion

// Arithmatics
#pragma region Arithmatics

        // TODO : CHECK ALL ARITH FLAGS
        case 0x61: // ADC (dir,x)
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x63: // ADC stk,S
        {
            // TODO : BCD for D flag
            add24 add = cregs.S + ReadByte(instAddress + 1);
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x65: // ADC dir
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x67: // ADC [dir]
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x69: // ADC imm
        {
            // TODO : BCD for D flag
            // cin.get();
            uint16 imm = ReadM(instAddress + 1);
            doADC(imm);

            cregs.PC += 3 - flags.M;

            break;
        }
        case 0x6d: // ADC abs
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 3;
            break;
        }

        case 0x6f: // ADC long
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 4;
            break;
        }
        case 0x71: // ADC (dir),y
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }

        case 0x72: // ADC (dir)
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x73: // ADC (stk,s),y
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_16PTR(cregs.S + ReadByte(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x75: // ADC dir,x
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }
        case 0x77: // ADC [dir],y
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 2;
            break;
        }

        case 0x79: // ADC abs,y
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 3;
            break;
        }

        case 0x7d: // ADC abs,x
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 3;
            break;
        }

        case 0x7f: // ADC long,x
        {
            // TODO : BCD for D flag
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doADC(d);
            cregs.PC += 4;
            break;
        }

        case 0xe1: // SBC (dir,x)
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xe3: // SBC stk,S
        {

            add24 add = cregs.S + ReadByte(instAddress + 1);
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }

        case 0xe5: // SBC dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }

        case 0xe7: // SBC [dir]
        {

            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }

        case 0xe9: // SBC imm
        {

            uint16 imm = ReadM(instAddress + 1);
            doSBC(imm);
            cregs.PC += 3 - flags.M;
            break;
        }

        case 0xed: // SBC abs
        {

            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 3;
            break;
        }
        case 0xef: // SBC long
        {

            add24 add = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 4;
            break;
        }
        case 0xf1: // SBC (dir),y
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xf2: // SBC (dir)
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xf3: // SBC (stk,S),y
        {

            add24 add = AddressingMode_16PTR(cregs.S + ReadByte(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xf5: // SBC dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xf7: // SBC [dir],y
        {

            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 2;
            break;
        }
        case 0xf9: // SBC abs,y
        {

            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 3;
            break;
        }
        case 0xfd: // SBC abs,x
        {

            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 3;
            break;
        }
        case 0xff: // SBC long,x
        {

            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            doSBC(d);
            cregs.PC += 4;
            break;
        }

        case 0x3a: // DEC acc
        {
            cregs.C -= 1;
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xc6: // DEC dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            d -= 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xce: // DEC abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            d -= 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }
        case 0xd6: // DEC dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            d -= 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xde: // DEC abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            d -= 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0xca: // DEX
        {
            cregs.X -= 1;
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0x88: // DEY
        {
            cregs.Y -= 1;
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0x1a: // INC acc
        {
            cregs.C += 1;
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0xe6: // INC dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            d += 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xee: // INC abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            d += 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }
        case 0xf6: // INC dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            d += 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xfe: // INC abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            d += 1;
            WriteM(add, d);
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0xe8: // INX
        {
            cregs.X += 1;
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xc8: // INY
        {
            cregs.Y += 1;

            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

#pragma endregion
// Bitwise arithmatic
#pragma region Bitwise_Arithmatics
        case 0x21: // AND (dir,x)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;

            break;
        }
        case 0x23: // AND stk,S
        {
            add24 add = ReadByte(instAddress + 1) + cregs.S;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x25: // AND dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x27: // AND [dir]
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x29: // AND imm
        {
            uint16 d = ReadM(instAddress + 1);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.M;
            break;
        }
        case 0x2d: // AND abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0x2f: // AND long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        case 0x31: // AND (dir),y
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x32: // AND (dir)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x33: // AND (stks,S),y
        {
            add24 add = AddressingMode_16PTR(ReadByte(instAddress + 1) + cregs.S) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x35: // AND dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x37: // AND [dir],y
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x39: // AND abs,y
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;

            break;
        }
        case 0x3d: // AND abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            // cin.get();
            break;
        }

        case 0x3f: // AND long,x
        {
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C & d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        // EORS
        case 0x41: // EOR (dir,x)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x43: // EOR stk,S
        {
            add24 add = ReadByte(instAddress + 1) + cregs.S;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x45: // EOR dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x47: // EOR [dir]
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x49: // EOR imm
        {
            uint16 d = ReadM(instAddress + 1);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.M;
            break;
        }
        case 0x4d: // EOR abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0x4f: // EOR long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        case 0x51: // EOR (dir),y
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x52: // EOR (dir)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x53: // EOR (stks,S),y
        {
            add24 add = AddressingMode_16PTR(ReadByte(instAddress + 1) + cregs.S) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x55: // EOR dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x57: // EOR [dir],y
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x59: // EOR abs,y
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0x5d: // EOR abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }

        case 0x5f: // EOR long,x
        {
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C ^ d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        // ORAs
        case 0x01: // ORA (dir,x)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x03: // ORA stk,S
        {
            add24 add = ReadByte(instAddress + 1) + cregs.S;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x05: // ORA dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x07: // ORA [dir]
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x09: // ORA imm
        {
            uint16 d = ReadM(instAddress + 1);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.M;
            break;
        }
        case 0x0d: // ORA abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0x0f: // ORA long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        case 0x11: // ORA (dir),y
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x12: // ORA (dir)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x13: // ORA (stks,S),y
        {
            add24 add = AddressingMode_16PTR(ReadByte(instAddress + 1) + cregs.S) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x15: // ORA dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0x17: // ORA [dir],y
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0x19: // ORA abs,y
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0x1d: // ORA abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }

        case 0x1f: // ORA long,x
        {
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            UpdateC(cregs.C | d);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }

        case 0x06: // ASL dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 t = ReadM(add);
            if (flags.M)
            {

                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = t << 1;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0x0a: // ASL acc
        {
            if (flags.M)
            {

                flags.C = cregs.C & 0x0080;
            }
            else
            {
                flags.C = cregs.C & 0x8000;
            }
            UpdateC(cregs.C << 1);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }
        case 0x0e: // ASL abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 t = ReadM(add);
            if (flags.M)
            {

                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = t << 1;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0x16: // ASL dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            if (flags.M)
            {

                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = t << 1;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }

        case 0x1e: // ASL abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            if (flags.M)
            {

                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = t << 1;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0x46: // LSR dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 t = ReadM(add);
            flags.C = t & 0x0001;
            t = t >> 1;
            WriteM(add, t);
            flags.N = 0;
            flags.Z = !t;
            cregs.PC += 2;
            break;
        }

        case 0x4a: // LSR acc
        {
            flags.C = cregs.C[0];
            UpdateC(cregs.C >> 1);
            flags.N = 0;
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0x4e: // LSR abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 t = ReadM(add);
            flags.C = t & 0x0001;
            t = t >> 1;
            WriteM(add, t);
            flags.N = 0;
            flags.Z = !t;
            cregs.PC += 3;
            break;
        }

        case 0x56: // LSR dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            flags.C = t & 0x0001;
            t = t >> 1;
            WriteM(add, t);
            flags.N = 0;
            flags.Z = !t;
            cregs.PC += 2;
            break;
        }

        case 0x5e: // LSR abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            flags.C = t & 0x0001;
            t = t >> 1;
            WriteM(add, t);
            flags.N = 0;
            flags.Z = !t;
            cregs.PC += 3;
            break;
        }

        case 0x26: // ROL dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 t = ReadM(add);
            bool c = flags.C;
            if (flags.M)
            {
                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = (t << 1) | c;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }

        // TODO : update this later to uese UpdateC
        case 0x2a: // ROL acc
        {

            if (flags.M)
            {
                bool c = flags.C;
                flags.C = cregs.C[7];
                UpdateC((cregs.C << 1) + c);
            }
            else
            {
                bool c = flags.C;
                flags.C = cregs.C[15];
                UpdateC((cregs.C << 1) + c);
            }
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }

        case 0x2e: // ROL abs
        {

            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 t = ReadM(add);
            bool c = flags.C;
            if (flags.M)
            {
                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = (t << 1) | c;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0x36: // ROL dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            bool c = flags.C;
            if (flags.M)
            {
                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = (t << 1) | c;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }

        case 0x3e: // ROL abs,x
        {

            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            bool c = flags.C;
            if (flags.M)
            {
                flags.C = t & 0x0080;
            }
            else
            {
                flags.C = t & 0x8000;
            }
            t = (t << 1) | c;
            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0x66: // ROR dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 t = ReadM(add);
            bool c = flags.C;
            flags.C = t & 0x0001;
            t = (t >> 1) | (flags.M ? c << 7 : c << 15);

            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }

        case 0x6a: // ROR acc
        {

            bool c = flags.C;
            flags.C = cregs.C & 0x0001;
            UpdateC((cregs.C >> 1) | (flags.M ? c << 7 : c << 15));

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }

        case 0x6e: // ROR abs
        {

            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 t = ReadM(add);
            bool c = flags.C;
            flags.C = t & 0x0001;
            t = (t >> 1) | (flags.M ? c << 7 : c << 15);

            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0x76: // ROR dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            bool c = flags.C;
            flags.C = t & 0x0001;
            t = (t >> 1) | (flags.M ? c << 7 : c << 15);

            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }

        case 0x7e: // ROR abs,x
        {

            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 t = ReadM(add);
            bool c = flags.C;
            flags.C = t & 0x0001;
            t = (t >> 1) | (flags.M ? c << 7 : c << 15);

            WriteM(add, t);
            flags.N = t & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }
#pragma endregion

        // Transfer
#pragma region Reg_Transfer

        case 0xeb: // XBA
        {
            swap(cregs.C.H, cregs.C.L);
            // Flags are always set based on the lower 8 bits (in this case)
            flags.N = cregs.C & 0x0080;
            flags.Z = !cregs.C.L;
            cregs.PC += 1;
            break;
        }

        case 0x5b: // TCD - D <- C
        {
            cregs.D = cregs.C;

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }
        case 0x1b: // TCS - S <- C
        {
            cregs.S = cregs.C;

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }

        case 0x7b: // TDC - C <- D
        {
            UpdateC(cregs.D);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }

        case 0x3b: // TSC - C <- S
        {
            UpdateC(cregs.S);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));

            cregs.PC += 1;
            break;
        }

        case 0xaa: // TAX - X <- C
        {
            // TODO : in 8 bit mode, how should X.H be affected?
            // Replaced? keep what ever there was?
            // Should we instead write : cregs.X.L = cregs.C.L?
            UpdateX(cregs.C);
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xa8: // TAY - Y <- C
        {
            UpdateY(cregs.C);
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0xba: // TSX - X <- S
        {

            UpdateX(cregs.S);
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0x8a: // TXA - C <- X
        {

            UpdateC(cregs.X);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0x9a: // TXS - S <- X
        {

            cregs.S = cregs.X;

            cregs.PC += 1;
            break;
        }
        case 0x9b: // TXY - Y <- X
        {

            UpdateY(cregs.X);
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }

        case 0x98: // TYA - C <- Y
        {

            UpdateC(cregs.Y);
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
        case 0xbb: // TYX - X <- Y
        {

            UpdateX(cregs.Y);
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 1;
            break;
        }
#pragma endregion

        // Load and Store
#pragma region Load_and_Store

        case 0xa1: // LDA (dir,x)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xa3: // LDA stk,S
        {
            add24 add = AddressingMode_STK(instAddress + 1);
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xa5: // LDA dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xa7: // LDA  [dir]
        {
            uint16 d = ReadM(AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)));
            UpdateC(d);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xa9: // LDA imm
        {
            uint16 imm = ReadM(instAddress + 1);
            UpdateC(imm);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 3 - flags.M;
            break;
        }
        case 0xad: // LDA abs
        {
            uint16 d = ReadM(AddressingMode_Absolute(instAddress + 1));
            UpdateC(d);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 3;
            break;
        }
        case 0xaf: // LDA long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 4;
            break;
        }

        case 0xb1: // LDA (dir),y
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }

        case 0xb2: // LDA (dir)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }

        case 0xb3: // LDA (stk,S),y
        {
            add24 add = AddressingMode_16PTR(AddressingMode_STK(instAddress + 1)) + cregs.Y;
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xb5: // LDA dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }
        case 0xb7: // LDA [dir],y
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 2;
            break;
        }

        case 0xb9: // LDA abs,y
        {
            uint16 d = ReadM(AddressingMode_Absolute(instAddress + 1) + cregs.Y);
            UpdateC(d);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 3;
            break;
        }
        case 0xbd: // LDA abs,x
        {
            uint16 d = ReadM(AddressingMode_Absolute(instAddress + 1) + cregs.X);
            UpdateC(d);

            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 3;
            break;
        }
        case 0xbf: // LDA long,x
        {
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            UpdateC(ReadM(add));
            flags.N = cregs.C & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(cregs.C & (flags.M ? 0x00FF : 0xFFFF)); // If M is one, the upper bits of C is already 0, so theres no need for checking JUST the 8 lower bits.
            cregs.PC += 4;
            break;
        }
        case 0xa2: // LDX imm
        {
            uint16 imm = ReadX(instAddress + 1);

            UpdateX(imm);

            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.X;
            break;
        }
        case 0xa6: // LDX dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            UpdateX(ReadX(add));
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xae: // LDX abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            UpdateX(ReadX(abs));
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }

        case 0xb6: // LDX dir,y
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.Y;
            UpdateX(ReadX(add));
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0xbe: // LDX abs,y
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            UpdateX(ReadX(abs));
            flags.N = cregs.X & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.X & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }

        case 0xa0: // LDY imm
        {
            uint16 imm = ReadX(instAddress + 1);

            UpdateY(imm);

            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.X;
            break;
        }

        case 0xa4: // LDY dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            UpdateY(ReadX(add));
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }

        case 0xac: // LDY abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            UpdateY(ReadX(abs));
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0xb4: // LDY dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            UpdateY(ReadX(add));
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xbc: // LDY abs,x
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            UpdateY(ReadX(abs));
            flags.N = cregs.Y & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(cregs.Y & (flags.X ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }

        case 0x81: // STA (dir,x)
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }

        case 0x83: // STA stk,s
        {

            add24 add = AddressingMode_STK(instAddress + 1);
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x85: // STA dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x87: // STA [dir]
        {

            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x8d: // STA abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteM(abs, cregs.C);
            cregs.PC += 3;
            break;
        }
        case 0x8f: // STA long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            WriteM(add, cregs.C);
            cregs.PC += 4;
            break;
        }
        case 0x91: // STA (dir),y
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x92: // STA (dir)
        {

            add24 add = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x93: // STA (stk,s),y
        {

            add24 add = AddressingMode_16PTR(AddressingMode_STK(instAddress + 1)) + cregs.Y;
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x95: // STA dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x97: // STA [dir],y
        {

            add24 add = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            WriteM(add, cregs.C);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x99: // STA abs,y
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteM(abs + cregs.Y, cregs.C);
            cregs.PC += 3;
            break;
        }

        case 0x9d: // STA abs,x
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteM(abs + cregs.X, cregs.C);
            cregs.PC += 3;
            break;
        }

        case 0x9f: // STA long,x
        {
            add24 add = AddressingMode_Long(instAddress + 1) + cregs.X;
            WriteM(add, cregs.C);
            cregs.PC += 4;
            break;
        }

        case 0x86: // STX dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            WriteX(add, cregs.X);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x8e: // STX abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteX(abs, cregs.X);
            cregs.PC += 3;
            break;
        }
        case 0x96: // STX dir,y
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.Y;
            WriteX(add, cregs.X);
            // No Flags
            cregs.PC += 2;
            break;
        }

        case 0x84: // STY dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            WriteX(add, cregs.Y);
            // No Flags
            cregs.PC += 2;
            break;
        }

        case 0x8c: // STY abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteX(abs, cregs.Y);
            cregs.PC += 3;
            break;
        }

        case 0x94: // STY dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            WriteX(add, cregs.Y);
            // No Flags
            cregs.PC += 2;
            break;
        }

        case 0x64: // STZ dir
        {

            add24 add = AddressingMode_Direct(instAddress + 1);
            WriteM(add, 0);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x74: // STZ dir,x
        {

            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            WriteM(add, 0);
            // No Flags
            cregs.PC += 2;
            break;
        }
        case 0x9c: // STZ abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            WriteM(abs, 0);
            cregs.PC += 3;
            break;
        }
        case 0x9e: // STZ abs,x
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            WriteM(abs, 0);
            cregs.PC += 3;
            break;
        }
#pragma endregion
// Checks
#pragma region Compare_and_Test
        case 0xc1: // CMP (dir,x)
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1) + cregs.X);
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xc3: // CMP stk,s
        {
            // TODO : Check 8 bit operation
            add24 abs = ReadByte(instAddress + 1) + cregs.S;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xc5: // CMP dir
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xc7: // CMP [dir]
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xc9: // CMP imm
        {
            uint16 imm = ReadM(instAddress + 1);

            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= imm;
            imm = cregs.C - imm;

            flags.N = imm & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(imm & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.M;

            break;
        }
        case 0xcd: // CMP abs
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0xcf: // CMP long
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Long(instAddress + 1);
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }
        case 0xd1: // CMP (dir),y
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xd2: // CMP (dir)
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_16PTR(AddressingMode_Direct(instAddress + 1));
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xd3: // CMP (stk,s),y
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_16PTR(ReadByte(instAddress + 1) + cregs.S) + cregs.Y;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xd5: // CMP dir,x
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xd7: // CMP [dir],y
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_24PTR(AddressingMode_Direct(instAddress + 1)) + cregs.Y;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 2;
            break;
        }
        case 0xd9: // CMP abs,y
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Absolute(instAddress + 1) + cregs.Y;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0xdd: // CMP abs,x
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3;
            break;
        }
        case 0xdf: // CMP long,x
        {
            // TODO : Check 8 bit operation
            add24 abs = AddressingMode_Long(instAddress + 1) + cregs.X;
            uint16 d = ReadM(abs);
            // cout << "CMP : " << std::hex << d << " " << cregs.C << endl;
            flags.C = (cregs.C & (flags.M ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.C - d;
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 4;
            break;
        }
        case 0xe0: // CPX imm
        {
            uint16 imm = ReadX(instAddress + 1);
            flags.C = (cregs.X & (flags.X ? 0x00FF : 0xFFFF)) >= imm;

            imm = cregs.X - imm;
            flags.N = imm & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(imm & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 3 - flags.X;
            break;
        }
        case 0xe4: // CPX dir
        {
            add24 abs = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadX(abs);
            flags.C = (cregs.X & (flags.X ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.X - d;
            flags.N = d & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xec: // CPX abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadX(abs);
            // cout << "CPX " << hex << cregs.X << " , " << d << endl;
            flags.C = (cregs.X & (flags.X ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.X - d;

            flags.N = d & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }

        case 0xc0: // CPY imm
        {
            uint16 imm = ReadX(instAddress + 1);
            flags.C = (cregs.Y & (flags.X ? 0x00FF : 0xFFFF)) >= imm;
            imm = cregs.Y - imm;
            flags.N = imm & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(imm & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 3 - flags.X;
            break;
        }
        case 0xc4: // CPY dir
        {
            add24 abs = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadX(abs);
            flags.C = (cregs.Y & (flags.X ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.Y - d;
            flags.N = d & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 2;
            break;
        }
        case 0xcc: // CPY abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadX(abs);
            flags.C = (cregs.Y & (flags.X ? 0x00FF : 0xFFFF)) >= d;
            d = cregs.Y - d;
            flags.N = d & (flags.X ? 0x0080 : 0x8000);
            flags.Z = !(d & (flags.X ? 0x00FF : 0xFFFF));

            cregs.PC += 3;
            break;
        }
        case 0x24: // BIT dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            // cout << "BIT : " << hex << cregs.C << " , " << d << endl;

            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.V = d & (flags.M ? 0x0040 : 0x4000);
            cregs.PC += 2;
            break;
        }
        case 0x2c: // BIT abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            // cout << "BIT : " << hex << cregs.C << " , " << d << endl;
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.V = d & (flags.M ? 0x0040 : 0x4000);
            cregs.PC += 3;
            break;
        }
        case 0x34: // BIT dir,x
        {
            add24 add = AddressingMode_Direct(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.V = d & (flags.M ? 0x0040 : 0x4000);
            cregs.PC += 2;
            break;
        }
        case 0x3c: // BIT abs,x
        {
            add24 add = AddressingMode_Absolute(instAddress + 1) + cregs.X;
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF)); // THE AND NOT THE ARGUMENT!!
            flags.N = d & (flags.M ? 0x0080 : 0x8000);
            flags.V = d & (flags.M ? 0x0040 : 0x4000);
            cregs.PC += 3;
            break;
        }
        case 0x89: // BIT imm
        {
            uint16 imm = ReadM(instAddress + 1);
            uint16 t = cregs.C & imm;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            cregs.PC += 3 - flags.M;
            break;
        }

        case 0x14: // TRB dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            d = d & (~cregs.C);
            WriteM(add, d);

            cregs.PC += 2;
            break;
        }
        case 0x1c: // TRB abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            d = d & (~cregs.C);
            WriteM(add, d);

            cregs.PC += 3;
            break;
        }
        case 0x04: // TSB dir
        {
            add24 add = AddressingMode_Direct(instAddress + 1);
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            d = d | cregs.C;
            WriteM(add, d);

            cregs.PC += 2;
            break;
        }
        case 0x0c: // TSB abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            uint16 d = ReadM(add);
            uint16 t = cregs.C & d;
            flags.Z = !(t & (flags.M ? 0x00FF : 0xFFFF));
            d = d | cregs.C;
            WriteM(add, d);

            cregs.PC += 3;
            break;
        }

#pragma endregion
        // Flow Control
#pragma region Flow_Control
        case 0x90: // BCC
        {
            signed char ll = ReadByte(instAddress + 1);
            if (!flags.C)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0xB0: // BCS
        {
            signed char ll = ReadByte(instAddress + 1);
            if (flags.C)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0xd0: // BNE rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            if (!flags.Z)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0x10: // BPL rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            if (!flags.N)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0x80: // BRA rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            cregs.PC += 2 + ll;
            break;
        }
        case 0xf0: // BEQ rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            if (flags.Z)
                cregs.PC += ll;
            cregs.PC += 2;

            break;
        }
        case 0x30: // BMI
        {
            signed char ll = ReadByte(instAddress + 1);
            if (flags.N)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0x70: // BVS rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            if (flags.V)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0x50: // BVC rel8
        {
            signed char ll = ReadByte(instAddress + 1);
            if (!flags.V)
                cregs.PC += ll;
            cregs.PC += 2;
            break;
        }
        case 0x82: // BRL rel16
        {
            signed short ll = ReadWord(instAddress + 1);

            cregs.PC += ll;
            cregs.PC += 3;
            break;
        }

        case 0x4c: // JMP abs
        {
            add24 add = AddressingMode_Absolute(instAddress + 1);
            cregs.PC = add & 0x00FFFF;
            break;
        }

        case 0x5c: // JMP long
        {
            add24 add = AddressingMode_Long(instAddress + 1);
            cregs.PC = add & 0x00FFFF;
            cregs.K = (add & 0xFF0000) >> 16;
            break;
        }

        case 0x6c: // JMP (abs)
        {
            add24 add = AddressingMode_16PTR(AddressingMode_Absolute(instAddress + 1, true));
            cregs.PC = add & 0x00FFFF;
            break;
        }

        case 0x20: // JSR abs
        {
            add24 abs = AddressingMode_Absolute(instAddress + 1, true);
            // push PC+2 to stack
            PushWord(cregs.PC + 2);
            // No Flags
            cregs.PC = abs;
            break;
        }
        case 0xfc: // JSR (abs,x)
        {
            // This case also needs K in addressing the address
            add24 abs = AddressingMode_16PTR((cregs.K << 16) | (AddressingMode_Absolute(instAddress + 1, true) + cregs.X));
            // push PC+2 to stack
            PushWord(cregs.PC + 2);
            // cout << "wow" << endl;
            // cin.get();
            //  No Flags
            cregs.PC = abs;
            break;
        }
        case 0x22: // JSL long
        {
            Push(cregs.K);
            PushWord(cregs.PC + 3);
            cregs.PC = ReadWord(instAddress + 1);
            cregs.K = ReadByte(instAddress + 3);
            break;
        }
        case 0x7c: // JMP (abs,x)
        {
            // This case also needs K in addressing the address
            add24 add = AddressingMode_16PTR((cregs.K << 16) | (AddressingMode_Absolute(instAddress + 1, true) + cregs.X));
            cregs.PC = add;
            // No Flags
            break;
        }
        case 0xdc: // JMP [abs]
        {
            add24 add = AddressingMode_24PTR(AddressingMode_Absolute(instAddress + 1, true));
            cregs.PC = add & 0x00FFFF;
            cregs.K = (add & 0xFF0000) >> 16;
            break;
        }
        case 0x60: // RTS
        {
            cregs.PC = PopWord();
            cregs.PC += 1; // Intentional
            break;
        }
        case 0x6B: // RTL
        {
            cregs.PC = PopWord();
            cregs.PC += 1; // Intentional
            cregs.K = Pop();
            break;
        }
        case 0x40: // RTI
        {
            flags = Pop();
            cregs.PC = PopWord();
            if (!flagE)
                cregs.K = Pop();
            break;
        }

        case 0x44: // MVP
        {
            add24 srcB = ReadByte(instAddress + 2) << 16;
            add24 dstB = ReadByte(instAddress + 1) << 16;
            add24 src, dest;
            if (cregs.C != 0xFFFF)
            {
                src = srcB | cregs.X;
                dest = dstB | cregs.Y;
                WriteByte(dest, ReadByte(src));
                cregs.X -= 1;
                cregs.Y -= 1;
                cregs.C -= 1;
            }
            cregs.DBR = ReadByte(instAddress + 1); // DBR = dest bank

            if (cregs.C == 0xFFFF)
                cregs.PC += 3;
            break;
        }
        case 0x54: // MVN
        {
            add24 srcB = ReadByte(instAddress + 2) << 16;
            add24 dstB = ReadByte(instAddress + 1) << 16;
            add24 src, dest;
            if (cregs.C != 0xFFFF)
            {
                src = srcB | cregs.X;
                dest = dstB | cregs.Y;
                //cout << "MVN : " << hex << src << " --> " << dest << " : " << hex << (uint16)ReadByte(src) << endl;
                WriteByte(dest, ReadByte(src));
                cregs.X += 1;
                cregs.Y += 1;
                cregs.C -= 1;
            }
            cregs.DBR = ReadByte(instAddress + 1); // DBR = dest bank

            if (cregs.C == 0xFFFF)
                cregs.PC += 3;
            break;
        }
#pragma endregion

        default:
            opcodeNotFound = true;
            break;
        }

        if (opcodeNotFound)
        {
            cout << "OP CODE NOT FOUND!" << endl;
            cin.get();
        }
    }
};