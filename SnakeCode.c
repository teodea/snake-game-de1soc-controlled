#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>
#include "address_map_arm_brl4.h"

#define HW_REGS_BASE ( 0xff200000 )
#define HW_REGS_SPAN ( 0x00200000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define LED_PIO_BASE 0x0
#define SW_PIO_BASE 0x40
#define KEY_PIO_BASE 0x50

#define BLACK 0x0000 	
#define NAVY 0x000F 	
#define DGREEN 0x03E0 	
#define DCYAN 0x03EF 	
#define MAROON 0x7800 	
#define PURPLE 0x780F 	
#define OLIVE 0x7BE0 	
#define DGRAY 0x7BEF 	 
#define BLUE 0x001F 	
#define GREEN 0x07E0 	
#define CYAN 0x07FF 	 
#define RED 0xF800 	
#define MAGENTA 0xF81F 	
#define YELLOW 0xFFE0 	
#define WHITE 0xFFFF	
#define LGRAY 0xC618
#define UMORANGE 0xF321
#define UMGREEN 0x03E0

/* function prototypes */
void VGA_box(int, int, int, int, short);
void VGA_line(int, int, int, int, short);
void VGA_text(int, int, char *);
void VGA_text_clear();

// virtual to real address pointers
volatile unsigned int *red_LED_ptr = NULL;
void *h2p_lw_virtual_base;
volatile unsigned int *vga_pixel_ptr = NULL;
void *vga_pixel_virtual_base;
volatile unsigned int *vga_char_ptr = NULL;
void *vga_char_virtual_base;
void *h2p_lw_sw_addr = NULL;
void *h2p_lw_key_addr = NULL;
int fd;

struct eaten {
    int x;
    int y;
};

void youLost() {
    VGA_text_clear();
    VGA_text(0, 0, "Game Over");
    drawUM();
}

int fruit(int x, int y, int * cx, int * cy) {
    if((x<(*cx)+4 && x>(*cx)-4) && (y<(*cy)+4 && y>(*cy)-4)) {
        *cx = rand()%316;
        *cy = rand()%236; 
        return 1;
    } else {
        VGA_box(0+(*cx), 0+(*cy), (*cx)+4, (*cy)+4, OLIVE);
        return 0;
    }
}

int bad_fruit(int x, int y, int * cx, int * cy){
   if((x<(*cx)+5 && x>(*cx)-5) && (y<(*cy)+5 && y>(*cy)-5)) {
        *cx = rand()%316;
        *cy = rand()%236; 
        return 1;
    } else {
        VGA_box(0+(*cx), 0+(*cy), (*cx)+4, (*cy)+4, PURPLE);
        return 0;
    }
}

void touch(struct eaten *position, int x1, int y1, int x2, int y2){
    if(position[0].x>x1 && position[0].x<x2 && position[0].y>y1 && position[0].y<y2){
            youLost();
            exit(1);
        }
}

void drawSnake(struct eaten *position, int size, int direction) {
    for (int i=size-1; i>0; i--) {
        position[i].x = position[i-1].x;
        position[i].y = position[i-1].y;
    }
    if(direction==1) {
        usleep(1000);
        position[0].y = position[0].y - 1;
    }
    if(direction==2) {
        usleep(1000);
        position[0].x = position[0].x - 1;
    }
    if(direction==3) {
        usleep(1000);
        position[0].x = position[0].x + 1;
    }
    if(direction==4) {
        usleep(1000);
        position[0].y = position[0].y + 1;
    }
    for (int i=0; i<size; i++){
        VGA_box(position[i].x, position[i].y, position[i].x+4, position[i].y+4, GREEN);
    }
}

