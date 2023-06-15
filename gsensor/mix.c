#include "ADXL345.h"
#include "ADXL345.h"
#include <stdio.h>



#define HW_REGS_BASE_ ( 0xff200000 )
#define HW_REGS_SPAN_ ( 0x00200000 )
#define HW_REGS_MASK_ ( HW_REGS_SPAN - 1 )
#define LED_PIO_BASE 0x0
#define SW_PIO_BASE 0x40

int main(void){

    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];
    volatile unsigned int *h2p_lw_led_addr=NULL;
    volatile unsigned int *h2p_lw_sw_addr=NULL;
    void *virtual_base;
    int fd;
#include "ADXL345.h"
#include <stdio.h>



#define HW_REGS_BASE_ ( 0xff200000 )
#define HW_REGS_SPAN_ ( 0x00200000 )
#define HW_REGS_MASK_ ( HW_REGS_SPAN - 1 )
#define LED_PIO_BASE 0x0
#define SW_PIO_BASE 0x40

int main(void){

    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];
    volatile unsigned int *h2p_lw_led_addr=NULL;
    volatile unsigned int *h2p_lw_sw_addr=NULL;
    void *virtual_base;
    int fd;
include <stdio.h>



#define HW_REGS_BASE_ ( 0xff200000 )
#define HW_REGS_SPAN_ ( 0x00200000 )
#define HW_REGS_MASK_ ( HW_REGS_SPAN - 1 )
#define LED_PIO_BASE 0x0
#define SW_PIO_BASE 0x40

int main(void){

    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];
    volatile unsigned int *h2p_lw_led_addr=NULL;
    volatile unsigned int *h2p_lw_sw_addr=NULL;
    void *virtual_base;
    int fd;
    // For the bubble level, we will do a 10x10 grid output to console
    int x,y,z,i;
   if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
        printf( "ERROR: could not open \"/dev/mem\"...\n" );
        return( 1 );
    } 
    // Map physical addresses
    Map_Physical_Addrs();
    
    // Configure Pin Muxing
    Pinmux_Config();
    
    // Initialize I2C0 Controller
    I2C0_Init();
    
    // 0xE5 is read from DEVID(0x00) if I2C is functioning correctly
    ADXL345_REG_READ(0x00, &devid);

    virtual_base = mmap( NULL, HW_REGS_SPAN_, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE_ );

    h2p_lw_led_addr=(unsigned int *)(virtual_base + (( LED_PIO_BASE ) & ( HW_REGS_MASK_ ) ));
    h2p_lw_sw_addr=(unsigned int *)(virtual_base + (( SW_PIO_BASE ) & ( HW_REGS_MASK_ ) ));
    
     
        

       // Correct Device ID
    if (devid == 0xE5){
    
        printf("Device ID Verified\n");
    
        // Initialize accelerometer chip
        ADXL345_Init();
        
        printf("ADXL345 Initialized\n");
     // volatile unsigned int temp = *h2p_lw_led_addr; 
        while(1){
	//printf("%d", temp);
	//sleep(1);
            if (ADXL345_IsDataReady()){
                ADXL345_XYZ_Read(XYZ);

                              
                // Limit to +/- 1000mg, which means +/-250 in XYZ
                // output: limit to 20x10 grid (since chars are taller vertically)
                x = (XYZ[0] + 250)/25;
                y = (XYZ[1] + 250)/50;
                z = (XYZ[2] + 250)/50;
                x = (x > 20) ? 20 : (x < 0 ? 0 : x);
                y = (y > 10) ? 10 : (y < 0 ? 0 : y);
                z = (z > 10) ? 10 : (z < 0 ? 0 : z);
		

		/* unsigned int *buff = 0;
		if(x > 0 && x <=10)
		*buff = 1;
		else
		*buff = 0;
*/
		if(x > 1 && x<=8){
		  *h2p_lw_led_addr = 1023;
			

		}else{
			*h2p_lw_led_addr = 0;

            	}    
                printf("\033[2J\033[%d;%dH",(10-y)+1,x+1);
                printf("o");
                printf("\033[12;1HX=%d mg, Y=%d mg, Z=%d mg", XYZ[0]*mg_per_lsb, XYZ[1]*mg_per_lsb, XYZ[2]*mg_per_lsb);
                fflush(stdout);
            }
        }
    } else {
        printf("Incorrect device ID\n");
    }
    
    Close_Device();
    close(fd);
    return 0;
}
