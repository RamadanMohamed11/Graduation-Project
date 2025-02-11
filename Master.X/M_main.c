/* 
 * Master Node
 * File:   M_main.c
 * Author: Hesham Ahmed
 * Edit date: 2, 2, 2024
 * Version:05
 */

/*------------------------------ Includes ------------------------------*/
#include <16F628A.h>
#fuses INTRC_IO, NOWDT, NOPROTECT, NOMCLR ,NOLVP
#use delay(CLOCK=4000000)

#define OPTION *((volatile unsigned char*)0x81)
#define RBPU    7
/*------------------------- Macros Declaration -------------------------*/

/*---^^ Master Configurations ^^---*/
//Timer setting
#define TIMER_PRELOAD   206
#define PRESCALER   T0_DIV_1 //1

//Idle pulse width = LONG_PULSE + 2
#define LONG_PULSE  49

//Number of floors
#define SLAVES          16

//the display bits number
#define DISPLAY_BITS 4

//End of the Frame in half cycle
#define MASTER_SLAVE_FRAME_END  (4*SLAVES*2+LONG_PULSE)
#define DISPLAY_FRAME_END       (4*SLAVES*2 + LONG_PULSE + DISPLAY_BITS * 2)

/*---^^^^^^^^^^^^^^^^^^^^^^^^^^^^---*/

/*logical states*/
#define LOW     0
#define HIGH    1

#define TRUE        1
#define FALSE       0

/**/
#define UP_REQUEST      0
#define DOWN_REQUEST    1


/*Master clock input PIN*/
#define M_CLK_PIN PIN_A0

/*Master Read PIN*/
#define M_R_PIN   PIN_A1

/*Master Write PIN*/
#define M_W_PIN   PIN_A2 

#define PORTA_DIRECTION 0X00

#define UP_PIN      PIN_B0
#define STOP_PIN    PIN_B1
#define DOWN_PIN    PIN_B2
#define TEST_LED    PIN_B3
#define PORTB_PULLUP    0X07
#define PORTB_DIRECTION 0X07


/*-------------------- Macro Functions Declaration ---------------------*/
/*Get bit*/
#define GET_BIT(Reg, bit) ((Reg >> bit)&1)
/*Clear bit*/
#define CLEAR_BIT(Reg, bit) Reg = Reg&(~(1 << bit))

/*----------------------- Data Types Declaration -----------------------*/
typedef unsigned char u8;
typedef signed char s8;
typedef int16 s16;

/*-------------------- Global Variables Definition ---------------------*/
//master slave data registering
volatile u8 order_from_slave[SLAVES][2];

//current floor number
u8 floorNumber  = 0;
u8 preFloor = floorNumber;
//Timer overflow counter
volatile s16 overflowCount = 0;

//Cycle counter
volatile u8 cycleNumber = -1;

//shift count
volatile u8 shiftCount = DISPLAY_BITS - 1;

//End of the frame in half cycle
volatile s16 masterSlaveFrameEnd = MASTER_SLAVE_FRAME_END;
volatile s16 displayFrameEnd = DISPLAY_FRAME_END;

//
u8 stopFlag = TRUE;
/*-------------------------- APIs Definition ---------------------------*/
void setPullup_b()
{
    CLEAR_BIT(OPTION, RBPU);
}

