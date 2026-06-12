#include "MicroBit.h"

extern void spi_send_arr(volatile uint8_t * arr, uint32_t size);

void busy_sleep(uint32_t delay_us){
    NRF_TIMER1->TASKS_STOP = 1; // stop other timers
    NRF_TIMER1->BITMODE = 3; // 32 bit
    NRF_TIMER1->CC[0] = delay_us; // set cmp to time
    NRF_TIMER1->PRESCALER = 4;

    // clear flags
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    NRF_TIMER1->TASKS_CLEAR = 1;

    // wait for delay to be met
    NRF_TIMER1->TASKS_START = 1;
    while(NRF_TIMER1->EVENTS_COMPARE[0] == 0);
    
    // clear flags
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
}

int main()
{
    NRF_P0->PIN_CNF[MICROBIT_PIN_COL1] = 1;
    NRF_P0->PIN_CNF[MICROBIT_PIN_ROW1] = 1;
    
    NRF_P0->PIN_CNF[P0_02] = 1;
    NRF_P0->PIN_CNF[P0_03] = 1;
    NRF_P0->PIN_CNF[P0_04] = 1;

    NRF_P0->OUTCLR =  (1 << MICROBIT_PIN_COL1);
    NRF_P0->OUTSET =  (1 << MICROBIT_PIN_ROW1);

    volatile uint8_t send_arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    volatile uint8_t drive_bits = 0b111;

    while(true){
        spi_send_arr(send_arr, 10);

        // aletrnates adder inputs between 000 and 111
        if (drive_bits & 0b001) NRF_P0->OUTSET = (1 << P0_02);  
        else                    NRF_P0->OUTCLR = (1 << P0_02);  
        if (drive_bits & 0b010) NRF_P0->OUTSET = (1 << P0_03); 
        else                    NRF_P0->OUTCLR = (1 << P0_03); 
        if (drive_bits & 0b100) NRF_P0->OUTSET = (1 << P0_04); 
        else                    NRF_P0->OUTCLR = (1 << P0_04);

        drive_bits +=  1; // because who cares about overflow

        busy_sleep(3000000);
    }
}