void drawUM(){
    int offsetX = 319/2 - 52;
    int offsetY = 238/2 - 64;
    // left side UM
    VGA_box(offsetX, offsetY, offsetX+24, offsetY+56, UMORANGE);
    VGA_box(offsetX+4, offsetY+56, offsetX+24+4, offsetY+56+4, UMORANGE);
    VGA_box(offsetX+4+4, offsetY+56+4, offsetX+24+4+4, offsetY+56+4+4, UMORANGE);
    VGA_box(offsetX+4+4+8, offsetY+56+4+4, offsetX+24+4+4+36, offsetY+56+4+4+4, UMORANGE);
    VGA_box(offsetX+4+4+8+8, offsetY+56+4+4+4, offsetX+4+4+8+8+28, offsetY+56+4+4+4+4, UMORANGE);
    // right side UM
    VGA_box(offsetX+52, offsetY+64, offsetX+52+28, offsetY+64+8, UMGREEN);
    VGA_box(offsetX+52+20, offsetY+64-4, offsetX+52+20+16, offsetY+64-4+8, UMGREEN);
    VGA_box(offsetX+52+20+4, offsetY+64-4-4, offsetX+52+20+4+20, offsetY+64-4-4+8, UMGREEN);
    VGA_box(offsetX+52+20+4, offsetY+64-4-4, offsetX+52+20+4+24, offsetY+64-4-4+4, UMGREEN);
    VGA_box(offsetX+52+20+4+4, offsetY, offsetX+52+20+4+4+24, offsetY+56, UMGREEN);
}

int speed_regulator(int sw) {
    int speed = 0;
    if(sw&0x1) {
        speed = speed + 1;
    }
    if(sw&0x2) {
        speed = speed + 2;
    }
    if(sw&0x4) {
        speed = speed + 4;
    }
    if(sw&0x8) {
        speed = speed + 8;
    }
    return speed;
}

