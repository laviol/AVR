#include <stdlib.h>
#include <avr/eeprom.h>

#include "dali.h"
#include "daliCmd.h"

extern DaliRegisters dali;
extern uint8_t requestedLevel;
extern uint16_t specialModeTimeout;
extern uint8_t compareMode;
extern uint8_t physicalSelectionMode;


// Indirect arc power commands
void daliCmdImmediateOff(void)
{
    dali.actualDimLevel = 0;
    return;
}


void daliCmdUp200ms(void)
{
    daliUpOutputWithFadeRate();
    return;
}


void daliCmdDown200ms(void)
{
    daliDownOutputWithFadeRate();
    return;
}


void daliCmdStepUp(void)
{
    if (dali.actualDimLevel < dali.maxLevel && dali.status.lampOn == 1) {
        dali.actualDimLevel++;
    }
    return;
}


void daliCmdStepDown(void)
{
    if (dali.actualDimLevel > dali.minLevel && dali.status.lampOn == 1) {
        dali.actualDimLevel--;
    }
    return;
}


void daliCmdRecallMaxLevel(void)
{
    dali.actualDimLevel = dali.maxLevel;
    return;
}


void daliCmdRecallMinLevel(void)
{
    dali.actualDimLevel = dali.minLevel;
    return;
}


void daliCmdStepDownAndOff(void)
{
    if (dali.status.lampOn == 1) {
        if (dali.actualDimLevel > dali.minLevel) {
            dali.actualDimLevel--;
        }
        else {
            dali.actualDimLevel = 0;
        }
    }
    return;
}


void daliCmdOnAndStepUp(void)
{
    if (dali.status.lampOn == 1) {
        if (dali.actualDimLevel < dali.maxLevel) {
            dali.actualDimLevel++;
        }
    }
    else {
        dali.actualDimLevel = dali.minLevel;
    }
    return;
}


void daliCmdGoToScene(void)
{
    requestedLevel = dali.scene[(dali.commandByte & 0x0f)];
    daliChangeOutputWithFadeTime();
    return;
}


// Settings commands
void daliCmdReset(void)
{
    eeprom_write_byte((uint8_t*)ADD_EEPROM_STATUS, 0);  // clears the flag EEPROM_INITIALIZED
    daliInit();                                         // reset values will be stored in eeprom
    dali.status.resetState = 1;
    dali.status.powerFailure = 0;
}


void daliCmdStoreActualLevelInDTR(void)
{
    dali.dtr = dali.actualDimLevel;
}


void daliCmdStoreTheDTRAsMaxLevel(void)
{
    dali.maxLevel = dali.dtr;
    if (dali.maxLevel <= dali.minLevel) {
        dali.maxLevel = dali.minLevel + 1;
    }
    if (dali.actualDimLevel > dali.maxLevel) {
        dali.actualDimLevel = dali.maxLevel;
    }
    eeprom_write_byte((uint8_t*)ADD_MAX_LEVEL, dali.maxLevel);
}


void daliCmdStoreTheDTRAsMinLevel(void)
{
    dali.minLevel = dali.dtr;
    if (dali.minLevel >= dali.maxLevel) {
        dali.minLevel = dali.maxLevel - 1;
    }
    if (dali.actualDimLevel < dali.minLevel) {
        dali.actualDimLevel = dali.minLevel;
    }
    eeprom_write_byte((uint8_t*)ADD_MIN_LEVEL, dali.minLevel);
}


void daliCmdStoreTheDTRAsSystemFailureLevel(void)
{
    dali.systemFailureLevel = dali.dtr;
    eeprom_write_byte((uint8_t*)ADD_SYSTEM_FAILURE_LEVEL, dali.systemFailureLevel);
}


void daliCmdStoreTheDTRAsPowerOnLevel()
{
    dali.powerOnLevel = dali.dtr;
    if (dali.powerOnLevel > 254) {
        dali.powerOnLevel = 254;
    }
    eeprom_write_byte((uint8_t*)ADD_POWER_ON_LEVEL, dali.powerOnLevel);
}


void daliCmdStoreTheDTRAsFadeTime(void)
{
    dali.fadeTime = dali.dtr & 0xf;
    eeprom_write_byte((uint8_t*)ADD_FADE_TIME, dali.fadeTime);
}


void daliCmdStoreTheDTRAsFadeRate(void)
{
    if (dali.dtr != 0) {    // value 0 is not allowed for fadeRate
        dali.fadeRate = dali.dtr & 0xf;
        eeprom_write_byte((uint8_t*)ADD_FADE_RATE, dali.fadeRate);
    }
}

void daliCmdStoreTheDTRAsShortAddress(void)
{
    if (dali.dtr == 0xff) {
        dali.shortAddress = 0xff;
        dali.status.missingShortAddress = 1;
    }
    else {
        dali.shortAddress = dali.dtr & 0x3f;
        dali.status.missingShortAddress = 0;
    }
    eeprom_write_byte((uint8_t*)ADD_SHORT_ADD, dali.shortAddress);
}


