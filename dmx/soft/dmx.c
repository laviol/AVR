/* dmx.c
 *
 * DMX-to-8-PWM decoder
 *
 * TODO:
 *  Manage fans with speed and temp
 *  DMX address in EEPROM?
 */

// Includes
// #include <avr/io.h>      // Done by the Makefile
#include <avr/interrupt.h>
// #include <avr/sleep.h>

// Defines
#define PWM_PORT PORTB
#define PWM_CH0  PB0
#define PWM_CH1  PB1
#define PWM_CH2  PB2
#define PWM_CH3  PB3
#define PWM_CH4  PB4
#define PWM_CH5  PB5
#define PWM_CH6  PB6
#define PWM_CH7  PB7

#define FLAG_CH0 (1 << PWM_CH0)
#define FLAG_CH1 (1 << PWM_CH1)
#define FLAG_CH2 (1 << PWM_CH2)
#define FLAG_CH3 (1 << PWM_CH3)
#define FLAG_CH4 (1 << PWM_CH4)
#define FLAG_CH5 (1 << PWM_CH5)
#define FLAG_CH6 (1 << PWM_CH6)
#define FLAG_CH7 (1 << PWM_CH7)

#define PWM_RATE 100                        // PWM refresh rate (>= 100Hz)
#define PWM_INVERT 0xff                     // Set bit to 1 to invert the output logic

#define SWITCH_PORT PIND

#define DMX_BAUD 250000                     // DMX baudrate (250kbps)
#define MYUBRR (F_CPU / 16 / DMX_BAUD - 1)  // F_CPU is defined in the Makefile

// Consts
enum {IDLE, BREAK, STARTB, STARTADR};       // DMX available states

// Vars
volatile uint8_t gDmxValue[8];              // Array of DMX vals (raw)
volatile uint16_t gDmxAddress;              // Start address


// IO init
void initIO(void)
{
    DDRB = 0xff;                            // PORTB as output
    PORTB = 0x00 ^ PWM_INVERT;              // default outputs

    DDRD = 0x00;                            // PORTD as input
    PORTD = 0xff;                           // Enable pullup resistors

    ACSR = (1 << ACD);                      // Disable analog comparator

    DIDR = (1 << AIN0D) | (1 << AIN1D);     // Disable digital input buffer on AIN1/0
}


// Timer init
void initTimer(void)
{

    // Use CLK/1 prescale value, clear timer/counter on compareA match
    TCCR1B = (1 << CS10) | (1 << WGM12);

    // Preset timer1 high/low byte
    OCR1A = ((F_CPU / PWM_RATE / 256) - 1);

    // Enable Timer/Counter1, Output Compare A Match Interrupt
    TIMSK  = (1 << OCIE1A);
}


// USART init
void initUSART(void)
{

    // USART Control and Status Register B
    UCSRB = (1 << RXEN) | (1 << RXCIE);     // Receiver Enable + RX Complete Interrupt Enable

    // USART Control and Status Register C
    UCSRC = (3 << UCSZ0) | (1 << USBS);     // Character Size (8 bits) + Stop Bit Select (2 bits)

    // Set baud rate
    UBRRH = (unsigned char)(MYUBRR >> 8);
    UBRRL = (unsigned char)(MYUBRR);
}


// Get DMX base address from dip switch
void readDmxAddress(void)
{
    gDmxAddress = (((SWITCH_PORT >> 1) ^ 0x3f) << 3) + 1;

    PORTD = 0x00;   // Disable pullup resistors
}


// Timer interrupt routine
ISR(TIMER1_COMPA_vect)
{
    static uint8_t softCounter = 0xff;                      // Timer tick
    static uint8_t pwmRegister = 0x00;                      // PWM register
    static uint8_t pwmValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Array of PWM vals (for double buffering)

    // Update port outputs
    PWM_PORT = pwmRegister ^ PWM_INVERT;

    // Increment modulo 256 counter and update the
    // PWM values only when counter reach 0
    if (++softCounter == 0) {

        // Update double buffer (verbose for speed)
        pwmValue[0] = gDmxValue[0];
        pwmValue[1] = gDmxValue[1];
        pwmValue[2] = gDmxValue[2];
        pwmValue[3] = gDmxValue[3];
        pwmValue[4] = gDmxValue[4];
        pwmValue[5] = gDmxValue[5];
        pwmValue[6] = gDmxValue[6];
        pwmValue[7] = gDmxValue[7];

        // Reset PWM register
        pwmRegister = 0xff;
    }
    if (softCounter == pwmValue[0]) {
        pwmRegister &= ~FLAG_CH0;
    }
    if (softCounter == pwmValue[1]) {
        pwmRegister &= ~FLAG_CH1;
    }
    if (softCounter == pwmValue[2]) {
        pwmRegister &= ~FLAG_CH2;
    }
    if (softCounter == pwmValue[3]) {
        pwmRegister &= ~FLAG_CH3;
    }
    if (softCounter == pwmValue[4]) {
        pwmRegister &= ~FLAG_CH4;
    }
    if (softCounter == pwmValue[5]) {
        pwmRegister &= ~FLAG_CH5;
    }
    if (softCounter == pwmValue[6]) {
        pwmRegister &= ~FLAG_CH6;
    }
    if (softCounter == pwmValue[7]) {
        pwmRegister &= ~FLAG_CH7;
    }
}


// USART interrupt routine
ISR(USART_RX_vect)
{
    // Static vars
    static uint16_t dmxCount = 0;
    static uint8_t dmxState = IDLE;         // DMX state

    // Dynamic vars
    uint8_t USARTstate = UCSRA;             // Get state before data
    uint8_t dmxByte = UDR;                  // Get data (inverted)

    sei();

    if (USARTstate & (1 << FE)) {           // Check for break
        UCSRA &= ~(1 << FE);                // Reset flag (necessary for simulation in AVR Studio)
        dmxCount = gDmxAddress;             // Reset channel counter (count channels before start address)
        dmxState = BREAK;
    }
    else if (dmxState == BREAK) {
        if (dmxByte == 0) {
            dmxState = STARTB;              // Normal start code detected
        }
        else {
            dmxState= IDLE;
        }
    }
    else if (dmxState == STARTB) {
        if (--dmxCount == 0) {              // Start address reached?
            dmxCount = 1;                   // Set up counter for required channels
            gDmxValue[0] = dmxByte;         // Get 1st DMX channel of device
            dmxState = STARTADR;
        }
    }
    else if (dmxState == STARTADR) {
        gDmxValue[dmxCount++] = dmxByte;    // Get channel
        if (dmxCount >= 8) {                // All channels received?
            dmxState = IDLE;
        }
    }
}

int main(void)
{

    // Inits
    initIO();
    initTimer();
    initUSART();

    // Read DMX start address, set by switches
    readDmxAddress();

    // Enable interrupts
    sei();

    // Main loop
    while (1) {
    }

    return 1;
}
