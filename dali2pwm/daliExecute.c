#include "dali.h"
#include "daliCmd.h"

extern DaliRegisters dali;
extern uint8_t requestedLevel;


// Data_byte processing
void daliExecute(void)
{
    uint8_t cmdNumber = 0;

    switch (dali.cmdType) {
        case DALI_CMD_TYPE_DIRECT_ARC_POWER:
            requestedLevel = dali.commandByte;
            daliChangeOutputWithFadeTime();
            dali.status.resetState = 0;
            dali.status.powerFailure = 0;
            break;

        case DALI_CMD_TYPE_SPECIAL_CMD:
            switch (dali.commandByte) {
                case DALI_CMD_TERMINATE:
                    daliCmdTerminate();
                    break;

                case DALI_CMD_DTR:
                    daliCmdTerminate();
                    break;

                case DALI_CMD_INITIALIZE:
                    daliCmdInitialize();
                    break;

                case DALI_CMD_RANDOMISE:
                    daliCmdRandomise();
                    break;

                case DALI_CMD_COMPARE:
                    daliCmdCompare();
                    break;

                case DALI_CMD_WITHDRAW:
                    daliCmdWithdraw();
                    break;

                case DALI_CMD_SEARCH_ADDRESS_H:
                    daliCmdSearchAddressH();
                    break;

                case DALI_CMD_SEARCH_ADDRESS_M:
                    daliCmdSearchAddressM();
                    break;

                case DALI_CMD_SEARCH_ADDRESS_L:
                    daliCmdSearchAddressL();
                    break;

                case DALI_CMD_PROGRAM_SHORT_ADDRESS:
                    daliCmdProgramShortAddress();
                    break;

                case DALI_CMD_VERIFY_SHORT_ADDRESS:
                    daliCmdVerifyShortAddress();
                    break;

                case DALI_CMD_QUERY_SHORT_ADDRESS:
                    daliCmdQueryShortAddress();
                    break;

                case DALI_CMD_PHYSICAL_SELECTION:
                    daliCmdPhysicalSelection();
                    break;

                case DALI_CMD_ENABLE_DEVICE_TYPE_X:
                    daliCmdEnableDeviceTypeX();
                    break;
            }
            dali.status.resetState = 0;
            break;

        case DALI_CMD_TYPE_INDIRECT_ARC_POWER:
            cmdNumber = dali.commandByte & 0x1f;
            switch (cmdNumber) {
                case DALI_CMD_IMMEDIATE_OFF:
                    daliCmdImmediateOff();
                    break;

                case DALI_CMD_UP_200MS:
                    daliCmdUp200ms();
                    break;

                case DALI_CMD_DOWN_200MS:
                    daliCmdDown200ms();
                    break;

                case DALI_CMD_STEP_UP:
                    daliCmdStepUp();
                    break;

                case DALI_CMD_STEP_DOWN:
                    daliCmdStepDownAndOff();
                    break;

                case DALI_CMD_RECALL_MAX_LEVEL:
                    daliCmdRecallMaxLevel();
                    break;

                case DALI_CMD_RECALL_MIN_LEVEL:
                    daliCmdRecallMinLevel();
                    break;

                case DALI_CMD_STEP_DOWN_AND_OFF:
                    daliCmdStepDownAndOff();
                    break;

                case DALI_CMD_ON_AND_STEP_UP:
                    daliCmdOnAndStepUp();
                    break;

                default:
                    // TODO: use pattern?
                    if ((cmdNumber >= DALI_CMD_GO_TO_SCENE) && (cmdNumber <= DALI_CMD_GO_TO_SCENE + 15)) {
                        daliCmdGoToScene();
                    }
                    break;
            }
            dali.status.resetState = 0;
            dali.status.powerFailure = 0;
            break;

        case DALI_CMD_TYPE_CONFIG_CMD:
            dali.status.resetState = 0;
            switch (dali.commandByte) {
                case DALI_CMD_RESET:
                    daliCmdReset();
                    break;

                case DALI_CMD_STORE_ACTUAL_LEVEL_IN_DTR:
                    daliCmdStoreActualLevelInDTR();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_MAX_LEVEL:
                    daliCmdStoreTheDTRAsMaxLevel();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_MIN_LEVEL:
                    daliCmdStoreTheDTRAsMinLevel();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL:
                    daliCmdStoreTheDTRAsSystemFailureLevel();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_POWER_ON_LEVEL:
                    daliCmdStoreTheDTRAsPowerOnLevel();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_FADE_TIME:
                    daliCmdStoreTheDTRAsFadeTime();
                    break;

                case DALI_CMD_STORE_THE_DTR_AS_FADE_RATE:
                    daliCmdStoreTheDTRAsFadeRate();
                    break;

                case DALI_CMD_STORE_DTR_AS_SHORT_ADDRESS:
                    daliCmdStoreTheDTRAsShortAddress();
                    break;

                default:
                    switch (dali.commandByte & 0xf0) {
                        case DALI_CMD_STORE_THE_DTR_AS_SCENE:
                            daliCmdStoreTheDTRAsScene();
                            break;

                        case DALI_CMD_REMOVE_FROM_SCENE:
                            daliCmdRemoveFromScene();
                            break;

                        case DALI_CMD_ADD_TO_GROUP:
                            daliCmdAddToGroup();
                            break;

                        case DALI_CMD_REMOVE_FROM_GROUP:
                            daliCmdRemoveFromGroup();
                            break;
                    }
                    break;
            }
            break;

        case DALI_CMD_TYPE_QUERY_CMD:
            switch (dali.commandByte) {
                case DALI_CMD_QUERY_STATUS:
                    daliCmdQueryStatus();
                    break;

                case DALI_CMD_QUERY_BALLAST:
                    daliCmdQueryBallast();
                    break;

                case DALI_CMD_QUERY_LAMP_FAILURE:
                    daliCmdQueryLampFailure();
                    break;

                case DALI_CMD_QUERY_LAMP_POWER_ON:
                    daliCmdQueryLampPowerOn();
                    break;

                case DALI_CMD_QUERY_LIMIT_ERROR:
                    daliCmdqueryLimitError();
                    break;

                case DALI_CMD_QUERY_RESET_STATE:
                    daliCmdQueryResetState();
                    break;

                case DALI_CMD_QUERY_MISSING_SHORT_ADDRESS:
                    daliCmdQueryMissingShortAddress();
                    break;

                case DALI_CMD_QUERY_VERSION_NUMBER:
                    daliCmdQueryVersionNumber();
                    break;

                case DALI_CMD_QUERY_CONTENT_DTR:
                    daliCmdQueryContentDTR();
                    break;

                case DALI_CMD_QUERY_DEVICE_TYPE:
                    daliCmdQueryDeviceType();
                    break;

                case DALI_CMD_QUERY_PHYSICAL_MINIMUM_LEVEL:
                    daliCmdQueryPhysicalMinimumLevel();
                    break;

                case DALI_CMD_QUERY_POWER_FAILURE:
                    daliCmdQueryPowerFailure();
                    break;

                case DALI_CMD_QUERY_ACTUAL_LEVEL:
                    daliCmdQueryActualLevel();
                    break;

                case DALI_CMD_QUERY_MAX_LEVEL:
                    daliCmdQueryMaxLevel();
                    break;

                case DALI_CMD_QUERY_MIN_LEVEL:
                    daliCmdQueryMinLevel();
                    break;

                case DALI_CMD_QUERY_POWER_ON_LEVEL:
                    daliCmdQueryPowerOnLevel();
                    break;

                case DALI_CMD_QUERY_SYSTEM_FAILURE_LEVEL:
                    daliCmdQuerySystemFailureLevel();
                    break;

                case DALI_CMD_QUERY_FADE_SETTINGS:
                    daliCmdQueryFadeSettings();
                    break;

                case DALI_CMD_QUERY_GROUPS_0_7:
                    daliCmdQueryGroups0_7();
                    break;

                case DALI_CMD_QUERY_GROUPS_8_15:
                    daliCmdQueryGroups8_15();
                    break;

                case DALI_CMD_QUERY_RANDOM_ADDRESS_H:
                    daliCmdQueryRandomAddressH();
                    break;

                case DALI_CMD_QUERY_RANDOM_ADDRESS_M:
                    daliCmdQueryRandomAddressM();
                    break;

                case DALI_CMD_QUERY_RANDOM_ADDRESS_L:
                    daliCmdQueryRandomAddressL();
                    break;

                default:
                    // TODO: use pattern?
                    if ((dali.commandByte >= DALI_CMD_QUERY_SCENE_LEVEL) && (dali.commandByte < DALI_CMD_QUERY_GROUPS_0_7)) {
                        daliCmdQuerySceneLevel();
                    }
                    break;
            }
            break;

        case DALI_CMD_TYPE_NONE:
            break;
    }
}
