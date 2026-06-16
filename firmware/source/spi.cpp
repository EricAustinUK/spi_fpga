#include "nrf.h"

void cnf_spi_pins(){
    static bool cfgd = false;
    if(cfgd) return;
    cfgd = true;
    
    

    NRF_SPIM3->PSEL.SCK  = 3; // send clock signal over external pin 1
    NRF_SPIM3->PSEL.MISO = 0xFFFFFFFF;  // blanking MISO for now
    NRF_SPIM3->PSEL.MOSI = 2; // send bits over external pin 0
    NRF_SPIM3->PSEL.CSN = 4; // send CS over external pin 2
    NRF_SPIM3->FREQUENCY = 0x80000000; // frequency to 8MHz
    NRF_SPIM3->CONFIG = 0b000;
    NRF_SPIM3->RXD.MAXCNT = 0;

    NRF_P0->PIN_CNF[3] = 1;
    NRF_P0->PIN_CNF[2] = 1;
    NRF_P0->PIN_CNF[4] = 1;
    NRF_P0->OUTSET = (1 << 4);
}

void spi_send_arr(volatile uint32_t *arr, uint32_t size, bool big_endian){
    cnf_spi_pins();
    NRF_SPIM3->EVENTS_END = 0; // clear event register
    if(big_endian) *arr = __REV(*arr);
    // configure TX buffer
    NRF_SPIM3->TXD.PTR = (uint32_t)arr; 
    NRF_SPIM3->TXD.MAXCNT = (size) * sizeof(uint32_t); 
    
    NRF_SPIM3->ENABLE = 0b111; // enable SPI peripheral

    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_END==0); // busy wait (for now)

    NRF_SPIM3->TASKS_STOP = 1; // stop sending when buffer is sent
    
    NRF_SPIM3->ENABLE = 0; // disable peripheral
}
