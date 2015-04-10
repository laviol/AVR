#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#include "main.h"
#include "dali.h"
#include "daliCmd.h"

// This array contains fade times, in ms
// The fade period will be multiplied by 2 in software for this value.
// Last value (FADE_TIME[15]) is actually 90.510s.
uint16_t FADE_TIME[16] = {    0,   707,  1000,  1414,  2000,  2828,  4000,  5657,
                           8000, 11314, 16000, 22627, 32000, 45255, 64000, 45255
                           };

// This array contains the fade rate, in steps/200ms
uint8_t FADE_RATE[16] = { 0, 72, 51, 36, 25, 18, 13, 9,
                          6,  4,  3,  2,  2,  1,  1, 0
                          };

uint8_t newDaliAddress = 0;
uint8_t newDaliCommand = 0;
volatile uint8_t newRx = 0;
static uint8_t daliRunning = 0;

volatile uint8_t tick1msCounter = 0;    // Incremented every ms.
volatile uint8_t timeout = 0;           // Used to generate a delay between 1 and 255 milliseconds
// TODO: find a better name (generalTimeout)
uint16_t specialModeTimeout = 0;        // Long time delay (1-65535), in 1/4th seconds.
// when this timer is decounting, dali is in "special mode" (for 15 minutes).
// when specialModeTimeout is equal to 0, all special modes shall be terminated

uint16_t fadePeriod;                    // Period between each fade step. = fadeTime/(actualLevel - requestedLevel) or = 1/fadeRate.
uint8_t requestedLevel = 0;             // Counter, decremented every 1ms, used for fading...
uint8_t dimSens = DOWN;                 // Indicates wether the power must increase or decrease.
uint8_t compareMode = 0;
uint8_t physicalSelectionMode = 0;

DaliRegisters dali;


// This interrupt routine is called each time a new dali frame is received
ISR(USART_RX_vect)
{

    // Check if the 2 stop bits value are 1, frame is 16 bits long and no frame error occured
    if ((EUCSRC & (1 << FEM | 1 << F1617 | 3 << STP0)) == (3 << STP0)) {
        newDaliAddress = EUDR;
        newDaliCommand = UDR;
        newRx = 1;
        daliRunning = 1;
    }
    else {

        // If an error occured, the interrupt flag should be cleared by reading UDR.
        // However, doing this will remember that an error occured, and will generate
        // an error for the next valid frame... (hardware problem ?)
        // To avoid this problem, disabling receiver and renabling it clears flag
        // and all error flags!
        DALI_DISABLE_RX();
    }
    DALI_ENABLE_RX();
}

// Dali base timing
ISR(TIMER0_COMP_A_vect)
{
    daliTick();
}


// Configure EUSART
void daliInitEUSART(void)
{
    EUCSRA = (3 << UTxS0) |     // 8 bits Tx size
             (14 << URxS0);     // 16 bits Rx size (Manchester encoding)

    EUCSRB = (1 << EUSART) |    // Enable EUSART
             (1 << EUSBS) |     // 2 stop bits
             (1 << EMCH) |      // Manchester encoding
             (1 << BODR);       // MSB first

    MUBRRH = (uint8_t)(MUBRR >> 8);
    MUBRRL = (uint8_t)(MUBRR & 0x00ff);

    UBRRH = (uint8_t)(UBRR >> 8);
    UBRRL = (uint8_t)(UBRR & 0x00ff);

    UCSRA = 0x00;
    UCSRB = (1 << RXCIE) |      // Rx complete interrupt enabled
            (1 << RXEN) |       // Rx enabled
            (1 << TXEN);        // Tx enabled
    UCSRC = (1 << USBS);        // 2 Tx stop bits

    // Clear all flags
    DALI_DISABLE_RX();
    DALI_ENABLE_RX();
}



// Configure Timer0
void daliInitTimer0(void)
{

    // Clear timer/counter on compareA match
    TCCR0A = (1 << WGM01);

    // Timer0 divider :8
    TCCR0B = TIMER0_DIVIDER_8;

    // Set CTC max value
    OCR0A = (uint8_t)((F_CLKIO / (2 * 8 * F_DALI_TICK)) - 1);

    // Enable Timer/Counter1, Output Compare A Match Interrupt
    TIMSK0 = (1 << OCIE0A);
}