/*-------------------------- ISRs Definition ---------------------------*/
#int_TIMER0
void timer0_isr(void)
{
    set_timer0(TIMER_PRELOAD);
    if( (overflowCount >= LONG_PULSE) && (overflowCount < masterSlaveFrameEnd) )
    { 
        output_toggle(M_CLK_PIN);
        if((overflowCount%2==1)) // Falling Edge
        {
            //it was high after the toggle will be low
            cycleNumber++;
            /*Write*/
            if(cycleNumber >= (SLAVES*2))
            {
                // Master Write
                if(order_from_slave[((u8)(cycleNumber/2)-SLAVES)][(cycleNumber%2)]==LOW)
                {
                    //turn Write transistor on
                    output_high(M_W_PIN); 
                }
            }
                //making read pin input
                output_float(M_R_PIN);
        }
        else if((overflowCount%2==0)) // Rising Edge
        {
            //it was low after the toggle will be high
            /*Read*/
            if(cycleNumber < (SLAVES*2))
            {
                if(input(M_R_PIN) == LOW)
                {  
                    if(((cycleNumber/2) != floorNumber)||(!stopFlag))
                    {
                        order_from_slave[(cycleNumber/2)][(cycleNumber%2)]=LOW;   
                    }
                }
                //order_from_slave[cycleNumber]=input(M_R_PIN);
                //Recharge the read capacitor
                output_drive(M_R_PIN);
                output_high(M_R_PIN);
            }
            else
            {
                //turn Write transistor off
                output_low(M_W_PIN);
            }
            
        }
    }
    else if((overflowCount >= masterSlaveFrameEnd) && (overflowCount < displayFrameEnd))
    {
        output_toggle(M_CLK_PIN);
        if(overflowCount%2==1) // Falling Edge
        {
            if(GET_BIT(floorNumber, shiftCount) == LOW)
            {
                output_high(M_W_PIN);
            }
            shiftCount--;
        }
        else if(overflowCount%2==0) // Rising Edge
        {
             output_low(M_W_PIN);
        }
    }
    else if(overflowCount == displayFrameEnd)
    {
        output_high(M_CLK_PIN);
        
        overflowCount = -1;
        cycleNumber = -1;
        
        output_drive(M_R_PIN);
        output_high(M_R_PIN);
        
        shiftCount = DISPLAY_BITS - 1;
    }
    overflowCount++;
}

/*-------------------------- Application ---------------------------*/
void main(void)
{
/*---------------------------- Setup -------------------------------*/
    // Initialize the order_from_slave with no orders
    for(u8 i=0;i<SLAVES;i++)
    {
        order_from_slave[i][UP_REQUEST]=HIGH;
        order_from_slave[i][DOWN_REQUEST]=HIGH;
    }

    set_tris_a(PORTA_DIRECTION);
    /*master read pin*/
    output_high(M_R_PIN);
    /*master clock pin*/
    output_high(M_CLK_PIN);
    /*master write pin*/
    output_low(M_W_PIN);
    
    set_tris_b(PORTB_DIRECTION);
    setPullup_b();
    output_low(TEST_LED);
    if(get_tris_b()==0)
    {
        output_high(TEST_LED);
    }
    /*charging slaves caps*/
    delay_ms(1000);
    
    setup_timer_0(T0_INTERNAL|PRESCALER|T0_8_BIT);
    enable_interrupts(GLOBAL);
    enable_interrupts(INT_TIMER0);
    set_timer0(TIMER_PRELOAD);
    
    

/*--------------------------- MAIN LOOP ----------------------------*/
    while(TRUE)
    {
         if(input_state(UP_PIN)==LOW)
        {
            while(input_state(UP_PIN)==LOW);
            //for bouncing
            delay_ms(50);
            stopFlag = FALSE;
            preFloor = floorNumber;
            
            if(floorNumber < (SLAVES - 1))
            {
                floorNumber++;
            }
        }
        else if(input_state(DOWN_PIN)==LOW)
        {
            while(input_state(DOWN_PIN)==LOW);
            //for bouncing
            delay_ms(50);
            stopFlag = FALSE;
            preFloor = floorNumber;
            if(floorNumber > 0)
            {
                floorNumber--;
            }
        }
        else if(input_state(STOP_PIN)==LOW)
        {
            while(input_state(STOP_PIN)==LOW);
            //for bouncing
            delay_ms(50);
            stopFlag = TRUE;
            if(preFloor < floorNumber)  
            {
                /*up direction*/
               order_from_slave[floorNumber][UP_REQUEST]=HIGH; 
            }
            else if(preFloor > floorNumber)
            {
                /*down direction*/
                order_from_slave[floorNumber][DOWN_REQUEST]=HIGH;
            }
            else
            {
                /*last or first floor */
                order_from_slave[floorNumber][UP_REQUEST]=HIGH;
                order_from_slave[floorNumber][DOWN_REQUEST]=HIGH;
            }
        }
    }
}
