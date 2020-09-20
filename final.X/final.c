/*
 * File:   final.c
 * Author: s08gl
 *
 * Created on 2019年12月16日, 下午 11:17
 */


#include <xc.h>

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config PBADEN = OFF
#pragma config LVP = OFF
#define _XTAL_FREQ 1000000
#define true 1
#define false 0

typedef unsigned char bool;

unsigned int timer;
unsigned int ready_timer;
unsigned char score;
bool is_playing, is_ended, is_showing_time;
bool rd1_flag, rd2_flag;

void __interrupt(high_priority) Hi_ISR(void)
{ // 8 times per sec
    PIR1bits.TMR2IF = 0;
    if(ready_timer > 0) { // start game
        --ready_timer;
        return;
    }
    if(--timer <= 0) {
        if(is_playing) { // end game 
            is_playing = false;
            is_ended = true;
        } else { // toggle time showing
            is_showing_time = !is_showing_time;
        }
        timer = 4;
    }
}

void timer2_init() {
    //timer2
    RCONbits.IPEN = 1;
    INTCONbits.GIE = 1;
    IPR1bits.TMR2IP = 1;
    PIE1bits.TMR2IE = 1;
    PIR1bits.TMR2IF = 0;
    //OSC: 1M Hz
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 0;
    OSCCONbits.IRCF0 = 0;
    // 1/8 sec
    PR2 = 122;
    //scaler: 16 * 16, timer2: off
    T2CON = 0b01111010;
}

void start_game() {
    is_playing = true;
    is_ended = false;
    is_showing_time = true;
    rd1_flag = true;
    score = 0;
    ready_timer = 66;
    timer = 60 * 8; //unit: 1/8 sec
    TMR2 = 0; //clear timer2
    T2CONbits.TMR2ON = 1; //timer2: on
}

void signal_init() {
    TRISBbits.RB0 = 0; //RB0: ball in signal
    LATB = 0;
}

void LED_init() {
    rd2_flag = true;
    TRISC = 0b00000000; //RC: LED number
    TRISD = 0b00000110; //RD4~7: LED digit, RD1: ball in, RD2: game start
    is_playing = false;
    is_ended = false;
    is_showing_time = true;
    LATC = LATD = 0;
}

void main(void) {
    unsigned char score_digit = 0;
    unsigned char latc_temp, latd_temp, glowing_digit;
    LED_init();
    signal_init();
    timer2_init();
    while(1) {
        
        if(PORTDbits.RD2 == 0) { //start game
            if(!rd2_flag) {
                rd2_flag = true;
                start_game();
            }
        } else {
            rd2_flag = false;
        }
        
        if(is_playing) {
            if(PORTDbits.RD1 == 0) { // ball in
                if(!rd1_flag) {
                    rd1_flag = true;
                    if(ready_timer == 0) {
                        LATBbits.LATB0 = 1;
                        if(score < 99) ++score;
                    }
                }
            } else {
                rd1_flag = false;
                LATBbits.LATB0 = 0;
            }
        }
    
        //get score digit
        if(++glowing_digit >= 4) glowing_digit = 0;
        switch(glowing_digit){
            case 0: score_digit = score % 10; break;
            case 1: score_digit = score / 10 % 10; break;
            case 2: score_digit = is_playing ? (timer + 7) / 8 % 10 : 0; break;
            case 3: score_digit = is_playing ? (timer + 7) / 80 % 10 : 0; break;
        }

        //show score digit
        switch(score_digit){
            case 0: latc_temp = 0b01110111; break;
            case 1: latc_temp = 0b00000011; break;
            case 2: latc_temp = 0b01011101; break;
            case 3: latc_temp = 0b00011111; break;
            case 4: latc_temp = 0b00101011; break;
            case 5: latc_temp = 0b00111110; break;
            case 6: latc_temp = 0b01111110; break;
            case 7: latc_temp = 0b00000111; break;
            case 8: latc_temp = 0b01111111; break;
            case 9: latc_temp = 0b00111111; break;
        }

        //digit setting
        switch(glowing_digit){
            case 0: latd_temp = 0b00010000; break;
            case 1: latd_temp = 0b00100000; break;
            case 2:
                latd_temp = is_showing_time ? 0b01000000 : 0;
                latc_temp |= 0b10000000; //floating point
                break;
            case 3:
                latd_temp = is_showing_time ? 0b10000000 : 0;
                break;
        }

        //toggle
        latc_temp ^= 0b11111111;
        //output
        LATC = latc_temp;
        LATD = latd_temp;
    }
    return;
}