// Initialise DALI registers
// Load eeprom if previous values exist
void daliInit(void)
{
    uint8_t n;

    dali.cmdType = DALI_CMD_TYPE_NONE;
    dali.addressByte = 0;
    dali.commandByte = 0;
    dali.dtr = 0;

    // The following reset values are determined by dali standard
    dali.actualDimLevel     = 0xfe;
    dali.powerOnLevel       = 0xfe;
    dali.systemFailureLevel = 0xfe;
    dali.minLevel           = DALI_PHYSICAL_MIN_LEVEL;
    dali.maxLevel           = 0xfe;
    dali.fadeRate           = 0x07;
    dali.fadeTime           = 0;
//     dali.shortAddress       = 0;
    dali.searchAddressH     = 0xff;
    dali.searchAddressM     = 0xff;
    dali.searchAddressL     = 0xff;
    dali.randomAddressH     = 0xff;
    dali.randomAddressM     = 0xff;
    dali.randomAddressL     = 0xff;
    dali.group              = 0x0000;
    for (n = 0; n < 16; n++) {
        dali.scene[n]         = 0xff;
    }
    dali.status.powerFailure        = 1;    // MSB
    dali.status.missingShortAddress = 1;
    dali.status.resetState          = 1;
    dali.status.fadeRunning         = 0;
    dali.status.limitError          = 0;
    dali.status.lampOn              = 1;
    dali.status.lampFailure         = 0;
    dali.status.ballastFailure      = 0;    // LSB
//     dali.status.statusInformation = 0xe4;   // As described above

    // TODO: add a procedure to reset the eeprom if a switch is on at startup
    if (eeprom_read_byte(ADD_EEPROM_STATUS) == EEPROM_INITIALIZED) {

        // eeprom contains previsously saved values, load
        dali.powerOnLevel = eeprom_read_byte((const uint8_t*)ADD_POWER_ON_LEVEL);
        dali.systemFailureLevel = eeprom_read_byte((const uint8_t*)ADD_SYSTEM_FAILURE_LEVEL);
        dali.minLevel = eeprom_read_byte((const uint8_t*)ADD_MIN_LEVEL);
        dali.maxLevel = eeprom_read_byte((const uint8_t*)ADD_MAX_LEVEL);
        dali.fadeRate = eeprom_read_byte((const uint8_t*)ADD_FADE_RATE);
        dali.fadeTime = eeprom_read_byte((const uint8_t*)ADD_FADE_TIME);
        dali.shortAddress = eeprom_read_byte((const uint8_t*)ADD_SHORT_ADD);
        dali.randomAddressH = eeprom_read_byte((const uint8_t*)ADD_RANDOM_ADDH);
        dali.randomAddressM = eeprom_read_byte((const uint8_t*)ADD_RANDOM_ADDM);
        dali.randomAddressL = eeprom_read_byte((const uint8_t*)ADD_RANDOM_ADDL);
        dali.group |= (eeprom_read_byte((const uint8_t*)ADD_GROUPH)) << 8;
        dali.group |= (eeprom_read_byte((const uint8_t*)ADD_GROUPL));
        for (n = 0; n < 16; n++) {
            dali.scene[n] = eeprom_read_byte((const uint8_t*)(ADD_SCENE_0 + n));
        }

        // If eeprom is loaded, the device is not in reset state any more.
        dali.status.resetState = 0;
    }
    else {

        // eeprom is empty, save
        eeprom_write_byte((uint8_t*)ADD_POWER_ON_LEVEL, dali.powerOnLevel);
        eeprom_write_byte((uint8_t*)ADD_SYSTEM_FAILURE_LEVEL, dali.systemFailureLevel);
        eeprom_write_byte((uint8_t*)ADD_MIN_LEVEL, dali.minLevel);
        eeprom_write_byte((uint8_t*)ADD_MAX_LEVEL, dali.maxLevel);
        eeprom_write_byte((uint8_t*)ADD_FADE_RATE, dali.fadeRate);
        eeprom_write_byte((uint8_t*)ADD_FADE_TIME, dali.fadeTime);
        eeprom_write_byte((uint8_t*)ADD_SHORT_ADD, dali.shortAddress);      // TODO: Read from dip switches?
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDH, dali.randomAddressH);
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDM, dali.randomAddressM);
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDL, dali.randomAddressL);
        eeprom_write_byte((uint8_t*)ADD_GROUPH, ((uint8_t)(dali.group >> 8)));
        eeprom_write_byte((uint8_t*)ADD_GROUPL, ((uint8_t)(dali.group)));
        for (n = 0; n < 16; n++) {
            eeprom_write_byte(((uint8_t*)(ADD_SCENE_0 + n)), dali.scene[n]);
        }
        eeprom_write_byte((uint8_t*)ADD_EEPROM_STATUS, (uint8_t)EEPROM_INITIALIZED);
    }

    // Check if short address exists :
    if (dali.shortAddress == 0xff) {
        dali.status.missingShortAddress = 1;
    }
    else {
        dali.status.missingShortAddress = 0;
    }

    daliRunning = 0;
    dali.actualDimLevel = dali.powerOnLevel;

    daliInitEUSART();
    daliInitTimer0();
}


