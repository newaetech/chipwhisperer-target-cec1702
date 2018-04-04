/*****************************************************************************
 * © 2017 Microchip Technology Inc. and its subsidiaries.
 * You may use this software and any derivatives exclusively with
 * Microchip products.
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".
 * NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP
 * PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
 * TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
 * CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF
 * FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE
 * OF THESE TERMS.
 *****************************************************************************/

/** @file otp_progm.c
 *MEC2016 otp_progm
 */
/** @defgroup MEC2016 otp_progm
 */


/* Hardware includes. */
#include "efuse_data.h"
#include    <stdio.h>
#define TRUE    1
#define FALSE   0
#define HIGH    1
#define LOW     0
#define MEC2016_PERIPH_BASE      (0x40000000UL)                              /*!< (Peripheral) Base Address */
#define MEC2016_PERIPH_SPB_BASE  ((MEC2016_PERIPH_BASE) + (0x00080000UL))
#define MEC2016_EFUSE_BASE       (MEC2016_PERIPH_SPB_BASE + 0x2000UL) /*!< (eFUSE ) Base Address  0x40082000     */

#define MEC2016_EFUSE       ((MEC2016_EFUSE_TypeDef *) MEC2016_EFUSE_BASE)
#define EFUSE_BITLEN        (4096ul)
#define EFUSE_BYTELEN       (4096ul >> 3)
#define EFUSE_HWORDLEN      (4096ul >> 4)

typedef unsigned          int uint32_t;
typedef volatile unsigned char     vuint8_t;

#define ADDR_CEC1702_CLOCKSET      0x4000A414UL
#define MMCR_CEC1702_CLOCKSET      (*(vuint8_t *)(ADDR_CEC1702_CLOCKSET))

typedef struct
{
     volatile uint32_t CONTROL;                  /*!< Offset: 0x0000   Control  */
     volatile uint16_t MAN_CONTROL;              /*!< Offset: 0x0004   Manual Control  */
     volatile uint16_t MAN_MODE;                 /*!< Offset: 0x0006   Manual Mode  */
     volatile uint32_t RESERVED[2];              /*!< Offset: 0x0006   Manual Mode  */
     union {
          volatile uint32_t MAN_DATA;               /*!< Offset: 0x0010   Manual Mode Data */
          volatile uint16_t MEM16[EFUSE_HWORDLEN];  /*!< Offset: 0x0010   Memory mapped bit-array 8 or 16-bit accessible  */
          volatile uint8_t  MEM8[EFUSE_BYTELEN];
     };
} MEC2016_EFUSE_TypeDef;
/*@}*/ /* end of group MEC2016_EFUSE */

void efuse_the_device(void);
void set_up_the_external_volts_pins(void);
void program_the_device(void);
void reverify_efuse_data(void);
void mainTOGGLE_LED(uint8_t loop) ;
void efuse_main( void );
void SmallDelay (uint32_t delay);
void pre_qualify_before_key_write(void);
void InitGPIO(void);

uint8_t buff1[512];
uint8_t proceed = 0 ;
uint8_t key_info = 0 ;

uint8_t override  = 0;

#define ENCRYPTION_KEY_EXISTS       (1U<<0U)
#define AUTHNETICATION_KEY_EXISTS   (1U<<1U)

/******************************************************************************/
/** main
 * Initilaize and call the main routine for the efuse programing up on successful
 * completion the LED 156 is set to low solid glow state
 * @param void
 * @return void
 *******************************************************************************/
void main(void) {
     InitGPIO();     //Initilaize all the GPIO's
     GPIO_OUTPUT_120_127.B5 = 1;   // Set for clicker 1.03 for the UART pin setting
     GPIO_OUTPUT_150_157.B7 = 0;   //GPIO for the LED
     MMCR_CEC1702_CLOCKSET = 1;
     UART0_Init(57600);
     efuse_main();
     UART0_Write_Text("DONE LEDS ON\n");
     GPIO_OUTPUT_150_157.B7 = 1;
     while(1){;  }
}

/******************************************************************************/
/** efuse_main
 * Initilaize and call the main routine for the efuse programing up on successful
 * completion the LED 156 is set to low solid glow state
 * @param void
 * @return void
 *******************************************************************************/
void efuse_main( void )
{
     key_info = 0 ;
     GPIO_OUTPUT_150_157.B6 =  1;
     UART0_Write_Text("Starting Efuse Programming\n");
     efuse_the_device();
     UART0_Write_Text("Starting Efuse Programming -Done\n");
     GPIO_OUTPUT_150_157.B6 =  0; //Glow the LED for completion of the Program
     //for( ;; );
}

/******************************************************************************/
/** efuse_the_device
 * The efuse block registers and power rails for the blocks are initialized and
 * get ready for the efuse programing. After programming the device read back the
 * content for data validity and diable the block for low power consumption
 * @param void
 * @return void
 *******************************************************************************/
