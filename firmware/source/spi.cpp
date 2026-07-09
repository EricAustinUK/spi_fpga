#include "nrf.h"
#include "camera_regs.h"

#define CAM_CSN 9
#define NANO_CSN 4
#define IMG_SIZE (320*240/8)
#define BUFFER_COUNT 4
#define BUFFER_SIZE (320*240*2/BUFFER_COUNT)
#define THRESHOLD_SET_COUNT_MIN 2000
#define THRESHOLD_SET_COUNT_MAX 4000


void slp_100(){
    NRF_TIMER2->TASKS_STOP = 1; // stop other timers
    NRF_TIMER2->BITMODE = 3; // 32 bit
    NRF_TIMER2->CC[0] = 100000; // set cmp to time
    NRF_TIMER2->PRESCALER = 4;

    // clear flags
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
    NRF_TIMER2->TASKS_CLEAR = 1;

    // wait for delay to be metd
    NRF_TIMER2->TASKS_START = 1;
    while(NRF_TIMER2->EVENTS_COMPARE[0] == 0);
    
    // clear flags
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
}

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
    NRF_SPIM3->TXD.MAXCNT = size; 

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

    NRF_TWIM0->SHORTS = TWIM_SHORTS_LASTTX_STOP_Msk; // should prevent race condition

    NRF_TWIM0->TASKS_STARTTX = 1; // start transfer over SCCB

    while(NRF_TWIM0->EVENTS_STOPPED==0); // busy wait for stop
}

void cnf_camera(){
    static volatile bool cfgd = false;
    if(cfgd) return;
    cfgd = true;

    NRF_TWIM0->PSEL.SDA = 32; // set i2c sda to external pin 19
    NRF_TWIM0->PSEL.SCL = 26; // set i2c scl to external pin 20
    NRF_TWIM0->FREQUENCY = 0x06400000; // frequency to 400kHz
    NRF_TWIM0->ENABLE = 0b110; // enable twim peripheral
    NRF_TWIM0->ADDRESS = 0x30; // set to arducams address


    volatile sensor_reg init_cmd = { 0x0A, 0x0B };
    volatile sensor_reg select_sensor_bank = { 0xFF, 0x01 };
    volatile sensor_reg reset = { 0x12, 0x80 }; // must send reset for some reason!

    send_i2c_cmd(&init_cmd);
    send_i2c_cmd(&select_sensor_bank);
    send_i2c_cmd(&reset);

    slp_100();

    for(uint32_t i = 0; i < sizeof(OV2640_QVGA)/sizeof(sensor_reg); i++){
        volatile sensor_reg * row_ptr = &OV2640_QVGA[i];
        send_i2c_cmd(row_ptr);
    }   
    
    send_i2c_cmd(&select_sensor_bank);

    for(uint32_t i = 0; i < sizeof(OV2640_YUV422)/sizeof(sensor_reg); i++){
        volatile sensor_reg * row_ptr = &OV2640_YUV422[i];
        send_i2c_cmd(row_ptr);
    }   
    
    
    for(uint32_t i = 0; i < sizeof(OV2640_320x240_JPEG)/sizeof(sensor_reg); i++){
        volatile sensor_reg * row_ptr = &OV2640_320x240_JPEG[i];
        send_i2c_cmd(row_ptr);
    }   

    volatile sensor_reg select_dsp_bank = { 0xFF, 0x00 };
    volatile sensor_reg disable_jpeg = { 0xDA, 0x00 };
    volatile sensor_reg control_bit = { 0xC2, 0x0C };
    

    send_i2c_cmd(&select_dsp_bank);
    send_i2c_cmd(&disable_jpeg);
    send_i2c_cmd(&control_bit);
    

    NRF_TWIM0->ENABLE = 0; // disable twim peripheral
}

void write_cam_reg(sensor_reg reg){
    reg.reg |= 0x80;
    spi_send_arr((uint32_t) &reg, 2, false);
}

uint8_t read_cam_reg(uint8_t reg){
    // clear event registers
    NRF_SPIM3->EVENTS_END = 0;
    volatile uint16_t send_packet = reg | (0x00 << 8);
    // configure TX buffer
    NRF_SPIM3->TXD.PTR = (uint32_t) &send_packet; 
    NRF_SPIM3->TXD.MAXCNT = 2; 

    volatile uint16_t value;

    NRF_SPIM3->RXD.MAXCNT = 2;
    NRF_SPIM3->RXD.PTR = (uint32_t) &value;

    NRF_P0->OUTCLR = (1 << CAM_CSN); // set CS low
    
    NRF_SPIM3->TASKS_START = 1; // start sending array

    while(NRF_SPIM3->EVENTS_END==0); // busy wait for end of rxd and txd buffers

    NRF_SPIM3->TASKS_STOP = 1; // stop when both happen

    NRF_P0->OUTSET = (1 << CAM_CSN); // set CS high
    
    return value >> 8;
}

void spi_recv_img(uint8_t * values, uint8_t * threshold){
    static volatile uint8_t buffer[(BUFFER_SIZE) + 1];
    uint32_t bucket_hist[64];
    
    for(uint8_t i = 0; i < 64; i++) bucket_hist[i] = 0;

    for(uint8_t i = 0; i < BUFFER_COUNT; i++){

        cnf_spi_pins();
        volatile uint8_t burst_cmd = 0x3C;

        // clear event registers
        NRF_SPIM3->EVENTS_END = 0;

        // configure TX buffer
        NRF_SPIM3->TXD.PTR = (uint32_t)&burst_cmd; 
        NRF_SPIM3->TXD.MAXCNT = 1; 
        NRF_SPIM3->ORC = 0x00;

        NRF_SPIM3->RXD.PTR = (uint32_t) &buffer;
        NRF_SPIM3->RXD.MAXCNT = BUFFER_SIZE + 1;

        NRF_P0->OUTCLR = (1 << CAM_CSN); // set CS low
        
        NRF_SPIM3->TASKS_START = 1; // start sending array

        while(NRF_SPIM3->EVENTS_END==0); // busy wait for last TX and RX byte

        NRF_P0->OUTSET = (1 << CAM_CSN); // set CS high

        NRF_SPIM3->TASKS_STOP = 1; // stop sending when buffer is sent
        
        for(uint32_t j = 1; j < BUFFER_SIZE + 1; j+=2){
            uint32_t pixel_num = i*(BUFFER_SIZE/2) + (j-1)/2;
            uint32_t output_index = pixel_num >> 3; 
            uint8_t byte_index = pixel_num & 0x07;
            uint8_t buffer_value = buffer[j];
            bucket_hist[buffer_value >> 3] += 1; 
            values[output_index] &=  ~((buffer_value <= *threshold ? 1 : 0) << (byte_index^0x7));
        }
    }

    uint32_t pixel_acc = 0;
    for(uint8_t i = 0; i < 64; i++){
        pixel_acc += bucket_hist[i];
        if(pixel_acc > THRESHOLD_SET_COUNT_MAX) break;
        if(pixel_acc > THRESHOLD_SET_COUNT_MIN){ 
            *threshold = (i+1) >> 3 -  1;
            break;
        }
    }
}

void get_img(uint8_t * output, uint8_t * threshold){
    cnf_spi_pins();
    cnf_camera();

    for(int i = 0; i < 20; i++)
        slp_100();

    write_cam_reg({ 0x04, 0x01 });  // clear FIFO

    write_cam_reg({ 0x04, 0x02 });  // start capture

    while(!(read_cam_reg(0x41) & 0x08)); // 0x41 is the trigger register and 0x08 is the bitmask for the capture done flag

    spi_recv_img(output, threshold);
}