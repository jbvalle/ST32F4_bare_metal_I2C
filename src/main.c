/**
 *@file main.c
 *@brief Register Level Programming Simple Blink Project
 *@include main.c
 **/

#include <stdint.h>
#include "peripherals.h"

#define MODER 2
#define pin5 5

#define CR1_SWRST       15
#define CR1_ACK         10
#define CR1_STOP         9
#define CR1_START        8
#define CR1_PE           0

#define SR1_TxE         7
#define SR1_RxNE        6
#define SR1_BTF         2
#define SR1_ADDR        1
#define SR1_SB          0

#define SR2_BUSY        1


RCC_t   * const RCC     = (RCC_t    *)  0x40023800; 
I2C_t   * const I2C1    = (I2C_t    *)  0x40005400;
GPIOx_t * const GPIOA   = (GPIOx_t  *)  0x40020000;
GPIOx_t * const GPIOB   = (GPIOx_t  *)  (0x40020000 + 0x400);

void wait_ms(int time){
    for(int i = 0; i < time; i++){
        for(int j = 0; j < 1600; j++);
    }
}


void I2C1_init(void){
    RCC->RCC_AHB1ENR |= 2;                  //Enable GPIOB Clock
    RCC->RCC_APB1ENR |= (1 << 21);          //Enable I2C Clock

    /* Configure PB8, PB9 */
    GPIOB->GPIOx_AFRH &= ~0xFF;             //Set Alternate Function AF04 for PB8, PB9
    GPIOB->GPIOx_AFRH |= 0x44;                                        
    GPIOB->GPIOx_MODER &= ~(3 << (2 * 8));  //Set MODER to use AF
    GPIOB->GPIOx_MODER &= ~(3 << (2 * 9));
    GPIOB->GPIOx_MODER |= (2 << (2 * 8));
    GPIOB->GPIOx_MODER |= (2 << (2 * 9));

    GPIOB->GPIOx_OTYPER |= (1 << 8);        //Set PB8,9 to OpenDrain
    GPIOB->GPIOx_OTYPER |= (1 << 9);        
    GPIOB->GPIOx_PUPDR &= ~(0xF << 16);     //Set PB8,9 to use Pullup resistors
    GPIOB->GPIOx_PUPDR |=  (1 << (2 * 8));     
    GPIOB->GPIOx_PUPDR |=  (1 << (2 * 9));     

    I2C1->I2C_CR1 = 0x8000;                 //Software Reset
    I2C1->I2C_CR1 &= ~0x8000;               //deactivate RESET
    I2C1->I2C_CR2 = 0x0010;                 //Set FREQ to 16MHz
    I2C1->I2C_CCR = 80;                     //Set Standard Mode 100kHz
    I2C1->I2C_TRISE = 17;                   //Set maximum rise time
    I2C1->I2C_CR1 |= 0x0001;                //enable I2C1 Module
}

int I2C1_byte_read(char slave_addr, char mem_addr, uint8_t *data){

    volatile int tmp;

    while(I2C1->I2C_SR2 & (1 << SR2_BUSY));               //Wait until bus is not busy

    I2C1->I2C_CR1 |= (1 << CR1_START);                 //Generate Start Signal

    while(!(I2C1->I2C_SR1 & (1 << SR1_SB)));            //Wait until START is set

    I2C1->I2C_DR = slave_addr << 1;         //Transmit SLA+W
    while(!(I2C1->I2C_SR1 & (1 << SR1_ADDR)));            //Wait for ACK
    tmp = I2C1->I2C_SR2;

    while(!(I2C1->I2C_SR1 & (1 << SR1_TxE)));         //Wait for TxNE is set
    I2C1->I2C_DR = mem_addr;                //SEND MEMORY ADDRESS
    while(!(I2C1->I2C_SR1 & (1 << SR1_TxE)));         //Wait for TxNE is set

    I2C1->I2C_CR1 |= (1 << CR1_START);                 //Generate RESTART
    while(!(I2C1->I2C_SR1 & 1));            //Wait until START is set

    I2C1->I2C_DR = slave_addr << 1 | 1;     //Transmit SLA+R
    while(!(I2C1->I2C_SR1 & (1 << SR1_ADDR)));            //Check if ACK recieved
    I2C1->I2C_CR1 &= ~(1 << CR1_ACK);                //Disable ACK from MASTER
    tmp = I2C1->I2C_SR2;                    //RESET ADDR FLAG by reading SR2

    I2C1->I2C_CR1 |= (1 << CR1_STOP);                 //Generate STOP

    while(!(I2C1->I2C_SR1 & (1 << SR1_RxNE)));         //Wait until RxNE flag is set
    *data = I2C1->I2C_DR;                   //Read Data

    return 0;
}

