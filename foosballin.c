/************************************************************
FOOSBALLIN'
By: Nicholas McGill

Code to make a binary score counter using an M2 microcontroller, 8 LEDs, 2 photodiodes, and 2 lasers.
With 4 LEDs per side, you can count up to 2^4 = 16 points.
************************************************************/

// INCLUDES
#include "m_general.h"
#include "m_usb.h"

// DEFINES
#define DEBUG 1
#define NUM_LEDS 4
#define CELEBRATE_TIMES 4
#define LIGHT_SHOW_DELAY1_MS 5
#define LIGHT_SHOW_DELAY2_MS 10
#define LIGHT_SHOW_DELAY3_MS 5
#define RESET_LED_MS 100
#define GOAL_THRESHOLD 400
#define NO_DOUBLE_COUNTING 50
#define END_GAME_WAIT 20

// GLOBAL VARIABLES
int score1 = 0;
int score2 = 0;
int maxScore = 10;
int resetButton = 1;

int f0val;	// ADC F0 value [LSBs]
int f1val;	// ADC F1 value [LSBs]

int counterS1 = 0;	// Variable used to set the LED #1 for the score displays
int counterS2 = 0;	// Variable used to set the LED #1 for the score displays

char rx_buffer; //computer interactions for debug

// SUBROUTINES
void init_LEDs(void);
void init_buttons(void);
void LED_show(void);
void player1_celebration(void);
void player2_celebration(void);
void reset_score(void);
void update_score(void);
void set_ADC(void);			// Initialize F0 and F1 as ADC ports
void update_ADC(void);		// Updates ADC values saved in f0val and f1val
void LED_S1_update(void);	// Update P1's LED score
void LED_S2_update(void);	// Update P2's LED score
void debug(void);



//_______________________________________ MAIN
int main(void){

	if (DEBUG){
		m_usb_init(); // connect usb
		while(!m_usb_isconnected()){};  //wait for connection
	}

	init_LEDs();
	init_buttons();
	LED_show();

	while(1){	// Play foosball forever...

		// Interrupts will drive the ADC-enabled scoring system, incrementing the score.
		// While no one has scored enough points to win, and no one's hit the reset button...
		while( (score1 < maxScore) && (score2 < maxScore) && resetButton ){
			resetButton = check(DDRC,6);
			update_ADC();
			update_score();
			if (DEBUG) debug();
		}

		// Evaluate who was the victor!
		if(score1 >=  maxScore){
			player1_celebration();
		}
		else if(score2 >=  maxScore){
			player2_celebration();
		}
		m_wait(END_GAME_WAIT);
		reset_score();		// After celebrating, reset the score and get ready to play again!

	} // END OF WHILE(1)
}

//_______________________________________ Set the correct ports to output for binary LED score
void init_LEDs(void){
	// Player 1: All LEDs set to output and LOW
	set(DDRB, 0);
	set(DDRB, 1);
	set(DDRB, 2);
	set(DDRB, 3);
	clear(PORTB, 0);
	clear(PORTB, 1);
	clear(PORTB, 2);
	clear(PORTB, 3);

	// Player 2: All LEDs set to output and LOW
	set(DDRD, 0);
	set(DDRD, 1);
	set(DDRD, 2);
	set(DDRD, 3);
	clear(PORTD, 0);
	clear(PORTD, 1);
	clear(PORTD, 2);
	clear(PORTD, 3);
}

//_______________________________________ Set up buttons (one for now, RESET)
void init_buttons(void){
	set(DDRC, 6);	// Set C6 to input
	set(PORTC, 6);	// Enable internal pull-up resistor -- button is wired to ground.
}

//_______________________________________ LED show
void LED_show(void){
	player1_celebration();
	m_wait(LIGHT_SHOW_DELAY2_MS);
	player2_celebration();
}

//_______________________________________ Player 1's dancing LEDS
void player1_celebration(void){
	// Do a fancy little light show for the audience
	int count = 0;
	while(count < CELEBRATE_TIMES){
		for(int i=0; i<NUM_LEDS; i++){
			set(PORTB, i);
			m_wait(LIGHT_SHOW_DELAY1_MS);	// m_wait takes in milliseconds
		}
		m_wait(LIGHT_SHOW_DELAY2_MS);
		for(int i=0; i<NUM_LEDS; i++){
			clear(PORTB, i);
			m_wait(LIGHT_SHOW_DELAY3_MS);
		}
		count++;
	}
}

//_______________________________________ Player 2's dancing LEDS
void player2_celebration(void){
	// Do a fancy little light show for the audience
	int count = 0;
	while(count < CELEBRATE_TIMES){
		for(int i=0; i<NUM_LEDS; i++){
			set(PORTD, i);
			m_wait(LIGHT_SHOW_DELAY1_MS);	// m_wait takes in milliseconds
		}
		m_wait(LIGHT_SHOW_DELAY2_MS);
		for(int i=0; i<NUM_LEDS; i++){
			clear(PORTD, i);
			m_wait(LIGHT_SHOW_DELAY3_MS);
		}
		count++;
	}
}

