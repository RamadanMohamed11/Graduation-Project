/*
 *  Slave 1 Node
 *  Author: Hesham Ahmed
 *  Edit date: 2, 2, 2024
 *  Version:20
 */
/*------------------------------ Includes ------------------------------*/
#include <16F628A.h>
#fuses HS,INTRC_IO, NOWDT, NOPROTECT, NOMCLR ,NOLVP
#use delay(CLOCK=4000000)
#define OPTION *((volatile unsigned char*)0x81)
#define RBPU    7
/*------------------------- Macros Declaration -------------------------*/


/*Timer starting count*/
//overflow_width = PRESCALER *(256 - TIMER_PRELOAD)
#define TIMER_PRELOAD   106
#define PRESCALER   T0_DIV_16  //16

/*logical states*/
#define HIGH        1
#define LOW         0

#define TRUE        1
#define FALSE       0


#define DIP_ENABLE_PIN PIN_A3

/*Slave clock input PIN*/
#define S_CLK_PIN       PIN_A2

/*Slave Read PIN*/
#define S_R_PIN         PIN_A0

/*Slave Write PIN*/
#define S_W_PIN         PIN_A1

/*Slave Button read PIN*/
#define BUT_UP_PIN         PIN_A4
#define BUT_DOWN_PIN       PIN_A5

/*Slave LED PIN(Indicates that it's order is registered)*/
#define LED_DOWN_PIN       PIN_A7 
#define LED_UP_PIN         PIN_A6 

/*Timer conditions*/
//start number of slaves calculation
#define TIMER_SRART     0
//end number of slaves calculation
#define TIMER_END       1
//the normal operation at every idle long pulse
#define TIMER_NORMAL    2

/*DIP pins*/
//to get slave ID
#define DIP_PIN0         PIN_B0
#define DIP_PIN1         PIN_B1
#define DIP_PIN2         PIN_B2
#define DIP_PIN3         PIN_B3

/*7 Segment Pins of Least digit*/
#define SEG1_PINA PIN_B0
#define SEG1_PINB PIN_B1
#define SEG1_PINC PIN_B2
#define SEG1_PIND PIN_B3

/*7 Segment Pins of Most Digit*/
#define SEG2_PINA PIN_B4
#define SEG2_PINB PIN_B5

/*Arrow Segment Pins*/
#define ARROW_UP PIN_B6
#define ARROW_DOWN PIN_B7

/*the display bits number*/
#define DISPLAY_BITS    4

/*-------------------- Macro Functions Declaration ---------------------*/
/*Get bit*/
#define GET_BIT(Reg, bit) ((Reg >> bit)&1)
/*Set bit*/
#define SET_BIT(Reg, bit) Reg = Reg|(1 << bit)
/*Clear bit*/
#define CLEAR_BIT(Reg, bit) Reg = Reg&(~(1 << bit))

/*----------------------- Data Types Declaration -----------------------*/
typedef unsigned char u8;
typedef signed char s8;
typedef int16 s16;

/*-------------------- Global Variables Definition ---------------------*/
volatile s16 cycleCounter = -1;
volatile u8 slaves = 0;
volatile s16 edge_counter = 0;
volatile u8 slaves_calculaion = TRUE;
volatile u8 overflowAction = TIMER_NORMAL;
volatile u8 ID = 0;
volatile u8 currentFloorNumber = 0;
volatile u8 bitShift = DISPLAY_BITS - 1;
volatile u8 isStop = TRUE;
volatile u8 UP_DOWN = TRUE;
/*-------------------------- APIs Definition ---------------------------*/
void setPullup_b(u8 PULL_UP_ENABLE)
{
    switch(PULL_UP_ENABLE)
    {
        case HIGH:
            CLEAR_BIT(OPTION, RBPU);
            break;
        case LOW:
            SET_BIT(OPTION, RBPU);
            break;
    }
}

void display()
{
    u8 leastDigit = currentFloorNumber/10;
    u8 mostDigit = currentFloorNumber%10;
    output_bit(SEG1_PINA, GET_BIT(leastDigit, 0));
    output_bit(SEG1_PINB, GET_BIT(leastDigit, 1));
    output_bit(SEG1_PINC, GET_BIT(leastDigit, 2));
    output_bit(SEG1_PIND, GET_BIT(leastDigit, 3));
    
    output_bit(SEG2_PINA, GET_BIT(mostDigit, 0));
    output_bit(SEG2_PINB, GET_BIT(mostDigit, 1));
    
    if(isStop)
    {
        output_low(ARROW_DOWN);
        output_low(ARROW_UP);
    }
    else
    {
        if(UP_DOWN == TRUE)
        {
            output_high(ARROW_UP);
            output_low(ARROW_DOWN);
        }
        else
        {
            output_high(ARROW_DOWN);
            output_low(ARROW_UP);
        }
    }
}