void daliCmdStoreTheDTRAsScene(void)
{
    dali.scene[(dali.commandByte & 0x0f)] = dali.dtr;
    eeprom_write_byte((uint8_t*)(ADD_SCENE_0 + (dali.commandByte & 0x0f)), dali.scene[(dali.commandByte & 0xf)]);
}


void daliCmdRemoveFromScene(void)
{
    dali.scene[(dali.commandByte & 0x0f)] = 0xff;
    eeprom_write_byte((uint8_t*)(ADD_SCENE_0 + (dali.commandByte & 0x0f)), 0xff);
}


void daliCmdAddToGroup(void)
{
    dali.group = 1 << (dali.commandByte & 0xf);
    eeprom_write_byte((uint8_t*)ADD_GROUPH, ((uint8_t)(dali.group >> 8)));
    eeprom_write_byte((uint8_t*)ADD_GROUPL, ((uint8_t)(dali.group)));
}


void daliCmdRemoveFromGroup(void)
{
    dali.group = 0x0000;
    eeprom_write_byte((uint8_t*)ADD_GROUPH, 0);
    eeprom_write_byte((uint8_t*)ADD_GROUPL, 0);
}


// Query commands
void daliCmdQueryStatus(void)
{
    daliAnswer(dali.status.statusInformation);
    return;
}


void daliCmdQueryBallast(void)
{
    daliAnswer(DALI_YES);
    return;
}


void daliCmdQueryLampFailure(void)
{
    if (dali.status.lampFailure == 1) {
        daliAnswer(DALI_YES);
    }
    // no answer means 'DALI_NO'
  return;
}


void daliCmdQueryLampPowerOn(void)
{
    if (dali.status.lampOn == 1) {
        daliAnswer(DALI_YES);
    }
    // no answer means 'DALI_NO'
    return;
}


void daliCmdqueryLimitError(void)
{
    if (dali.status.limitError == 1) {
        daliAnswer(DALI_YES);
    }
    // no answer means 'DALI_NO'
    return;
}


void daliCmdQueryResetState(void)
{
    if (dali.status.resetState == 1) {
        daliAnswer(DALI_YES);
    }
    // no answer means 'DALI_NO'
    return;
}


void daliCmdQueryMissingShortAddress(void)
{
    if (dali.status.missingShortAddress == 1) {
        daliAnswer(DALI_YES);
    }
    // no answer means 'DALI_NO'
    return;
}


void daliCmdQueryVersionNumber(void)
{
    daliAnswer(DALI_VERSION_NUMBER);
    return;
}


void daliCmdQueryContentDTR(void)
{
    daliAnswer(dali.dtr);
    return;
}


void daliCmdQueryDeviceType(void)
{
    daliAnswer(DALI_DEVICE_TYPE);
    return;
}


void daliCmdQueryPhysicalMinimumLevel(void)
{
    daliAnswer(DALI_PHYSICAL_MIN_LEVEL);
    return;
}


void daliCmdQueryPowerFailure(void)
{
    daliAnswer(dali.status.powerFailure);
    return;
}


void daliCmdQueryActualLevel(void)
{
    daliAnswer(dali.actualDimLevel);
    return;
}


void daliCmdQueryMaxLevel(void)
{
    daliAnswer(dali.maxLevel);
    return;
}


void daliCmdQueryMinLevel(void)
{
    daliAnswer(dali.minLevel);
    return;
}


void daliCmdQueryPowerOnLevel(void)
{
    daliAnswer(dali.powerOnLevel);
    return;
}


void daliCmdQuerySystemFailureLevel(void)
{
    daliAnswer(dali.systemFailureLevel);
    return;
}


void daliCmdQueryFadeSettings(void)
{
    daliAnswer(dali.fadeTime << 4 | dali.fadeRate);
    return;
}


void daliCmdQuerySceneLevel(void)
{
    daliAnswer(dali.scene[(dali.commandByte & 0x0f)]);
    return;
}


void daliCmdQueryGroups0_7(void)
{
    daliAnswer((uint8_t)(dali.group));
    return;
}


void daliCmdQueryGroups8_15(void)
{
    daliAnswer((uint8_t)(dali.group >> 8));
     return;
}


void daliCmdQueryRandomAddressH(void)
{
    daliAnswer(dali.randomAddressH);
    return;
}

void daliCmdQueryRandomAddressM(void)
{
    daliAnswer(dali.randomAddressM);
    return;
}


void daliCmdQueryRandomAddressL(void)
{
    daliAnswer(dali.randomAddressL);
    return;
}


// Extended commands
void daliCmdTerminate(void)
{
    specialModeTimeout = 0;
    compareMode = 0;
    physicalSelectionMode = PHYSICAL_SELECTION_DISABLED;
    return;
}