// This function must be called every 1 ms
// Update counters
void daliTick(void)
{
    tick1msCounter++;       // Increase counter
    if (timeout != 0) {
        timeout--;
    }
    return;
}


// Updates actualDimLevel during fading
void daliStepOutputPower(uint8_t sense)
{
  dali.status.fadeRunning = 1;

    if (sense == UP) {

        // Step up arc power level
        if (dali.actualDimLevel == 0) {
            dali.actualDimLevel = dali.minLevel;
        }
        else {
            if (dali.actualDimLevel < dali.maxLevel) {
                dali.actualDimLevel++;
            }
        }
    }
    else {

        // Step down arc power level
        if (dali.actualDimLevel > dali.minLevel) {
            dali.actualDimLevel--;
        }
        else {
            dali.actualDimLevel = 0;
        }
    }

    // Disable fading if requested level is reached
    if (dali.actualDimLevel == requestedLevel) {
        fadePeriod = 0;
        dali.status.fadeRunning = 0;
    }
}


// Used with 'DALI_CMD_DIRECT ARC POWER' command
void daliChangeOutputWithFadeTime(void)
{
    if (requestedLevel != 0xff) {

        // Limits checking
        dali.status.limitError = 0;
        if (requestedLevel < dali.minLevel && requestedLevel != 0) {
            requestedLevel = dali.minLevel;
            dali.status.limitError = 1;
        }
        if (requestedLevel > dali.maxLevel){
            requestedLevel = dali.maxLevel;
            dali.status.limitError = 1;
        }

        // Fading != 0?
        if (dali.fadeTime != 0) {
            if (requestedLevel >  dali.actualDimLevel+1) {   // Minimum of 2 steps (avoid fadePeriod > maxuint16_t)
                if (dali.actualDimLevel == 0) {
                    fadePeriod = FADE_TIME[dali.fadeTime] / (requestedLevel - dali.minLevel+1);
                }
                else {
                    fadePeriod = FADE_TIME[dali.fadeTime] / (requestedLevel - dali.actualDimLevel);
                }

                dimSens = UP;
            }
            else {
                if (requestedLevel <  dali.actualDimLevel-1) {   // Minimum of 2 steps (avoid fadePeriod > maxuint16_t)
                    if (dali.commandByte == 0) {
                        fadePeriod = FADE_TIME[dali.fadeTime] / (dali.actualDimLevel - dali.minLevel+1);
                    }
                    else {
                        fadePeriod = FADE_TIME[dali.fadeTime] / (dali.actualDimLevel - requestedLevel);
                    }

                    dimSens = DOWN;
                }
                else {

                    // Requested level is closed to actual level, fading will not be enabled
                    fadePeriod = 0;    // setting fadePeriod to 0 disables fading!
                    dali.actualDimLevel = requestedLevel;
                }
            }
            if (dali.fadeTime == 15) {

                // 90.510s value does not fit in uint16_t.
                // FADE_TIME[15] = 45.255 and is then multiplied by 2
                fadePeriod = fadePeriod<<1;
            }
        }
        else{
            fadePeriod = 0;    // Setting fadePeriod to 0 disables fading!
            dali.actualDimLevel = requestedLevel;
        }
    }
    else {

        // fade time == 0, immediately set the requested level
        requestedLevel = dali.actualDimLevel;
        fadePeriod = 0;
    }
}


