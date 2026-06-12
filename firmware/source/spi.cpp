#include "nrf.h"

void cnf_spi_pins(){
    static bool cfgd = false;
    if(cfgd) return;
    cfgd = true;
    
    NRF_SPIM0->PSEL.SCK  = 13; // telling SPI peripheral to send clock to 13?
    NRF_SPIM0->PSEL.MISO = 0xFFFFFFFF;  // blanking MISO for now
    NRF_SPIM0->PSEL.MOSI = 15; // telling SPI peripheral to send over pin 15
    NRF_SPIM0->PSEL.CSN = 16; // lets send CS over 16 as well so its easier to implement the slave
    NRF_SPIM0->FREQUENCY = 0x80000000; // frequency to 8MHz
    NRF_SPIM0 -> CONFIG = 0b000;
}

void spi_send_arr(uint8_t *arr, uint32_t size){
    cnf_spi_pins();
    NRF_SPIM0->EVENTS_END = 0; // clear event register

    // configure TX buffer
    NRF_SPIM0->TXD.PTR = (uint32_t)arr; 
    NRF_SPIM0->TXD.MAXCNT = (size); 
    
    NRF_SPIM0->ENABLE = 0b111; // enable SPI peripheral

    NRF_SPIM0->TASKS_START = 1; // start sending array

    while(NRF_SPIM0->EVENTS_END==0); // busy wait (for now)

    NRF_SPIM0->TASKS_STOP = 1; // stop sending when buffer is sent
    
    NRF_SPIM0->ENABLE = 0; // disable peripheral
}