void efuse_the_device(void){

     MEC2016_EFUSE_TypeDef *EFUSE = (MEC2016_EFUSE_TypeDef*) MEC2016_EFUSE_BASE;

     set_up_the_external_volts_pins();
     GPIO_OUTPUT_050_057.B0 =  0;
     GPIO_OUTPUT_050_057.B1 =  0;

     pre_qualify_before_key_write();

     EFUSE->CONTROL =  0x11;                //enable block
     EFUSE->MAN_CONTROL =  0x01; //enable manual control
     EFUSE->CONTROL =  0x01;            //clear FSOURCE_EN_READ
     //EFUSE->CONTROL =  0x19;            //FSOURCE_EN_PRGM - connects eFuse FSOURCE pin to power pad
     while(EFUSE->CONTROL != 0x01);
     EFUSE->CONTROL =  0x09;     //clear FSOURCE_EN_READ
     //turn On external voltage
     GPIO_OUTPUT_050_057.B0 =  1;
     SmallDelay(30);
     EFUSE->MAN_CONTROL = 0x03;  //enable IP_CS
     //Program the Efuse
     program_the_device();

     //turn off external voltage
     GPIO_OUTPUT_050_057.B0 =  0;
     //EFUSE->CONTROL =  0x19;
     EFUSE->CONTROL = 0x01;
     while (EFUSE->CONTROL != 0x01);
     EFUSE->CONTROL = 0x11;
     EFUSE->MAN_CONTROL = 0x01;
     EFUSE->MAN_MODE= 0;
     EFUSE->MAN_CONTROL = 0;

     reverify_efuse_data();
     EFUSE->CONTROL =  0;
}

/******************************************************************************/
/** reverify_efuse_data
 * read back the programmed data to make sure the code is executed proper and the
 * efuse is programmed to the correct value from the table
 * Note: Only one time verification of the efuse is possible following a write. If
 * tried to rerun the program the verfication will fail since botrom will lock the
 * Microchip specific areas. And only when the Chip is in ATE mode this routine will
 * exit with sucess
 * @param void
 * @return void
 *******************************************************************************/
void reverify_efuse_data(void){
     uint8_t tdata, chk_bit;
     uint32_t efuse_base;
     uint16_t tot_len, idx, value,cIdx;

     MEC2016_EFUSE_TypeDef *EFUSE = (MEC2016_EFUSE_TypeDef        *)MEC2016_EFUSE_BASE;
     //read back data
     EFUSE->CONTROL = 0x11; //enable block and read mode)
     efuse_base=(uint32_t)(&EFUSE->MEM8[0]); // Address of eFUSE memory register
     for (idx=0; idx<512; idx++){
          proceed = override;
          cIdx = device_efuse_table_[idx].index;
          tdata = device_efuse_table_[idx].value;
          if ( 0xDEAD == cIdx) break;
          if ((cIdx == 35U)  && (tdata & (1U << 7U))) {
               proceed = TRUE;
               tdata &= (1U << 7U);
          }
          if((cIdx == 34U)  && (tdata & (0x12U))){
               proceed = TRUE;
               tdata &= (0x12U);
          }
          if((cIdx == 483U) && (tdata & (1U << 0U))){
               proceed = TRUE;
               tdata &= (1U << 0U);
          }

          if((31 >= cIdx) && !(key_info & ENCRYPTION_KEY_EXISTS)){
               proceed = TRUE;
          }

          if(((128U <= cIdx) && (cIdx <= 191U)) && !(key_info & AUTHNETICATION_KEY_EXISTS)){
               proceed = TRUE;
          }
          value = *(uint8_t *)(efuse_base+cIdx); // Read 16 bytes of eFUSE Memory
          chk_bit = (value & tdata);
          if(((508U <= cIdx) && (cIdx <= 509U))
             ||((192U <= cIdx) && (cIdx <= 479U))
             || proceed ){
               if ((chk_bit != tdata) && (value != tdata)){
                    UART0_Write_Text("ReVerify failed\n");
                    while(1) {mainTOGGLE_LED(1);}
               }
          }
     }
}

/******************************************************************************/
/** set_up_the_external_volts_pins
 * Set the GPIO's associated with the logic controller to controll the voltage levels
 * @param void
 * @return void
 *******************************************************************************/
void set_up_the_external_volts_pins(void){
     GPIO_Digital_Output(&GPIO_PORT_050_057, _GPIO_PINMASK_0 | _GPIO_PINMASK_1);
}

/******************************************************************************/
/** pre_qualify_before_key_write
 * Perform a check if there is any keys already programmed or not for the image
 * Authentication or for Encryption inthe device and any key is intended to target
 * the device from the provided table
 * @param void
 * @return void
 *******************************************************************************/