// Used with 'DALI_CMD_UP_200ms' command
void daliUpOutputWithFadeRate(void)
{

    // Check if the lamp is on, and fade rate is != 0
    if ((dali.fadeRate != 0) && (dali.status.lampOn == 1)) {

        // Check limits
        if (dali.actualDimLevel < (dali.maxLevel - FADE_RATE[dali.fadeRate])) {
            requestedLevel = FADE_RATE[dali.fadeRate] + dali.actualDimLevel;
        }
        else {
            requestedLevel = dali.maxLevel;
        }

        if (requestedLevel != dali.actualDimLevel) {
            fadePeriod = 200 / (requestedLevel - dali.actualDimLevel);
            dimSens = UP;
        }
        else {
            fadePeriod = 0;
        }
    }
    else {
      fadePeriod = 0;  // Setting fadePeriod to 0 disables fading!
    }
}


// Used with 'DALI_CMD_DOWN_200ms' command
void daliDownOutputWithFadeRate(void)
{
    // Check if the lamp is on, and fade rate is != 0
    if ((dali.fadeRate != 0) && (dali.status.lampOn == 1)) {

        // check limits
        if (dali.actualDimLevel > (dali.minLevel + FADE_RATE[dali.fadeRate])) {
            requestedLevel =  dali.actualDimLevel - FADE_RATE[dali.fadeRate];
        }
        else {
            requestedLevel = dali.minLevel;
        }

        if (requestedLevel != dali.actualDimLevel) {
            fadePeriod = 200 / (dali.actualDimLevel - requestedLevel);
            dimSens = DOWN;
        }
        else {
            fadePeriod = 0;
        }
    }
    else {
        fadePeriod = 0;    // Setting fadePeriod to 0 disables fading!
    }
}


// Sends a byte in a BW frame
// TODO: use _delay_ms() from avr-lib
void daliAnswer(uint8_t answer)
{
    timeout = 3;            // Delay of 3ms (minimum time between forward & backward frames)
    while (timeout != 0) {
    }

    UDR = answer;           // Writing UDR starts byte transmission

    timeout = 10;           // Delay of 10ms (backward frame duration)
    while (timeout != 0) {
    }

    newRx = 0;
}


// Update arc power level
// Returns the output level required (1 - 254)
uint8_t daliOutputPower(void)
{

    // Output power level is contained in dali.actualDimLevel
    // If dali.actualDimLevel < dali.minLevel, the lamp is off
    if (dali.actualDimLevel >= dali.minLevel) {
        if (dali.actualDimLevel > dali.maxLevel) {
            dali.actualDimLevel = dali.maxLevel;
        }
        dali.status.lampOn = 1;
        return dali.actualDimLevel;
    }
    else {
        dali.status.lampOn = 0;
        return 0;
    }
}

// Address_byte processing.
// Updates cmdType value
void daliAnalyse(void)
{
    static uint8_t storedDaliAddress = 0x00;
    static uint8_t storedDaliCommand = 0x00;

    dali.addressByte = newDaliAddress;
    dali.commandByte = newDaliCommand;

    if (((dali.addressByte & DALI_SPECIAL_CMD_MASK) == DALI_SPECIAL_CMD_1) ||
        ((dali.addressByte & DALI_SPECIAL_CMD_MASK) == DALI_SPECIAL_CMD_2)) {

        // Special command received (101x xxx1 or 110x xxx1)
        if ((dali.addressByte == DALI_CMD_INITIALIZE) || (dali.addressByte == DALI_CMD_RANDOMISE)) {
            if (timeout == 0) {            // first time command is received
                timeout = FW_FW_DELAY;     // 100ms delay (84 + FW frame duration)
                storedDaliAddress = dali.addressByte;
                storedDaliCommand = dali.commandByte;
                dali.cmdType = DALI_CMD_TYPE_NONE;
            }
            else {              // second time command is received
                timeout = 0;    // disable 100ms timer
                if ((dali.addressByte == storedDaliAddress) && (dali.commandByte == storedDaliCommand)) {
                    dali.cmdType = DALI_CMD_TYPE_SPECIAL_CMD;
                }
                else {
                    dali.cmdType = DALI_CMD_TYPE_NONE;
                }
            }
        }
        else {
            dali.cmdType = DALI_CMD_TYPE_SPECIAL_CMD;
        }
        return;
    }
    else {
        if (((dali.addressByte & DALI_BROADCAST) == DALI_BROADCAST) ||
            (((dali.addressByte & 0x80) == 0x00) && (((dali.addressByte >> 1) & 0x3f) == dali.shortAddress)) ||
            (((dali.addressByte & 0x80) == 0x80) && (((dali.addressByte >> 1) & 0x0f) == dali.group))) {

            // Address is accepted
            switch (dali.addressByte & DALI_SELECTOR_BIT_MASK) {
                case DALI_DIRECT_ARC_POWER_CMD:
                    if (dali.commandByte == 0xff) {
                        dali.cmdType = DALI_CMD_TYPE_NONE;
                        return;
                    }
                    dali.cmdType = DALI_CMD_TYPE_DIRECT_ARC_POWER;
                    return;
                    break;

                case DALI_COMAND_FOLLOWING_CMD:
                   if (dali.commandByte < DALI_CMD_RESET) {
                        dali.cmdType = DALI_CMD_TYPE_INDIRECT_ARC_POWER;
                        return;
                    }
                    else {
                        if (dali.commandByte <= DALI_CMD_STORE_DTR_AS_SHORT_ADDRESS) {

                            // Config. command receive (first)
                            // Command needs to be confirmed within 100ms
                            if (timeout == 0) {    // first time command is received
                                timeout = FW_FW_DELAY;
                                storedDaliAddress = dali.addressByte;
                                storedDaliCommand = dali.commandByte;
                                dali.cmdType = DALI_CMD_TYPE_NONE;
                            }
                            else {              // second time command is received
                                timeout = 0;    // disable 100ms timer
                                if ((dali.addressByte == storedDaliAddress) && (dali.commandByte == storedDaliCommand)) {
                                    dali.cmdType = DALI_CMD_TYPE_CONFIG_CMD;
                                }
                                else {
                                    dali.cmdType = DALI_CMD_TYPE_NONE;
                                }
                            }
                            return;
                        }
                        else {
                            if (dali.commandByte <= DALI_CMD_QUERY_RANDOM_ADDRESS_L) {

                                // Qurey command received
                                dali.cmdType = DALI_CMD_TYPE_QUERY_CMD;
                                return;
                            }
                        }
                    }
                    break;
            }   // switch
        }
    }

    dali.cmdType = DALI_CMD_TYPE_NONE;
    return;
}


