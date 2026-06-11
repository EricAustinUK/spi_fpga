#include "MicroBit.h"

int main()
{
    NRF_P0->PIN_CNF[MICROBIT_PIN_COL1] = 1;
    NRF_P0->PIN_CNF[MICROBIT_PIN_ROW1] = 1;
    
    NRF_P0->OUTCLR =  (1 << MICROBIT_PIN_COL1);
    NRF_P0->OUTSET =  (1 << MICROBIT_PIN_ROW1);
}