void pre_qualify_before_key_write(void){

     uint32_t efuse_base;
     uint16_t tot_len, idx, value,cIdx;

     MEC2016_EFUSE_TypeDef *EFUSE = (MEC2016_EFUSE_TypeDef        *)MEC2016_EFUSE_BASE;
     //read back data
     EFUSE->CONTROL = 0x11; //enable block and read mode)
     efuse_base=(uint32_t)(&EFUSE->MEM8[0]); // Address of eFUSE memory register

     for (idx=0; idx<=512; idx++){
          cIdx = device_efuse_table_[idx].index;
          if ( 0xDEAD == cIdx){
               break;
          }

          value = *(uint8_t *)(efuse_base+cIdx); // Read 16 bytes of eFUSE Memory

          if((31 >= cIdx) && value){
               key_info |= ENCRYPTION_KEY_EXISTS;
          }
          if(((128U <= cIdx) && (cIdx <= 191U)) && value) {
               key_info |= AUTHNETICATION_KEY_EXISTS;
          }
     }

}
/******************************************************************************/
/** program_the_device
 * Perform the efuse programming for the device; get the data fromhe table entry
 * check for only targetted efuse bits\bytes and perform the write operation
 * @param void
 * @return void
 *******************************************************************************/
void program_the_device(void){

     uint8_t tdata, bPos;
     uint16_t tot_len, idx, cIdx;
     uint32_t cbitLoc;
     MEC2016_EFUSE_TypeDef *EFUSE = (MEC2016_EFUSE_TypeDef *)MEC2016_EFUSE_BASE;

     for (idx=0; idx<=512; idx++){
          proceed = override;
          cIdx = device_efuse_table_[idx].index;
          tdata = device_efuse_table_[idx].value;
          if ( 0xDEAD == cIdx) break;
          if ((cIdx == 35U)  && (tdata & (1U << 7U))) { //ATE mode disable
               proceed = TRUE;
               tdata &= (1U << 7U);
               UART0_Write_Text("Disabling ATE Mode\n");
          }
          if((cIdx == 34U)  && (tdata & (0x12U))){ //JTAG Disable or SWD mode enable
               proceed = TRUE;
               tdata &= (0x12U);
          }
          if((cIdx == 483U) && (tdata & (1U << 0U))){ //Enbale Authentication
               proceed = TRUE;
               tdata &= (1U << 0U);
          }
          if((31 >= cIdx) && !(key_info & ENCRYPTION_KEY_EXISTS)){//Check for Encryption keys
               proceed = TRUE;
          }
          //Check for Authentication
          if(((128U <= cIdx) && (cIdx <= 191U)) && !(key_info & AUTHNETICATION_KEY_EXISTS)){
               proceed = TRUE;
          }

          if(((508U <= cIdx) && (cIdx <= 509U)) ||//Check for alternate TAG location
             ((192U <= cIdx) && (cIdx <= 479U)) ||//Check for Custom programmable region
             proceed){                            //Condition form above checking status
               cbitLoc = 1;
               for (bPos=0; bPos<=8; bPos++){
                    if ((tdata & cbitLoc) == 0) {
                         cbitLoc = cbitLoc << 1;
                         continue;
                    }
                    EFUSE->MAN_MODE =(uint16_t)(cIdx*8+bPos);
                    SmallDelay(20);
                    EFUSE->MAN_CONTROL = 0x03; //set IP_CS
                    SmallDelay(20);
                    EFUSE->MAN_CONTROL = 0x07; //set IP_PRGM_EN high
                    SmallDelay(10);
                    EFUSE->MAN_CONTROL = 0x03; //set IP_PRGM_EN low
                    SmallDelay(20);
                    EFUSE->MAN_CONTROL = 0x01; //clear IP_CS
                    SmallDelay(10);
                    cbitLoc = cbitLoc << 1;
               }
          }
     }
}

/******************************************************************************/
/** SmallDelay
 * Perform loop delay for the given delay counter
 * @param delay      Delay count for the loop
 * @return void
 *******************************************************************************/
void SmallDelay (uint32_t delay){

     uint32_t count=0;
     for (count=0; count<delay*10; count++) {;}
}
/******************************************************************************/
/** mainTOGGLE_LED
 * Toggle the leds 156 and 157 for the given loop times
 * @param loop      No of times to toggle the Leds
 * @return void
 *******************************************************************************/
void mainTOGGLE_LED(uint8_t loop){
     uint32_t val, delay, d2;
     /* Prepare the leds to toggle */
     val = 0xFFFFF;
     delay = val;
     while(loop--){
          if(delay & 0x80001) GPIO_OUTPUT_150_157.B6 ^= 1;;
          d2 = val & 0xFFFF;
          while(d2--);
          if(delay & 0x80001) GPIO_OUTPUT_150_157.B7 ^= 1;;
          d2 = val & 0xFFFFF;
          while(d2--);
          delay--;
     }
}
/******************************************************************************/
/** InitGPIO
 * Initialize the required GPIOs
 * @param void
 * @return void
 *******************************************************************************/
void InitGPIO(void) {
     GPIO_Digital_Output(&GPIO_PORT_120_127, _GPIO_PINMASK_5); /// For UART0 enable
     GPIO_Digital_Output(&GPIO_PORT_150_157, _GPIO_PINMASK_6 | _GPIO_PINMASK_7);
}
/* end otp_progm.c */
/**   @}
 */
