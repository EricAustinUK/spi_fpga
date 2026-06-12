#include "nrf.h"

void cnf_spi_pins(){
    static bool cfgd = false;
    if(cfgd) return;
    cfgd = true;
    
    NRF_SPIM3->PSEL.SCK  = 17; // telling SPI peripheral to send clock to 13?
    NRF_SPIM3->PSEL.MISO = 0xFFFFFFFF;  // blanking MISO for now
    NRF_SPIM3->PSEL.MOSI = 13; // telling SPI peripheral to send over pin 15
    NRF_SPIM3->PSEL.CSN = 34; // lets send CS over 16 as well so its easier to implement the slave
    NRF_SPIM3->FREQUENCY = 0x80000000; // frequency to 8MHz
    NRF_SPIM3->CONFIG = 0b000;
    NRF_SPIM3->RXD.MAXCNT = 0;

    NRF_P0->PIN_CNF[17] = 1;
    NRF_P0->PIN_CNF[13] = 1;
    NRF_P1->PIN_CNF[2] = 1;
    NRF_P1->OUTSET = (1 << 2);
}

void spi_send_arr(volatile uint8_t *arr, uint32_t size){
    cnf_spi_pins();
    NRF_SPIM3->EVENTS_END = 0; // clear event register

    // configure TX buffer
    NRF_SPIM3->TXD.PTR = (uint32_t)arr; 
    NRF_SPIM3->TXD.MAXCNT = (size); 
    
    NRF_SPIM3->ENABLE = 0b111; // enable SPI peripheral

    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_END==0); // busy wait (for now)

    NRF_SPIM3->TASKS_STOP = 1; // stop sending when buffer is sent
    
    NRF_SPIM3->ENABLE = 0; // disable peripheral
}