// Dali main function
// Shall be called every 1 ms ???!!!???
uint8_t daliControlGear(uint8_t lampFailure)
{
    uint8_t outputLevel;
    static uint16_t timeCounter = 0;    // Counts nb of ms elapsed...
    static uint8_t prediv = 0;
    static uint16_t busFailureCounter = 0;

    if (tick1msCounter != 0) {

        // A new tick occured
        if (prediv >= tick1msCounter) {
            prediv -= tick1msCounter;
        }
        else {

            // Here every 1/4th second
            prediv = 250;
            if (specialModeTimeout != 0) {
                specialModeTimeout--;
            }
        }

        if (fadePeriod != 0) {

            // Check if fading is enabled
            timeCounter += tick1msCounter;
            if (timeCounter >= fadePeriod) {

                // Update arc power level for fading
                daliStepOutputPower(dimSens);
                timeCounter = 0;
            }
        }

        if (DALI_RX() == 0) {   // ???!!!???

            // Check if bus is present (idle state is high)
            newRx = 0;     // Disable reception to avoid erroneous detection
            if (busFailureCounter < BUS_FAILURE_TIMEOUT) {
                busFailureCounter += tick1msCounter;
            }
            else {
                if (dali.systemFailureLevel != MASK) {
                    dali.actualDimLevel = dali.systemFailureLevel;
                }
            }
        }
        else {
//             if ((busFailureCounter != 0) && (savedCounter == 0)) {
//                 savedCounter = busFailureCounter;
//             }
            busFailureCounter = 0;
        }

        tick1msCounter = 0;
    }

    // If a new frame is received,
    // if there was no frame error,
    // then process
    if (newRx == 1) {
        newRx = 0;
        daliAnalyse();
        daliExecute();
    }

    outputLevel = daliOutputPower();

    // Check if a lamp is physically disconnected
    // TODO: how and where to check
    if (lampFailure == 1) {
        dali.status.lampFailure = 1;
        if (physicalSelectionMode == PHYSICAL_SELECTION_REQUESTED) {
            physicalSelectionMode = PHYSICAL_SELECTION_ENABLED;
        }
    }
    else {
        dali.status.lampFailure = 0;
        if (physicalSelectionMode == PHYSICAL_SELECTION_ENABLED) {
            physicalSelectionMode = PHYSICAL_SELECTION_REQUESTED;
        }
    }

    return outputLevel;
}


uint8_t isDaliRunning(void)
{
    return daliRunning;
}