void daliCmdDTR(void)
{
    dali.dtr = dali.commandByte;
    return;
}


void daliCmdInitialize(void)
{
    if ((dali.commandByte == 0) ||                                             // Broadcast
        ((dali.commandByte & 0xfe) == ((dali.shortAddress << 1) & 0xfe)) ||    // Short address OK ???
        ((dali.commandByte == 0xff) && (dali.shortAddress == 0xff))) {         // No short address ???
        specialModeTimeout = 3600;    // enables special commands for 3600 * 1/4th second = 15min.
        compareMode = 1;
    }
    return;
}


void daliCmdRandomise(void)
{
    if (specialModeTimeout != 0) {      // if specialModeTimeout > 0 , specialMode is enabled
        srand(TCNT1L);                  // initialise the first random value of a list
        dali.randomAddressH = rand();   // take the next random value
        dali.randomAddressM = rand();
        dali.randomAddressL = rand();
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDH, dali.randomAddressH);
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDM, dali.randomAddressM);
        eeprom_write_byte((uint8_t*)ADD_RANDOM_ADDL, dali.randomAddressL);
    }
    return;
}


void daliCmdCompare(void)
{
    if ((specialModeTimeout != 0) && (compareMode == 1)) {

        // specialMode enabled && compareMode enabled
        if (dali.randomAddressH > dali.searchAddressH) {
            return;     // answer 'DALI_NO'
        }
        else {
            if (dali.randomAddressH == dali.searchAddressH) {
                if (dali.randomAddressM > dali.searchAddressM) {
                    return;     // answer 'DALI_NO'
                }
                else {
                    if (dali.randomAddressM == dali.searchAddressM) {
                        if (dali.randomAddressL > dali.searchAddressL) {
                            return;     // answer 'DALI_NO'
                        }
                    }
                }
            }
        }
        daliAnswer(DALI_YES);
    }
    return;
}


void daliCmdWithdraw(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        if ((dali.randomAddressH == dali.searchAddressH) &&
            (dali.randomAddressM == dali.searchAddressM) &&
            (dali.randomAddressL == dali.searchAddressL)) {
            compareMode = 0;   // disable compare
        }
    }
    return;
}


void daliCmdSearchAddressH(void)
{
    if (specialModeTimeout != 0) {   // specialMode enabled
        dali.searchAddressH = dali.commandByte;
    }
    return;
}


void daliCmdSearchAddressM(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        dali.searchAddressM = dali.commandByte;
    }
    return;
}


void daliCmdSearchAddressL(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        dali.searchAddressL = dali.commandByte;
    }
    return;
}


void daliCmdProgramShortAddress(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        if (((dali.randomAddressH == dali.searchAddressH) &&
            (dali.randomAddressM == dali.searchAddressM) &&
            (dali.randomAddressL == dali.searchAddressL) &&
            (physicalSelectionMode == PHYSICAL_SELECTION_DISABLED)) ||  // ???!!!???
            (physicalSelectionMode == PHYSICAL_SELECTION_ENABLED)) {    // ???!!!???
            if (dali.commandByte == 0xff) {

                // clear short address
                dali.shortAddress = 0xff;
                dali.status.missingShortAddress = 1;
            }
            else {
                dali.shortAddress = dali.commandByte >> 1;
                dali.status.missingShortAddress = 0;
            }
            eeprom_write_byte((uint8_t*)ADD_SHORT_ADD, dali.shortAddress);
        }
    }
    return;
}


void daliCmdVerifyShortAddress(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        if ((dali.commandByte & 0xfe) == ((dali.shortAddress << 1) & 0xfe)) {
            daliAnswer(DALI_YES);
        }
    }
    return;
}


void daliCmdQueryShortAddress(void)
{
    if (specialModeTimeout != 0) {   // specialMode enabled
        if (((dali.randomAddressH == dali.searchAddressH) &&
            (dali.randomAddressM == dali.searchAddressM) &&
            (dali.randomAddressL == dali.searchAddressL) &&
            (physicalSelectionMode == PHYSICAL_SELECTION_DISABLED)) ||  // ???!!!???
            (physicalSelectionMode == PHYSICAL_SELECTION_ENABLED)) {    // ???!!!???
            daliAnswer((dali.shortAddress << 1) | 1);
        }
    }
    return;
}


void daliCmdPhysicalSelection(void)
{
    if (specialModeTimeout != 0) {  // specialMode enabled
        if (physicalSelectionMode == PHYSICAL_SELECTION_DISABLED) {
            physicalSelectionMode = PHYSICAL_SELECTION_REQUESTED;     // toggles physical selection mode
            compareMode = 0;
        }
        else {
            physicalSelectionMode = PHYSICAL_SELECTION_DISABLED;
            compareMode = 1;   // reactivate search & random address comparison
        }
    }
    return;
}


void daliCmdEnableDeviceTypeX(void)
{
    return;
}
