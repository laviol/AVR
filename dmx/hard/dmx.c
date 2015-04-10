/* dmx.c
 *
 * DMX-to-4_PWM controller
 *
 * TODO:
 *  Manage fans (with speed and temp?)
 *  DMX address in EEPROM?
 *  Use log curve
 *  Dynamically adjust PWM frequency to allow lower light levels (led drivers limitation)
 */

// Includes
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// Defines
#define PWM_NB_PORTS 4  // Number of PWM

#define PORT_PWM1 PB2   // PWM1 port
#define PORT_PWM2 PD5   // PWM2 port
#define PORT_PWM3 PB3   // PWM3 port
#define PORT_PWM4 PB4   // PWM4 port

#define PWM12_DIVIDER_1    (0 << CS02) | (0 << CS01) | (1 << CS00)  // PWM1 and PWM2 frequency divider :1
#define PWM12_DIVIDER_8    (0 << CS02) | (1 << CS01) | (0 << CS00)  // PWM1 and PWM2 frequency divider :8
#define PWM12_DIVIDER_64   (0 << CS02) | (1 << CS01) | (1 << CS00)  // PWM1 and PWM2 frequency divider :64
#define PWM12_DIVIDER_256  (1 << CS02) | (0 << CS01) | (0 << CS00)  // PWM1 and PWM2 frequency divider :256
#define PWM12_DIVIDER_1024 (1 << CS02) | (0 << CS01) | (1 << CS00)  // PWM1 and PWM2 frequency divider :1024

#define PWM34_DIVIDER_1    (0 << CS12) | (0 << CS11) | (1 << CS10)  // PWM3 and PWM4 frequency divider :1
#define PWM34_DIVIDER_8    (0 << CS12) | (1 << CS11) | (0 << CS10)  // PWM3 and PWM4 frequency divider :8
#define PWM34_DIVIDER_64   (0 << CS12) | (1 << CS11) | (1 << CS10)  // PWM3 and PWM4 frequency divider :64
#define PWM34_DIVIDER_256  (1 << CS12) | (0 << CS11) | (0 << CS10)  // PWM3 and PWM4 frequency divider :256
#define PWM34_DIVIDER_1024 (1 << CS12) | (0 << CS11) | (1 << CS10)  // PWM3 and PWM4 frequency divider :1024

#define PWM12_MODE_FAST_PWM_A (1 << WGM01) | (1 << WGM00)           // Fast PWM (mode 3)
#define PWM12_MODE_FAST_PWM_B (0 << WGM02)                          // Fast PWM (mode 3)

#define PWM34_MODE_FAST_PWM_8BITS_A  (0 << WGM11) | (1 << WGM10)    // Fast PWM - 8bits (mode 5)
#define PWM34_MODE_FAST_PWM_8BITS_B  (0 << WGM13) | (1 << WGM12)    // Fast PWM - 8bits (mode 5)
#define PWM34_MODE_FAST_PWM_16BITS_A (1 << WGM11) | (0 << WGM10)    // Fast PWM - 16bits (mode 14)
#define PWM34_MODE_FAST_PWM_16BITS_B (1 << WGM13) | (1 << WGM12)    // Fast PWM - 16bits (mode 14)

#define PWM1_OFF    (0 << COM0A1) | (0 << COM0A0)   // OC0A off
#define PWM1_NORMAL (1 << COM0A1) | (0 << COM0A0)   // Clear OC0A on Compare Match, Set OC0A at TOP
#define PWM1_INVERT (1 << COM0A1) | (1 << COM0A0)   // Set OC0A on Compare Match, Clear OC0A at TOP

#define PWM2_OFF    (0 << COM0B1) | (0 << COM0B0)   // OC0B off
#define PWM2_NORMAL (1 << COM0B1) | (0 << COM0B0)   // Clear OC0B on Compare Match, Set OC0B at TOP
#define PWM2_INVERT (1 << COM0B1) | (1 << COM0B0)   // Set OC0B on Compare Match, Clear OC0B at TOP

#define PWM3_OFF    (0 << COM1A1) | (0 << COM1A0)   // OC1A off
#define PWM3_NORMAL (1 << COM1A1) | (0 << COM1A0)   // Clear OC1A on Compare Match, Set OC1A at TOP
#define PWM3_INVERT (1 << COM1A1) | (1 << COM1A0)   // Set OC1A on Compare Match, Clear OC1A at TOP