/*-------------------------- ISRs Definition ---------------------------*/
#int_TIMER0
void timer0_isr(void)
{
    set_timer0(TIMER_PRELOAD);
    if(overflowAction == TIMER_NORMAL)
    {
        cycleCounter = -1;
        bitShift = DISPLAY_BITS - 1;
        display();
        currentFloorNumber=0;
    }
    else if(overflowAction == TIMER_SRART)
    {
        edge_counter = 0;
        overflowAction = TIMER_END;
    }
    else if(overflowAction == TIMER_END)
    {
        slaves = (edge_counter - DISPLAY_BITS * 2)/8;
        slaves_calculaion = FALSE;
        overflowAction = TIMER_NORMAL;
    }
}
/*-------------------------- Application ---------------------------*/
void main()
{
/*---------------------------- Setup -------------------------------*/
    /*CLK PIN*/
    output_float(S_CLK_PIN);
    /*LED PIN*/
    output_drive(LED_UP_PIN);
    output_low(LED_UP_PIN);
    
    output_drive(LED_DOWN_PIN);
    output_low(LED_DOWN_PIN);
    
    /*Enable DIP at first*/
    output_drive(DIP_ENABLE_PIN);
    output_high(DIP_ENABLE_PIN);
    
    /*Write PIN*/
    output_drive(S_W_PIN);
    output_low(S_W_PIN);
    
    /*Read PIN*/
    output_float(S_R_PIN);

    //Slave ID setting
    set_tris_b(0b00001111);
    setPullup_b(HIGH);
    ID  = input_state(DIP_PIN3) << 3;
    ID |= input_state(DIP_PIN2) << 2;
    ID |= input_state(DIP_PIN1) << 1;
    ID |= input_state(DIP_PIN0) << 0;
    
    u8 check_if_DIP_is_disable = 0;
    output_low(DIP_ENABLE_PIN);
    check_if_DIP_is_disable  = input_state(DIP_PIN3) << 3;
    check_if_DIP_is_disable |= input_state(DIP_PIN2) << 2;
    check_if_DIP_is_disable |= input_state(DIP_PIN1) << 1;
    check_if_DIP_is_disable |= input_state(DIP_PIN0) << 0;
    while(check_if_DIP_is_disable != 15);
    set_tris_b(0x00);
    setPullup_b(LOW);
    
    /*BUTTON PIN*/
    output_float(BUT_UP_PIN);
//    output_high(BUT_UP_PIN);
    
    output_float(BUT_DOWN_PIN);
//    output_high(BUT_DOWN_PIN);
    
    //Timer Setting
    setup_timer_0(T0_INTERNAL|PRESCALER|T0_8_BIT);
    enable_interrupts(GLOBAL);
    enable_interrupts(INT_TIMER0);
    
    u8 currentCLK_Edge;
    u8 prevCLK_Edge ;
    /*waiting*/
    delay_ms(1000);
    delay_ms(1000);
    delay_ms(1000);
    delay_ms(1000);
    delay_ms(1000);
  
    set_timer0(TIMER_PRELOAD);
    overflowAction = TIMER_SRART;
    prevCLK_Edge = input_state(S_CLK_PIN);//input(S_CLK_PIN);
    while(slaves_calculaion)
    {
        currentCLK_Edge = input_state(S_CLK_PIN);//input(S_CLK_PIN);
        if(prevCLK_Edge != currentCLK_Edge)
        {      
            set_timer0(TIMER_PRELOAD);
            edge_counter++;
        }
        prevCLK_Edge = currentCLK_Edge; 
    }

/*--------------------------- MAIN LOOP ----------------------------*/
    while (TRUE)
    {
        currentCLK_Edge = input_state(S_CLK_PIN);//input(S_CLK_PIN);
        if(prevCLK_Edge != currentCLK_Edge)
        {
            set_timer0(TIMER_PRELOAD);
        }
        if (prevCLK_Edge && !currentCLK_Edge)  // Falling edge detection
        {
            cycleCounter++;
            // Slave Write
            if(cycleCounter < (slaves*2))
            {
                if ((u8)(cycleCounter / 2) == ID)
                {
                    /*cycleCounter % 2 = 0 indicates up button
                      cycleCounter % 2 = 1 indicates down button*/
                    if((cycleCounter % 2) == 0)
                    {/*up cycle*/
              
                        if(input_state(BUT_UP_PIN) == LOW) 
                        {
                            output_high(S_W_PIN);
                            //output_high(BUT_UP_PIN);
                        }
                    }
                    else
                    {/*down cycle*/
                        if(input_state(BUT_DOWN_PIN) == LOW) 
                        {
                            output_high(S_W_PIN);
                            //output_high(BUT_DOWN_PIN);
                        }
                    }
                }
            }
            else
            {
                output_float(S_R_PIN);
            }
        }
        else if(!prevCLK_Edge && currentCLK_Edge)   // Rising edge detection
        {
            // Read Write
            if((cycleCounter >= (slaves * 2)) && (cycleCounter < (slaves*4)))
            {
                if ((u8)(cycleCounter / 2 - slaves) == ID)
                {
                    /*cycleCounter % 2 = 0 indicates up LED
                      cycleCounter % 2 = 1 indicates down LED*/
                    if(cycleCounter % 2 == 0)
                    {/*up cycle*/
                        output_bit(LED_UP_PIN, !(input_state(S_R_PIN)));
                        //each up cycle followed by down cycle
                        //charge the cap for the down read cycle
                        output_drive(S_R_PIN);
                        output_high(S_R_PIN);
                    }
                    else
                    {/*down cycle*/
                        output_bit(LED_DOWN_PIN, !(input_state(S_R_PIN)));
                    }
                }
            }
            else if(cycleCounter >= slaves*4)
            {
               currentFloorNumber |= (input_state(S_R_PIN)  << bitShift);
               bitShift--;
            }
            else
            {
                output_low(S_W_PIN);
            }
            
            /* Recharging the capacitor before its turn and before 
             * the master sends the floor number. */
            // OR charge before my turn while the last cycle was down
            if((cycleCounter >= (4 * slaves - 1)) || ((u8)(cycleCounter/2 + 1 - slaves)  == ID))
            {
                output_drive(S_R_PIN);
                output_high(S_R_PIN);
            }
        }
        prevCLK_Edge = currentCLK_Edge;
    }
}