int I2C1_byte_write(char slave_addr, char mem_addr, uint8_t data){

    volatile int tmp;

    while(I2C1->I2C_SR2 & (1 << SR2_BUSY));               //Wait until bus is not busy

    I2C1->I2C_CR1 |= (1 << CR1_START);                 //Generate Start Signal

    while(!(I2C1->I2C_SR1 & (1 << SR1_SB)));            //Wait until START is set
                                                        //
    I2C1->I2C_DR = slave_addr << 1;                     //Send SLA + WRITE
    while(!(I2C1->I2C_SR1 & (1 << SR1_ADDR)));          //Wait until ADDR matched
    tmp = I2C1->I2C_SR2;                                //Clear ADDR Bit

    while(!(I2C1->I2C_SR1 & (1 << SR1_TxE)));           //Wait until Tx Register Empty
    I2C1->I2C_DR = mem_addr;                            //Send Memory Address
                                                        //
    while(!(I2C1->I2C_SR1 & (1 << SR1_TxE)));           //Wait until Tx Register Empty
    I2C1->I2C_DR = data;

    while(!(I2C1->I2C_SR1 & (1 << SR1_BTF)));           //Wait until all the Transmit Bytes have been sent
    I2C1->I2C_CR1 |= (1 << CR1_STOP);                   //Generate STOP Signal
    return 0;
}
void configure_i2c_pullup(void){

    RCC->RCC_AHB1ENR |= 1;

    GPIOA->GPIOx_MODER &= ~(3 << (8 * 2));      //Set PA8,9 as OUTPUT
    GPIOA->GPIOx_MODER &= ~(3 << (9 * 2));
    GPIOA->GPIOx_MODER |= (1 << (8 * 2));
    GPIOA->GPIOx_MODER |= (1 << (9 * 2));

    GPIOA->GPIOx_PUPDR &= ~(3 << (8 * 2));      //Set PULLUP 
    GPIOA->GPIOx_PUPDR &= ~(3 << (9 * 2));
    GPIOA->GPIOx_PUPDR |=  (1 << (8 * 2));
    GPIOA->GPIOx_PUPDR |=  (1 << (9 * 2));

    GPIOA->GPIOx_OTYPER |= (1 << 8);            //Set PA8,9 as Opendrain 
    GPIOA->GPIOx_OTYPER |= (1 << 9);

    GPIOA->GPIOx_ODR |= (1 << 8);               //Set PA8,9 to HIGH
    GPIOA->GPIOx_ODR |= (1 << 9);
}

int main(void){

    uint8_t data;
    uint8_t x_coord;
    uint8_t y_coord;

    configure_i2c_pullup();

    I2C1_init();

    //Enable Clock to GPIOA Peripheral
    RCC->RCC_AHB1ENR |= 1;

    // Reset MODER Bitfield of PA5
    GPIOA->GPIOx_MODER &= ~(3 << (pin5 * MODER));
    // Write Value 1 to MODER Bitfield PA5 to configure PA5 as OUTPUT
    GPIOA->GPIOx_MODER |=  (1 << (pin5 * MODER));

    I2C1_byte_read(0x40, 0x0F, &data);
    I2C1_byte_write(0x40, 0x2E, 0x84);

    for(;;){


        I2C1_byte_read(0x40, 0x10, &x_coord);
        I2C1_byte_read(0x40, 0x11, &y_coord);

        // Toggle PA5)
        GPIOA->GPIOx_ODR ^= (1 << pin5);
        // Wait for 100ms
        wait_ms(100);
    }
}
