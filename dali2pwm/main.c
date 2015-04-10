#include <avr/interrupt.h>

#include "main.h"
#include "dali.h"

// TODO: Control current and temperature
//       Manage fan

// This is data table for level conversion (Logarithmic mode)
// Convert [0-254] to [0-65535]
// Generated with follwing equation: int(65.535 * 10 ** ((i - 1) / (253. / 3.)))
const uint16_t DIMMING_CURVE[255] = {    0,
                                        65,    67,    69,    71,    73,    75,    77,    79,    81,    83,
                                        86,    88,    90,    93,    96,    98,   101,   104,   107,   110,
                                       113,   116,   119,   122,   126,   129,   133,   136,   140,   144,
                                       148,   152,   157,   161,   165,   170,   175,   179,   184,   190,
                                       195,   200,   206,   212,   217,   223,   230,   236,   243,   249,
                                       256,   263,   271,   278,   286,   294,   302,   310,   319,   328,
                                       337,   346,   356,   366,   376,   386,   397,   408,   419,   431,
                                       443,   455,   467,   480,   494,   507,   521,   536,   551,   566,
                                       582,   598,   614,   631,   649,   667,   685,   704,   724,   744,
                                       765,   786,   807,   830,   853,   876,   901,   926,   951,   978,
                                      1005,  1033,  1061,  1090,  1121,  1152,  1184,  1216,  1250,  1285,
                                      1320,  1357,  1394,  1433,  1473,  1513,  1555,  1598,  1643,  1688,
                                      1735,  1783,  1832,  1883,  1935,  1989,  2044,  2100,  2159,  2218,
                                      2280,  2343,  2408,  2474,  2543,  2613,  2686,  2760,  2836,  2915,
                                      2996,  3079,  3164,  3251,  3341,  3434,  3529,  3627,  3727,  3830,
                                      3936,  4045,  4157,  4272,  4390,  4512,  4637,  4765,  4897,  5033,
                                      5172,  5315,  5462,  5614,  5769,  5929,  6093,  6261,  6435,  6613,
                                      6796,  6984,  7177,  7376,  7580,  7790,  8006,  8227,  8455,  8689,
                                      8930,  9177,  9431,  9692,  9960, 10236, 10519, 10810, 11110, 11417,
                                     11733, 12058, 12392, 12735, 13087, 13450, 13822, 14205, 14598, 15002,
                                     15417, 15844, 16282, 16733, 17196, 17672, 18161, 18664, 19181, 19712,
                                     20257, 20818, 21394, 21986, 22595, 23220, 23863, 24524, 25203, 25900,
                                     26617, 27354, 28111, 28889, 29689, 30511, 31355, 32223, 33115, 34032,
                                     34974, 35942, 36936, 37959, 39010, 40089, 41199, 42339, 43511, 44716,
                                     45953, 47225, 48533, 49876, 51257, 52675, 54133, 55632, 57172, 58754,
                                     60380, 62052, 63769, 65535
                                    };


// This function allows to initialize all the micrcontroller ports for the application
void initIO(void)
{

    // DDRx  = 1 output,      DDRx  = 0 input
    // PORTx = 1 output high, PORTx = 0 output low
    // PINx is the read value at the input of the microcontroller
    // Setting PINx if the port is configured as an output make it toggled

    // PB7 : ADC4       PIN24 I_LAMP                Led Current Measurement (not yet implemented)
    // PB6 : ADC7       PIN23
    // PB5 : ADC6       PIN22 DALI_ADDRESS_BIT_5    Dali address bit 5 (not yet implemented)
    // PB4 : AMP0+      PIN21 DALI_ADDRESS_BIT_4    Dali address bit 4 (not yet implemented)
    // PB3 : AMP0-      PIN20 DALI_ADDRESS_BIT_3    Dali address bit 3 (not yet implemented)
    // PB2 : ADC5       PIN16 DALI_ADDRESS_BIT_2    Dali address bit 2 (not yet implemented)
    // PB1 : PSCOUT21   PIN09 DALI_ADDRESS_BIT_1    Dali address bit 1 (not yet implemented)
    // PB0 : PSCOUT20   PIN08 DALI_ADDRESS_BIT_0    Dali address bit 0 (not yet implemented)

    DDRB = 0x00;                // Set all pins as input
//     PORTB = (0x3f << PB0);      // Enable pull-up resistors on PB0:5 (for DALI address reading)

    // PD7 : ACMP0      PIN15
    // PD6 : ADC3       PIN14 TEMPERATURE           Led Temperature measurement (not yet implemented)
    // PD5 : ACMP2      PIN13
    // PD4 : DALIRX     PIN12 DALI_RX
    // PD3 : DALITX     PIN05 DALI_TX
    // PD2 : 0C1A       PIN04 LED_PWM
    // PD1 : PD1        PIN03
    // PD0 : PSCOUT00   PIN01

    DDRD = (1 << PD4) |         // Set DALIRX pin as output
           (1 << PD2);          // Set 0C1A as output
    PORTD = 0x00;               // Disable all pull-up resistors
}


void init(void)
{

    // Clock divider
    CLKPR = (1 << CLKPCE);      // Enable the clock divider
    CLKPR = (3 << CLKPS0);      // Clock divider :8 (not needed if fuse CKDIV8 is set, which is the default)

    // Timer1 is used for fast PWM (led dimming)
    // TOP value
    ICR1H = 0xff;
    ICR1L = 0xff;

    // Compare match value (= PWM)
    TCCR1A = PWM_INVERT | PWM_MODE_FAST_PWM_16BITS_A;
    TCCR1B = PWM_MODE_FAST_PWM_16BITS_B | PWM_DIVIDER_1024;     // PWM ~ 977Hz

    // Power reduction mode
    PRR = (1 << PRADC) |    // Stop ADC clock
          (1 << PRSPI) |    // Stop SPI clock
          (7 << PRPSC0);    // Stop PSCn clock
}


int main(void)
{
    uint8_t ledFailure = 0;     // Used to check led problems (not yet implemented)
    uint8_t outputLevel;

    // Disable interrupts
    cli();

    // Inits
    initIO();
    init();
    daliInit();

    // Enable interrupts
    sei();

    // Main loop
    while (1) {
        // TODO: check led failure (current, temperature...)
        outputLevel = daliControlGear(ledFailure);
        OCR1A = DIMMING_CURVE[outputLevel];
    }

    return 1;
}
