/* temp2freq.c
 *
 * Temperature to frequency
 *
 * Convert a temperature (measured with a thermistor) to a frequency
 * To use between extruder hot-end and EMC².
 *
 */

// Includes
#include <avr/io.h>
#include <math.h>

// Consts
const double m_r0[] = {100000, 100000};  // stated resistance, e.g. 100k
const double m_t0[] = {25, 25};          // temperature at stated resistance, e.g. 25°C
const double m_beta[] = {3974, 3974} ;   // stated beta, e.g. 3500
const double m_r[] = {3850, 3850};       // effective bias impedance, e.g. 3.9k
const double m_vcc = 5.0;     // supply voltage to potential divider, e.g. 5V
const double m_vref = 5.0;    // ADC reference, e.g. 5V

const uint16_t m_fCpuDivider = 1;  // CPU clock divider for timers
                                   // don't forget to also modify CS[2:0] bits of TCCR1B register
// const uint16_t m_fCpuDivider = 8;  // CPU clock divider for timers
// const uint16_t m_fCpuDivider = 64;  // CPU clock divider for timers
// const uint16_t m_fCpuDivider = 256;  // CPU clock divider for timers
// const uint16_t m_fCpuDivider = 1024;  // CPU clock divider for timers


// IO init
void initIO(void)
{
    DDRA = (1 << PA0) |  // Make OC1A as output
           (1 << PA1);   // Make OC1B as output
}

// Timers init
void initTimers(void)
{
    TCCR1A = (0 << COM1A1) | (0 << COM1A0) |  // Compare mode: Toggle on Compare Match
             (0 << COM1B1) | (0 << COM1B0) |  // Compare mode: Toggle on Compare Match
             (0 << WGM11)  | (0 << WGM10);    // Waveform generation modes: Clear Timer on Compare

    TCCR1B = (0 << ICNC1) | (0 << ICES1) |
             (0 << WGM13) | (1 << WGM12) |              // Waveform generation modes: Clear Timer on Compare
             (0 << CS12)  | (0 << CS11) | (1 << CS10);  // Clock select bit description: ClkIO/1
                                                        // don't forget to also modify m_fCpuDivider
//              (0 << CS12)  | (1 << CS11) | (0 << CS10);  // Clock select bit description: ClkIO/8
//              (0 << CS12)  | (1 << CS11) | (1 << CS10);  // Clock select bit description: ClkIO/64
//              (1 << CS12)  | (0 << CS11) | (0 << CS10);  // Clock select bit description: ClkIO/256
//              (1 << CS12)  | (0 << CS11) | (1 << CS10);  // Clock select bit description: ClkIO/1024

    TCCR1C = 0x00;

    ICR1 = 0x0000;

    TIMSK1 = 0x00;

    TIFR1 = 0x00;
}

// ADC init
void initADC(void)
{
    ADMUX = (0 << REFS1) | (0 << REFS0) |  // Voltage Reference Selections: Vcc used as analog reference
            (1 << MUX5) | (0 << MUX4) | (0 << MUX3) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0);  // Analog channel selection: AGND

    ADCSRA = (1 << ADEN);  // ADC Enable

    ADCSRB = 0x00;

    DIDR0 = (1 << ADC0D) | (1 << ADC1D);  // Digital Input Disable Register: Pin PA0 and PA1 disabled
}

// Read adc
uint16_t readADC(uint8_t channel)
{
    // Select channel
    ADMUX &= (1 << REFS1) | (1 << REFS0);  // clear selection (MUX[5:0])
    ADMUX |= channel;

    // Read adc
    ADCSRA |= (1 << ADSC);         // start conversion
    while (ADCSRA & (1 << ADSC));  // wait for end of conversion

    return ADC;
}

// Convert ADC reading into a temperature in Celcius
double adc2temperature(uint16_t adc, uint8_t channel)
{
    double v, rth, k;

    v = adc * m_vref / 1024;               // convert the 10 bit ADC value to a voltage
    rth = m_r[channel] * v / (m_vcc - v);  // resistance of thermistor
    k = m_r0[channel] * exp(-m_beta[channel] / m_t0[channel]);

    return (m_beta[channel] / log(rth / k)) - 273.15;  // temperature
}

// // Convert a temperature into a ADC value
// uint16_t temperature2adc(double temperature, uint8_t channel)
// {
//     double v, rth;
//
//     rth = m_r0[channel] * exp(m_beta[channel] * (1 / (temperature + 273.15) - 1 / m_t0[channel]));  // resistance of the thermistor
//     v = m_vcc * rth / (m_r[channel] + rth);                                                         // the voltage at the potential divider
//
//     return round(v / m_vref * 1024);
// }

uint16_t frequency2OCR(double frequency)
{
    return F_CPU / (2 * m_fCpuDivider * frequency) - 1;
}

int main(void)
{
    uint16_t adc;
    double temperature;

    // Inits
    initIO();
    initTimers();
    initADC();

    // Main loop
    while (1) {
        adc = readADC(0);  // read ADC on channel 0 (PA0)
        temperature = adc2temperature(adc, 0);
        OCR1A = frequency2OCR(temperature * 10.);

        adc = readADC(1);  // read ADC on channel 1 (PA1)
        temperature = adc2temperature(adc, 1);
        OCR1B = frequency2OCR(temperature * 10.);
    }

    return 1;  // should never reach this point
}
