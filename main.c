/*
 * PhaseShifter.c
 *
 * Created: 3/3/2023 11:11:41 AM
 * Author : Szymon Hrehorowicz
 */ 

#define F_CPU 11059200UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "stdbool.h"

#define NUMBER_OF_FREQUENCIES 20U

volatile uint8_t baseStatus = 0x08; // 0b00001000
volatile uint8_t multiplyerStatus = 0x00;
volatile bool changed = false;

typedef struct frequency{
	uint8_t key;
	uint32_t icr;
	uint8_t prescaler;
}Frequency;

void setFrequencies(Frequency *frequencies);
void setOutputSignals();
void incrementMultiplyer();
void incrementBase();
ISR(PCINT1_vect);



int main(void)
{
	// LEDs setup
	DDRD = 0xFF;		// set all as output

	// Button interrupt setup
	DDRC &= 0b11011111; // set PC5 as an input
	PORTC = 0b00100000; // enable pull-up resistor at PC5
	
	PCICR |= (1 << PCIE1);
	PCMSK1 = (1 << PCINT13);
	
	// Enable interrupts
	sei();
	
	// Create frequencies array and set it up
	Frequency frequencies[NUMBER_OF_FREQUENCIES];
	setFrequencies(frequencies);

	uint8_t currentMask, previousMask;
	currentMask = baseStatus | multiplyerStatus;
	previousMask = currentMask;
	
	// Set up initial frequency as 1Hz
	setOutputSignals(frequencies[0]);
	
    while (1) 
    {
		currentMask = baseStatus | multiplyerStatus;
		PORTD = currentMask;
		
		if(currentMask != previousMask) {
			previousMask = currentMask;
			for(uint8_t i = 0; i < NUMBER_OF_FREQUENCIES; i++) {
				if(currentMask == frequencies[i].key) {
					setOutputSignals(frequencies[i]);
					break;
				}
			}
		}
    }
}

void setFrequencies(Frequency *frequencies) {
	uint32_t icrs[NUMBER_OF_FREQUENCIES] = {21599, 43199, 21599, 14399, 10799, 8639, 34559, 17279, 11519, 8639, 55295, 27647, 13823, 9215, 6911, 5529, 2764, 1381, 921, 690};
	uint8_t prescalers[NUMBER_OF_FREQUENCIES] = {4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	
	
	for(uint8_t i = 0; i < NUMBER_OF_FREQUENCIES; i++){
		static uint8_t multiplyer = 0b00000000;
		static uint8_t base = 0b00001000;
		static uint8_t counter = 0;
		uint8_t mask = base | multiplyer;
		
		frequencies[i].key = mask;
		frequencies[i].icr = icrs[i];
		frequencies[i].prescaler = prescalers[i];
		
		if(counter >= 4) {
			counter = 0;
			base = 0b00001000;
			if(multiplyer == 0) {
				multiplyer = 0x01;
				}else {
				multiplyer <<= 1;
			}
			}else {
			counter++;
			base <<= 1;
		}
	}
}

void setOutputSignals(Frequency newFrequency){
	DDRB |= (1<<PB1);
	DDRB |= (1<<PB2);
	
	TCCR1B = 0x18;
	TCCR1A = 0x50;
	
	ICR1 = newFrequency.icr;
	OCR1A = (int) (ICR1 * 0.02);
	OCR1B = (int) (ICR1 * 0.51);
	TCNT1 = 0x0;
	
	TCCR1A = 0xA0;
	TCCR1C = 0xC0;
	TCCR1A = 0x50;
	
	TCCR1B |= newFrequency.prescaler;
}

void incrementMultiplyer() {
	// 0b00000111
	// 1000 100 10
	if(multiplyerStatus >= 0x04) {
		multiplyerStatus = 0x00;
		}else {
		if(multiplyerStatus == 0x00) {
			multiplyerStatus = 0x01;
			}else {
			multiplyerStatus <<= 1;
		}
	}
}

void incrementBase() {
	// 0b11111000;
	// 8 6 4 2 1
	if(baseStatus >= 0x80) {
		baseStatus = 0x08;
		incrementMultiplyer();
		}else {
		baseStatus <<= 1;
	}
}

ISR(PCINT1_vect) {
	if(!changed){
		changed = true;
		incrementBase();
		}else {
		changed = false;
	}
}