#define PWM4_OFF    (0 << COM1B1) | (0 << COM1B0)   // OC1B off
#define PWM4_NORMAL (1 << COM1B1) | (0 << COM1B0)   // Clear OC1B on Compare Match, Set OC1B at TOP
#define PWM4_INVERT (1 << COM1B1) | (1 << COM1B0)   // Set OC1B on Compare Match, Clear OC1B at TOP

#define DMX_BAUD 250000                     // DMX baudrate (250kbps)
#define MYUBRR (F_CPU / 16 / DMX_BAUD - 1)  // F_CPU is defined in the Makefile

#define USE_LOG_TABLE

// Consts
enum {IDLE, BREAK, STARTB, STARTADR};   // DMX available states

// Generated with a log
uint8_t TABLE_8[256] PROGMEM = {  0,   0,   0,   0,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   3,   4,
                                  4,   4,   5,   5,   5,   5,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,
                                  9,   9,  10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  14,
                                 15,  15,  16,  16,  16,  17,  17,  18,  18,  19,  19,  19,  20,  20,  21,  21,
                                 22,  22,  23,  23,  24,  24,  24,  25,  25,  26,  26,  27,  27,  28,  28,  29,
                                 30,  30,  31,  31,  32,  32,  33,  33,  34,  34,  35,  36,  36,  37,  37,  38,
                                 39,  39,  40,  40,  41,  42,  42,  43,  44,  44,  45,  46,  46,  47,  48,  48,
                                 49,  50,  50,  51,  52,  53,  53,  54,  55,  56,  56,  57,  58,  59,  60,  60,
                                 61,  62,  63,  64,  64,  65,  66,  67,  68,  69,  70,  71,  71,  72,  73,  74,
                                 75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
                                 91,  92,  94,  95,  96,  97,  98,  99, 100, 101, 103, 104, 105, 106, 108, 109,
                                110, 111, 113, 114, 115, 116, 118, 119, 120, 122, 123, 124, 126, 127, 129, 130,
                                132, 133, 135, 136, 137, 139, 141, 142, 144, 145, 147, 148, 150, 152, 153, 155,
                                157, 158, 160, 162, 163, 165, 167, 169, 170, 172, 174, 176, 178, 180, 181, 183,
                                185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 206, 208, 210, 212, 214, 216,
                                219, 221, 223, 225, 228, 230, 232, 235, 237, 240, 242, 244, 247, 249, 252, 255
                               };
// [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 28, 28, 29, 30, 31, 32, 33, 34, 35, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 47, 48, 49, 51, 52, 54, 55, 57, 58, 60, 61, 63, 65, 67, 69, 71, 72, 74, 77, 79, 81, 83, 85, 88, 90, 93, 95, 98, 101, 103, 106, 109, 112, 115, 119, 122, 125, 129, 132, 136, 140, 144, 148, 152, 156, 160, 165, 169, 174, 179, 183, 189, 194, 199, 205, 210, 216, 222, 228, 235, 241, 248, 255]

// Generated with follwing equation: int(65.535 * 10 ** ((i - 1) / (253. / 3.))) from 0 to 254, and adding 0 at first to switch off
uint16_t TABLE_16[256] PROGMEM = {    0,    63,    65,    67,    69,    71,    73,    75,    77,    79,    81,    83,    86,    88,    90,    93,
                                     96,    98,   101,   104,   107,   110,   113,   116,   119,   122,   126,   129,   133,   136,   140,   144,
                                    148,   152,   157,   161,   165,   170,   175,   179,   184,   190,   195,   200,   206,   212,   217,   223,
                                    230,   236,   243,   249,   256,   263,   271,   278,   286,   294,   302,   310,   319,   328,   337,   346,
                                    356,   366,   376,   386,   397,   408,   419,   431,   443,   455,   467,   480,   494,   507,   521,   536,
                                    551,   566,   582,   598,   614,   631,   649,   667,   685,   704,   724,   744,   765,   786,   807,   830,
                                    853,   876,   901,   926,   951,   978,  1005,  1033,  1061,  1090,  1121,  1152,  1184,  1216,  1250,  1285,
                                   1320,  1357,  1394,  1433,  1473,  1513,  1555,  1598,  1643,  1688,  1735,  1783,  1832,  1883,  1935,  1989,
                                   2044,  2100,  2159,  2218,  2280,  2343,  2408,  2474,  2543,  2613,  2686,  2760,  2836,  2915,  2996,  3079,
                                   3164,  3251,  3341,  3434,  3529,  3627,  3727,  3830,  3936,  4045,  4157,  4272,  4390,  4512,  4637,  4765,
                                   4897,  5033,  5172,  5315,  5462,  5614,  5769,  5929,  6093,  6261,  6435,  6613,  6796,  6984,  7177,  7376,
                                   7580,  7790,  8006,  8227,  8455,  8689,  8930,  9177,  9431,  9692,  9960, 10236, 10519, 10810, 11110, 11417,
                                  11733, 12058, 12392, 12735, 13087, 13450, 13822, 14205, 14598, 15002, 15417, 15844, 16282, 16733, 17196, 17672,
                                  18161, 18664, 19181, 19712, 20257, 20818, 21394, 21986, 22595, 23220, 23863, 24524, 25203, 25900, 26617, 27354,
                                  28111, 28889, 29689, 30511, 31355, 32223, 33115, 34032, 34974, 35942, 36936, 37959, 39010, 40089, 41199, 42339,
                                  43511, 44716, 45953, 47225, 48533, 49876, 51257, 52675, 54133, 55632, 57172, 58754, 60380, 62052, 63769, 65535,
                                 };

