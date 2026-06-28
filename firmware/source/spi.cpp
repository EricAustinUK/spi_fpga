#include "nrf.h"
#include "camera_regs.h"

#define CAM_CSN 9
#define NANO_CSN 4


void cnf_spi_pins(){
    static volatile bool cfgd = false;
    if(cfgd) return;
    cfgd = true;

    NRF_SPIM3->PSEL.MOSI = 2; // send bits over external pin 0
    NRF_SPIM3->PSEL.SCK  = 3; // send clock signal over external pin 1
    NRF_SPIM3->PSEL.MISO = 34;  // set MISO to external pin 16
    NRF_SPIM3->PSEL.CSN = 0xFFFFFFFF; // blank to manually set CS pins now
    NRF_SPIM3->FREQUENCY = 0x80000000; // frequency to 8MHz
    NRF_SPIM3->CONFIG = 0b000;
    NRF_SPIM3->RXD.MAXCNT = 0; // TODO: change to allow buffering of image

    NRF_SPIM3->ENABLE = 0b111; // enable SPI peripheral

    // configure ext p0 and p1 as outputs
    NRF_P0->PIN_CNF[2] = 1;
    NRF_P0->PIN_CNF[3] = 1;

    // set chip selects high
    NRF_P0->PIN_CNF[NANO_CSN] = 1;
    NRF_P0->PIN_CNF[CAM_CSN] = 1;
    NRF_P0->OUTSET = (1 << NANO_CSN) | (1 << CAM_CSN);
}

void spi_send_arr(volatile uint32_t arr_ptr, uint32_t size, bool is_to_nano){
    cnf_spi_pins();

    // clear event registers
    NRF_SPIM3->EVENTS_END = 0;

    if(is_to_nano) *((uint32_t *) arr_ptr) = __REV(arr_ptr);
    // configure TX buffer
    NRF_SPIM3->TXD.PTR = arr_ptr; 
    NRF_SPIM3->TXD.MAXCNT = (size) * sizeof(uint32_t); 

    NRF_P0->OUTCLR = (1 << (is_to_nano ? NANO_CSN : CAM_CSN)); // set CS low
    
    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_END==0); // busy wait for event to end

    NRF_P0->OUTSET = (1 << (is_to_nano ? NANO_CSN : CAM_CSN)); // set CS high

    NRF_SPIM3->TASKS_STOP = 1; // stop sending when buffer is sent
}

void send_i2c_cmd(volatile sensor_reg * data_ptr){
    NRF_TWIM0->TXD.MAXCNT = 2; // command is two instructions in memory
    NRF_TWIM0->TXD.PTR = (uint32_t) data_ptr; // set pointer to command
    NRF_TWIM0->EVENTS_STOPPED = 0; // reset flag
    NRF_TWIM0->EVENTS_LASTTX = 0;

    NRF_TWIM0->TASKS_STARTTX = 1; // start transfer over SCCB
    
    while(NRF_TWIM0->EVENTS_LASTTX==0); // busy wait for final byte

    NRF_TWIM0->TASKS_STOP = 1; // issue stop

    while(NRF_TWIM0->EVENTS_STOPPED==0); // busy wait for stop

    NRF_TWIM0->TASKS_STOP = 1; // stop sending after its finished

}

void cnf_camera(){
    static volatile bool cfgd = false;
    if(cfgd) return;
    cfgd = true;

    NRF_TWIM0->PSEL.SCL = 26; // set i2c scl to external pin 20
    NRF_TWIM0->PSEL.SDA = 32; // set i2c sda to external pin 19
    NRF_TWIM0->FREQUENCY = 0x06400000; // frequency to 400kHz
    NRF_TWIM0->ENABLE = 0b110; // enable twim peripheral
    NRF_TWIM0->ADDRESS = 0x30; // set to arducams address

    for(uint32_t i = 0; !(OV2640_JPEG_INIT[i].reg == 0xFF && OV2640_JPEG_INIT[i].val == 0xFF); i++){
        volatile sensor_reg * row_ptr = &OV2640_JPEG_INIT[i];
        send_i2c_cmd(row_ptr);
    }    

    NRF_TWIM0->ENABLE = 0; // disable twim peripheral
}

void write_cam_reg(sensor_reg reg){
    spi_send_arr((uint32_t) &reg, 2, false);
}

uint8_t read_cam_reg(uint8_t reg){
    // clear event registers
    NRF_SPIM3->EVENTS_END = 0;
    volatile uint16_t send_packet = (reg << 8);
    // configure TX buffer
    NRF_SPIM3->TXD.PTR = (uint32_t) &send_packet; 
    NRF_SPIM3->TXD.MAXCNT = 2; 

    volatile uint8_t value;

    NRF_SPIM3->RXD.MAXCNT = 1;
    NRF_SPIM3->RXD.PTR = (uint32_t) &value;

    NRF_P0->OUTCLR = (1 << CAM_CSN); // set CS low
    
    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_END==0); // busy wait for end of rxd and txd buffers

    NRF_SPIM3->TASKS_STOP = 1; // stop when both happen

    NRF_P0->OUTSET = (1 << CAM_CSN); // set CS high
    
    return value;
}

void get_img(){
    cnf_spi_pins();

    write_cam_reg({ 0x04, 0x01 });  // clear FIFO
    write_cam_reg({ 0x04, 0x01 });  // clear FIFO flag? its in arducam's official github...
    write_cam_reg({ 0x04, 0x02 });  // start capture
    

    while(!read_cam_reg(0x41)&0x08); // 0x41 is the trigger register and 0x08 is the bitmask for the capture done flag

    // read each byte of the image buffer's size in memory
    volatile uint8_t size1 = read_cam_reg(0x42);
    volatile uint8_t size2 = read_cam_reg(0x43);
    volatile uint8_t size3 = read_cam_reg(0x44);

    volatile uint32_t size = ((size3 << 16) | (size2 << 8) | size1) & 0x07ffffff; // mask because size3 may contain random data in its more significant bits
}

void spi_get_img(){
    cnf_spi_pins();
    cnf_camera();
    get_img();  
}