//_______________________________________ Reset the score LEDs
void reset_score(void){
	for(int i=0; i<NUM_LEDS; i++){
		// Turn on LEDs
		set(PORTB, 0);
		set(PORTB, 1);
		set(PORTB, 2);
		set(PORTB, 3);
		set(PORTD, 0);
		set(PORTD, 1);
		set(PORTD, 2);
		set(PORTD, 3);

		m_wait(RESET_LED_MS);

		// Turn off LEDs
		clear(PORTB, 0);
		clear(PORTB, 1);
		clear(PORTB, 2);
		clear(PORTB, 3);
		clear(PORTD, 0);
		clear(PORTD, 1);
		clear(PORTD, 2);
		clear(PORTD, 3);

		m_wait(RESET_LED_MS);
	}

	score1 = 0;
	score2 = 0;
	counterS1 = 0;
	counterS2 = 0;
}

//_______________________________________ Reset the score LEDs
void update_score(void){
	if(f0val > GOAL_THRESHOLD){
		score1++;
		counterS1++;
		f0val = 0;
		LED_S1_update();
		m_wait(NO_DOUBLE_COUNTING);
	}
	else if(f1val > GOAL_THRESHOLD){
		score2++;
		counterS1++;
		f1val = 0;
		LED_S2_update();
		m_wait(NO_DOUBLE_COUNTING);
	}
}

//_______________________________________ Update S1 LEDs
void LED_S1_update(void){	// Make into one function, pass 2 vars?

	// LED #0
	if (score1 % 2 == 0)	clear(PORTB, 0);
	else set(PORTB, 0);

	// LED #1
	if ((score1 < 4) || ((score1 > 7) && (score1 < 12))) clear(PORTB, 1);
	else set(PORTB, 1);

	// LED #2
	if (counterS1 < 2){
		clear(PORTB, 2);
	}
	else if (counterS1 == 2){
		set(PORTB, 2);
	}
	else if (counterS1 > 2){
		set(PORTB, 2);
		counterS1 = 0;
	}

	// LED #3
	if (score1 < 8)	clear(PORTB, 3);
	else set(PORTB, 3);

}

//_______________________________________ Update S2 LEDs
void LED_S2_update(void){

	// LED #0
	if (score2 % 2 == 0)	clear(PORTD, 0);
	else set(PORTD, 0);

	// LED #1
	if ((score2 < 4) || ((score2 > 7) && (score2 < 12)) ) clear(PORTD, 1);
	else set(PORTD, 1);

	// LED #2
	if (counterS2 < 2){
		clear(PORTD, 2);
	}
	else if (counterS2 == 2){
		set(PORTD, 2);
	}
	else if (counterS1 > 2){
		set(PORTD, 2);
		counterS2 = 0;
	}

	// LED #3
	if (score2 < 8)	clear(PORTD, 3);
	else set(PORTD, 3);

}

//_______________________________________ Set up ADCs
void set_ADC(void){
	//****************** set ADC values
	clear(ADMUX, REFS1); // voltage Reference - set to VCC
	set(ADMUX, REFS0);   // ^
	
	//clear(ADMUX, REFS1); // voltage Reference - set to Vref, the Aref pin, 3.4V
	//clear(ADMUX, REFS0); // ^

	set(ADCSRA, ADPS2); // set the ADC clock prescaler, divide 16 MHz by 128 (set, set, set)
	set(ADCSRA, ADPS1); // ^
	set(ADCSRA, ADPS0); // ^
	
	set(DIDR0, ADC0D); // disable the f0 digital input
	set(DIDR0, ADC1D); // disable the f1 digital input
}


//_______________________________________ Update ADCs
void update_ADC(void){ 

	//-------------------> Set pin F0 to read ADC values
	clear(ADCSRB, MUX5); 	// Single-ended channel selection
	clear(ADMUX, MUX2); 	// ^
	clear(ADMUX, MUX1); 	// ^
	clear(ADMUX, MUX0); 	// ^
	
	set(ADCSRA, ADEN); 			// Start conversion process
	set(ADCSRA, ADSC); 			// ^
	while(!check(ADCSRA,ADIF));	// Wait until the ADC value has been converted and read in...

	f0val = ADC;		// Save the ADC value into f0val
	set(ADCSRA, ADIF); 	// Set flag after conversion


	//-------------------> Set pin F1 to read ADC values
	clear(ADCSRB, MUX5); 	// Single-ended channel selection
	clear(ADMUX, MUX2); 	// ^
	clear(ADMUX, MUX1); 	// ^
	set(ADMUX, MUX0); 		// ^
	
	set(ADCSRA, ADEN); 			// Start conversion process
	set(ADCSRA, ADSC); 			// ^
	while(!check(ADCSRA,ADIF));	// Wait until the ADC value has been converted and read in...

	f1val = ADC;		// Save the ADC value into f1val
	set(ADCSRA, ADIF); 	// Set flag after conversion
}

void debug(void){
	while(!m_usb_rx_available());  	//wait for an indication from the computer
	rx_buffer = m_usb_rx_char();  	//grab the computer packet

	m_usb_rx_flush();  				//clear buffer		

	if(rx_buffer == 1) {  			//computer wants ir data

		//write ir data as concatenated hex:  i.e. f0f1f4f5
		m_usb_tx_hex(f0val);
		m_usb_tx_char('\n');  //MATLAB serial command reads 1 line at a time

	}
	if (ADC > 512){
		m_green(ON);
		m_red(OFF);
	}	
	else{
		m_red(ON);
		m_green(OFF);
	}
}