int main() {
    int size = 1;
    struct eaten position[100];
    if((fd = open("/dev/mem", (O_RDWR|O_SYNC)))==-1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return(1);
    }
    // get virtual addr that maps to physical
    h2p_lw_virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, HW_REGS_BASE);    
    if(h2p_lw_virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return(1);
    }
    vga_pixel_virtual_base = mmap(NULL, FPGA_ONCHIP_SPAN, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, FPGA_ONCHIP_BASE);    
    if(vga_pixel_virtual_base==MAP_FAILED) {
        printf("ERROR: mmap3() failed...\n");
        close(fd);
        return(1);
    }
    vga_pixel_ptr = (unsigned int *)(vga_pixel_virtual_base);
    vga_char_virtual_base = mmap(NULL, FPGA_CHAR_SPAN, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, FPGA_CHAR_BASE);   
    if(vga_char_virtual_base==MAP_FAILED) {
        printf("ERROR: mmap2() failed...\n");
        close(fd);
        return(1);
    }
    vga_char_ptr = (unsigned int *)(vga_char_virtual_base);
    h2p_lw_key_addr = (unsigned int *)(h2p_lw_virtual_base + ((KEY_PIO_BASE) & (HW_REGS_MASK)));
    h2p_lw_sw_addr = (unsigned int *)(h2p_lw_virtual_base + ((SW_PIO_BASE) & (HW_REGS_MASK)));
    int xcount = 0;
    int ycount = 0;
    int previous = 3; // direction right
    time_t t;
    srand((unsigned) time(&t));
    int fruitx = rand()%319;
    int fruity = rand()%238;
    int bad_fruitx1 = rand()%319;
    int bad_fruity1 = rand()%238;
    int bad_fruitx2 = rand()%319;
    int bad_fruity2 = rand()%238;
    position[0].x = 50;
    position[0].y = 30;
    while(1) {
        if(size==10+1) {
            VGA_text_clear();
            VGA_text(0, 0, "YOU WIN");
            drawUM();
            break;
        }
        VGA_text_clear();
        char *score;
        score = (char*) malloc(sizeof(char)*50);
        sprintf(score, "Score %d", size-1);
        VGA_text(0, 0, score);
        usleep(10000);
        VGA_box(0, 0, 319, 239, 0); // update the screen
        int input = *(unsigned*) h2p_lw_key_addr; // button
        int level = *(unsigned*) h2p_lw_sw_addr; // switch
        int speed_choice = speed_regulator(level); // speed
        if(level&0x10) { // 0000010000 L1
           drawUM();
        }
        if(level&0x20) { // 0000100000 L2
            VGA_box(70, 50, 90, 55, RED);
            VGA_box(170, 200, 190, 205, RED);
            touch(position, 70, 50, 90, 55);
            touch(position, 170, 200, 190, 205);
        }
        if(level&0x40) { // 0001000000 L3
            VGA_box(70, 50, 90, 55, RED);
            VGA_box(170, 200, 190, 205, RED);
            VGA_box(110, 140, 115, 155, RED);
            VGA_box(250, 180, 270, 185, RED);
            touch(position, 70, 50, 90, 55);
            touch(position, 170, 200, 190, 205);
            touch(position, 110, 140, 115, 155);
            touch(position, 250, 180, 270, 185);
        }
        if(level&0x80) { // 0010000000 bad_fruit
            int bad_ate1 = bad_fruit(position[0].x, position[0].y, &bad_fruitx1, &bad_fruity1);
            int bad_ate2 = bad_fruit(position[0].x, position[0].y, &bad_fruitx2, &bad_fruity2);
            if(bad_ate1==1 || bad_ate2==1){
                size--;
                if(size<1)
                {youLost();
                }
            }
        }
        if(input==0x04) { // moving right
            previous = 3;
            for(int i=0; i<=speed_choice; i++) {
                position[0].x++;
                xcount++;
            }
        } else if(input==0x01) { // moving down
            previous = 1;
            for(int i=0; i<=speed_choice; i++) {
                position[0].y--;
                ycount--;
            }
        } else if(input==0x08) { // moving left
            previous = 2;
            for(int i=0; i<=speed_choice; i++) {
                position[0].x--;
                xcount--;
            }
        } else if(input==0x02) { // moving up
            previous = 4;
            for(int i=0; i<=speed_choice; i++) {
                position[0].y++;    
                ycount++;
            }
        } else {
            if(previous==1) {
                previous = 1;
                for(int i=0; i<=speed_choice; i++) {
                    position[0].y--;
                    ycount--;
                }
            }
            if(previous==2) {
                previous = 2;
                for(int i=0; i<=speed_choice; i++) {
                    position[0].x--;
                    xcount--;
                }
            }
            if(previous==3) {
                previous = 3;
                for(int i=0; i<=speed_choice; i++) {
                    position[0].x++;
                    xcount++;
                }
            }
            if(previous==4) {
                previous = 4;
                for(int i=0; i<=speed_choice; i++) {
                    position[0].y++;
                    ycount++;
                }
            }
        }
        if(position[0].x>319) {
            youLost();
            break;
        }
        if(position[0].y>239) {
            youLost();
            break;
        }
        if(position[0].x<0) {
            youLost();
            break;
        }
        if(position[0].y<0) {
            youLost();
            break;
        }
        drawSnake(position,size,previous);
        int ate = fruit(position[0].x, position[0].y, &fruitx, &fruity);
        if(ate==1) {
            size++;
            VGA_text_clear();
            score = (char*) malloc(sizeof(char)*50);
            sprintf(score, "Score %d", size);
            VGA_text(0, 0, score);
        }
    }
}

void VGA_text(int x, int y, char *text_ptr) {
    int offset;
    volatile char *character_buffer = (char *) vga_char_ptr ;  // VGA character buffer
    /* assume that the text string fits on one line */
    offset = (y << 7) + x;
    while(*(text_ptr)) {
        // write to the character buffer
        *(character_buffer + offset) = *(text_ptr); 
        ++text_ptr;
        ++offset;
    }
}

void VGA_text_clear() {
    volatile char *character_buffer = (char *) vga_char_ptr ;  // VGA character buffer
    int offset, x, y;
    for(x=0; x<79; x++) {
        for(y=0; y<59; y++) {
            /* assume that the text string fits on one line */
            offset = (y << 7) + x;
            // write to the character buffer
            *(character_buffer + offset) = ' ';     
        }
    }
}

void VGA_box(int x1, int y1, int x2, int y2, short pixel_color) {
    char *pixel_ptr;
    int row, col;
    /* assume that the box coordinates are valid */
    for(row=y1; row<=y2; row++) {
        for(col=x1; col<=x2; ++col) {
            pixel_ptr = (char *)vga_pixel_ptr + (row << 10) + (col << 1); 
            //set pixel color
            *(short *)pixel_ptr = pixel_color;      
        }
    }
}