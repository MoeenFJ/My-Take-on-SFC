#include <bits/stdc++.h>
#include "SPC700.cpp"
using namespace std;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int add24;

namespace APU
{
    uint8 aram[64 * 1024] = {0};
    uint8 arom[] = {
        0xCD, 0xEF,
        0xBD,
        0xE8, 0x00,
        0xC6,
        0x1D,
        0xD0, 0xFC,
        0x8F, 0xAA, 0xF4,
        0x8F, 0xBB, 0xF5,
        0x78, 0xCC, 0xF4,
        0xD0, 0xFB,
        0x2F, 0x19,
        0xEB, 0xF4,
        0xD0, 0xFC,
        0x7E, 0xF4,
        0xD0, 0x0B,
        0xE4, 0xF5,
        0xCB, 0xF4,
        0xD7, 0x00,
        0xFC,
        0xD0, 0xF3,
        0xAB, 0x01,
        0x10, 0xEF,
        0x7E, 0xF4,
        0x10, 0xEB,
        0xBA, 0xF6,
        0xDA, 0x00,
        0xBA, 0xF4,
        0xC4, 0XF4,
        0xDD,
        0x5D,
        0xD0, 0xDB,
        0x1F, 0x00, 0x00,
        0xC0, 0xFF
    };

    SPC700 *spc;

    uint8 APUIO0;
    uint8 APUIO1;
    uint8 APUIO2;
    uint8 APUIO3;

    bool ramRomSel = 1;

    uint8 SPCRead(uint16 address)
    {
        if (address <= 0x00EF) // RAM 0000 to 00EF
            return aram[address];
        else if (address <= 0x00FF) // IO 00F0 to 00FF
        {
            switch (address & 0x000F)
            {
            case 0x0: // Test
                break;
            case 0x1: // CONTROL
                break;
            case 0x2: // DSPADDR
                break;
            case 0x3: // DSPDATA
                break;
            case 0x4: // CPUIO0
                return APUIO0;
                break;
            case 0x5: // CPUIO1
                return APUIO1;
                break;
            case 0x6: // CPUIO2
                return APUIO2;
                break;
            case 0x7: // CPUIO3
                return APUIO3;
                break;
            case 0x8: // AUXIO4 (unused)
                break;
            case 0x9: // AUXIO5 (unused)
                break;
            case 0xa: // T0DIV
                break;
            case 0xb: // T1DIV
                break;
            case 0xc: // T2DIV
                break;
            case 0xd: // T0OUT
                break;
            case 0xe: // T1OUT
                break;
            case 0xf: // T2OUT
                break;

            default:
                break;
            }
        }
        else if (address <= 0x01FF) // RAM 0100 to 01FF
            return aram[address];
        else if (address <= 0xFFBF) // RAM 0200 to 0FFBF
            return aram[address];
        else // RAM or ROM FFC0 to FFFF based on reg 0x00F1
        {
            if(ramRomSel)
                return arom[address-0xFFC0];
            else
                return aram[address];
        }
    }
    void SPCWrite(uint16 address, uint8 data)
    {
         if (address <= 0x00EF) // RAM 0000 to 00EF
            aram[address] = data;
        else if (address <= 0x00FF) // IO 00F0 to 00FF
        {
            switch (address & 0x000F)
            {
            case 0x0: // Test
                break;
            case 0x1: // CONTROL
                ramRomSel = data & 0b10000000;
                break;
            case 0x2: // DSPADDR
                break;
            case 0x3: // DSPDATA
                break;
            case 0x4: // CPUIO0
                APUIO0= data;
                break;
            case 0x5: // CPUIO1
                APUIO1 = data;
                break;
            case 0x6: // CPUIO2
                APUIO2 = data;
                break;
            case 0x7: // CPUIO3
                APUIO3 = data;
                break;
            case 0x8: // AUXIO4 (unused)
                break;
            case 0x9: // AUXIO5 (unused)
                break;
            case 0xa: // T0DIV
                break;
            case 0xb: // T1DIV
                break;
            case 0xc: // T2DIV
                break;
            case 0xd: // T0OUT
                break;
            case 0xe: // T1OUT
                break;
            case 0xf: // T2OUT
                break;

            default:
                break;
            }
            aram[address] = data;
        }
        else if (address <= 0x01FF) // RAM 0100 to 01FF
            aram[address] = data;
        else if (address <= 0xFFBF) // RAM 0200 to 0FFBF
            aram[address] = data;
        else // Write always goes to ram
            aram[address] = data;
    }

    void Init()
    {
        ramRomSel = 1;
        spc = new SPC700(SPCRead, SPCWrite);
        spc->Reset();
    }

    void IOWrite(uint16 port, uint8 data)
    {
        switch (port)
        {
        case 0x2140:
            APUIO0 = data;
            break;

        case 0x2141:
            APUIO1 = data;
            break;

        case 0x2142:
            APUIO2 = data;
            break;

        case 0x2143:
            APUIO3 = data;
            break;

        default:
            break;
        }
    }

    uint8 IORead(uint16 port)
    {
        switch (port)
        {
        case 0x2140:
            return APUIO0;
            break;

        case 0x2141:
            return APUIO1;
            break;

        case 0x2142:
            return APUIO2;
            break;

        case 0x2143:
            return APUIO3;
            break;

        default:
            break;
        }
    }



    // Timing:
    // APU clock LCM = 24*128 = 3072
    // So the APU step counter must reset at a multiple of 3072
    // Biggest Multiple of 3072 that fits in 32 bits, is 1398101 * 3072
    uint16 cpuClkCnt = 0;
    uint32_t stepCnt= 0;
    void Step()
    {
        //1 SPC clock per 24 APU clock.
        if(stepCnt%24==0)
            cpuClkCnt++;


        if(spc->cycles < cpuClkCnt)
            spc->Step();
        if(spc->cycles > 60000 && cpuClkCnt > 60000)
        {
            cpuClkCnt -= 60000;
            spc->cycles -= 60000;
        }

        //Update step counter
        stepCnt++;
        if(stepCnt == 3072 * 1398101)
        {
            stepCnt = 0;
        }
    }
};