// Vars
volatile uint8_t gDmxValue[PWM_NB_PORTS];   // Array of DMX vals (raw)
volatile uint16_t gDmxAddress;              // Start address


// IO init
void initIO(void)
{
    DDRB = (1 << PORT_PWM1) |   // Make OC0A an output
           (1 << PORT_PWM3) |   // Make OC1A an output
           (1 << PORT_PWM4);    // Make OC1B an output

    PORTB =  (1 << 0) |         // Enable pullup resistors on bits 0 and 1
             (1 << 1);

    DDRD = (1 << PORT_PWM2);    // Make OC0B an output, other bits as input (including RX)

    PORTD = (1 << 1) |          // Enable pullup resistors on all bits but OC0B and RX
            (1 << 2) |
            (1 << 3) |
            (1 << 4) |
            (1 << 6);

    ACSR = (1 << ACD);                      // Disable analog comparator
    DIDR = (1 << AIN0D) | (1 << AIN1D);     // Disable digital input buffer on AIN1/0
}


// Timers init
void initTimers(void)
{
    // PWM1 + PWM2 settings
    TCCR0A = PWM1_INVERT |
             PWM2_INVERT |
             PWM12_MODE_FAST_PWM_A;

    TCCR0B = PWM12_MODE_FAST_PWM_B |
             PWM12_DIVIDER_64;

    // PWM3 + PWM4 setting
    TCCR1A = PWM3_INVERT | 
             PWM4_INVERT |
             PWM34_MODE_FAST_PWM_16BITS_A;

    TCCR1B = PWM34_MODE_FAST_PWM_16BITS_B |
             PWM34_DIVIDER_64;

    ICR1 = 0xffff;  // TOP value for PWM34 (mode 14)
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


// Get DMX base address from jumpers
void readDmxAddress(void)
{
    gDmxAddress = (((((PIND && 0x1e) | ((PIND && 0x40) >> 1) | ((PINB && 0x03) << 6)) >> 1) ^ 0x7f) << 2) + 1

    PORTB = 0x00;   // Disable pullup resistors
    PORTD = 0x00;   // Disable pullup resistors
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
        if (dmxCount >= PWM_NB_PORTS) {     // All channels received?
            dmxState = IDLE;
        }
    }
}

int main(void)
{

    // Inits
    initIO();
    initTimers();
    initUSART();

    // Read DMX start address, set by jumpers
    readDmxAddress();

    // Enable interrupts
    sei();

    // Main loop
    while (1) {
#ifdef USE_LOG_TABLE
        OCR0A = pgm_read_byte_near(&(TABLE_8[gDmxValue[0]]));
        OCR0B = pgm_read_byte_near(&(TABLE_8[gDmxValue[1]]));
        OCR1A = pgm_read_byte_near(&(TABLE_16[gDmxValue[2]]));
        OCR1B = pgm_read_byte_near(&(TABLE_16[gDmxValue[3]]));
#else
        OCR0A = gDmxValue[0];
        OCR0B = gDmxValue[1];
        OCR1A = gDmxValue[2] * 255;
        OCR1B = gDmxValue[3] * 255;
#endif
    }

    return 1;
}
