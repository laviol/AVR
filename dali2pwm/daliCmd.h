#ifndef _DALI_CMD_
#define _DALI_CMD_

// Pre-defined response on query
#define DALI_YES                                    0xFF
#define DALI_NO                                     0x00
#define DALI_MASK                                   0xFF
#define DALI_DONT_CHANGE                            0xFF

// Command analysis
#define DALI_BROADCAST                              0xFE
#define DALI_SPECIAL_CMD_MASK                       0xE1
#define DALI_SPECIAL_CMD_1                          0xA1
#define DALI_SPECIAL_CMD_2                          0xC1
#define DALI_SELECTOR_BIT_MASK                      0x01
#define DALI_DIRECT_ARC_POWER_CMD                   0x00
#define DALI_COMAND_FOLLOWING_CMD                   0x01

// Arc power control commands
#define DALI_CMD_IMMEDIATE_OFF                              0x00
#define DALI_CMD_UP_200MS                                   0x01
#define DALI_CMD_DOWN_200MS                                 0x02
#define DALI_CMD_STEP_UP                                    0x03
#define DALI_CMD_STEP_DOWN                                  0x04
#define DALI_CMD_RECALL_MAX_LEVEL                           0x05
#define DALI_CMD_RECALL_MIN_LEVEL                           0x06
#define DALI_CMD_STEP_DOWN_AND_OFF                          0x07
#define DALI_CMD_ON_AND_STEP_UP                             0x08
#define DALI_CMD_GO_TO_SCENE                                0x10  // 'commandByte' => "scene"

// General configuration commands
// Need to be received twice
#define DALI_CMD_RESET                                      0x20
#define DALI_CMD_STORE_ACTUAL_LEVEL_IN_DTR                  0x21  // "actualDimLevel" => 'dtr'

// Arc power parameters settings
// Need to be received twice
#define DALI_CMD_STORE_THE_DTR_AS_MAX_LEVEL                 0x2A  // 'dtr' => "maxLevel"
#define DALI_CMD_STORE_THE_DTR_AS_MIN_LEVEL                 0x2B  // 'dtr' => "minLevel"
#define DALI_CMD_STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL      0x2C  // 'dtr' => "systemFailureLevel"
#define DALI_CMD_STORE_THE_DTR_AS_POWER_ON_LEVEL            0x2D  // 'dtr' => "powerOnLevel"
#define DALI_CMD_STORE_THE_DTR_AS_FADE_TIME                 0x2E  // 'dtr' => "fadeTime"
#define DALI_CMD_STORE_THE_DTR_AS_FADE_RATE                 0x2F  // 'dtr' => "fadeRate"
#define DALI_CMD_STORE_THE_DTR_AS_SCENE                     0x40  // 'commandByte' => "scene", 'dtr' => new "sceneLevel"

// System parameters settings
// Need to be received twice
#define DALI_CMD_REMOVE_FROM_SCENE                          0x50  // 'commandByte' => "scene"
#define DALI_CMD_ADD_TO_GROUP                               0x60  // 'commandByte' => "group"
#define DALI_CMD_REMOVE_FROM_GROUP                          0x70  // 'commandByte' => "group"
#define DALI_CMD_STORE_DTR_AS_SHORT_ADDRESS                 0x80  // 'dtr' => "shortAddress"

// Queries
#define DALI_CMD_QUERY_STATUS                               0x90  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_BALLAST                              0x91  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_LAMP_FAILURE                         0x92  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_LAMP_POWER_ON                        0x93  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_LIMIT_ERROR                          0x94  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_RESET_STATE                          0x95  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_MISSING_SHORT_ADDRESS                0x96  // "DALI_YES" => 'commandByte' else nothing
#define DALI_CMD_QUERY_VERSION_NUMBER                       0x97  // "version_number" => 'commandByte'
#define DALI_CMD_QUERY_CONTENT_DTR                          0x98  // 'dtr' => 'commandByte'
#define DALI_CMD_QUERY_DEVICE_TYPE                          0x99  // "deviceType" => 'commandByte'
#define DALI_CMD_QUERY_PHYSICAL_MINIMUM_LEVEL               0x9A  // "physicalMinimumLevel" => 'commandByte'
#define DALI_CMD_QUERY_POWER_FAILURE                        0x9B  // ... response (c.f. DALI standard) => 'commandByte'
#define DALI_CMD_QUERY_ACTUAL_LEVEL                         0xA0  // "actualDimLevel" or "DALI_MASK" => 'commandByte'
#define DALI_CMD_QUERY_MAX_LEVEL                            0xA1  // "maxLevel" => 'commandByte'
#define DALI_CMD_QUERY_MIN_LEVEL                            0xA2  // "minLevel" => 'commandByte'
#define DALI_CMD_QUERY_POWER_ON_LEVEL                       0xA3  // "powerOnLevel" => 'commandByte'
#define DALI_CMD_QUERY_SYSTEM_FAILURE_LEVEL                 0xA4  // "systemFailureLevel" => 'commandByte'
#define DALI_CMD_QUERY_FADE_SETTINGS                        0xA5  // "fadeTime, fadeRate" => 'commandByte'
#define DALI_CMD_QUERY_SCENE_LEVEL                          0xB0  // 'commandByte' => "scene", "scene_level" => 'commandByte'
#define DALI_CMD_QUERY_GROUPS_0_7                           0xC0  // "group0_7" => 'commandByte'
#define DALI_CMD_QUERY_GROUPS_8_15                          0xC1  // "group8_15" => 'commandByte'
#define DALI_CMD_QUERY_RANDOM_ADDRESS_H                     0xC2  // "randomAddressH" => 'commandByte'
#define DALI_CMD_QUERY_RANDOM_ADDRESS_M                     0xC3  // "randomAddressM" => 'commandByte'
#define DALI_CMD_QUERY_RANDOM_ADDRESS_L                     0xC4  // "randomAddressHL" => 'commandByte'

// Use of extended commands ???
#define DALI_CMD_QUERY_APPLICATION_EXTENTED_COMMAND         0xE0  // for "deviceType" = 0, extended commands are not used

// Extended commands
#define DALI_CMD_TERMINATE                                  0xA1
#define DALI_CMD_DTR                                        0xA3  // received direct data => 'dtr'

// Extended commands
// Need to be received twice
#define DALI_CMD_INITIALIZE                                 0xA5    // 'commandByte' => "reaction_of_ballasts"
#define DALI_CMD_RANDOMISE                                  0xA7

// Extended commands
#define DALI_CMD_COMPARE                                    0xA9
#define DALI_CMD_WITHDRAW                                   0xAB
#define DALI_CMD_SEARCH_ADDRESS_H                           0xB1    // 'commandByte' => "search_address.h"
#define DALI_CMD_SEARCH_ADDRESS_M                           0xB3    // 'commandByte' => "search_address.m"
#define DALI_CMD_SEARCH_ADDRESS_L                           0xB5    // 'commandByte' => "search_address.l"
#define DALI_CMD_PROGRAM_SHORT_ADDRESS                      0xB7    // 'commandByte' => "shortAddress"
#define DALI_CMD_VERIFY_SHORT_ADDRESS                       0xB9    // 'commandByte' => "shortAddress", "DALI_YES" => 'commandByte' if equal
#define DALI_CMD_QUERY_SHORT_ADDRESS                        0xBB    // "shortAddress" or "DALI_MASK" => 'commandByte'
#define DALI_CMD_PHYSICAL_SELECTION                         0xBD

// Extended commands - Special extended command
#define DALI_CMD_ENABLE_DEVICE_TYPE_X                       0xC1    // for "deviceType" = 0, extended commands are not used


// Indirect arc power ocmmands
void daliCmdImmediateOff(void);
void daliCmdUp200ms(void);
void daliCmdDown200ms(void);
void daliCmdStepUp(void);
void daliCmdStepDown(void);
void daliCmdRecallMaxLevel(void);
void daliCmdRecallMinLevel(void);
void daliCmdStepDownAndOff(void);
void daliCmdOnAndStepUp(void);
void daliCmdGoToScene(void);

// Settings commands
void daliCmdReset(void);
void daliCmdStoreActualLevelInDTR(void);
void daliCmdStoreTheDTRAsMaxLevel(void);
void daliCmdStoreTheDTRAsMinLevel(void);
void daliCmdStoreTheDTRAsSystemFailureLevel(void);
void daliCmdStoreTheDTRAsPowerOnLevel(void);
void daliCmdStoreTheDTRAsFadeTime(void);
void daliCmdStoreTheDTRAsFadeRate(void);
void daliCmdStoreTheDTRAsShortAddress(void);
void daliCmdStoreTheDTRAsScene(void);
void daliCmdRemoveFromScene(void);
void daliCmdAddToGroup(void);
void daliCmdRemoveFromGroup(void);

// Query commands
void daliCmdQueryStatus(void);
void daliCmdQueryBallast(void);
void daliCmdQueryLampFailure(void);
void daliCmdQueryLampPowerOn(void);
void daliCmdqueryLimitError(void);
void daliCmdQueryResetState(void);
void daliCmdQueryMissingShortAddress(void);
void daliCmdQueryVersionNumber(void);
void daliCmdQueryContentDTR(void);
void daliCmdQueryDeviceType(void);
void daliCmdQueryPhysicalMinimumLevel(void);
void daliCmdQueryPowerFailure(void);
void daliCmdQueryActualLevel(void);
void daliCmdQueryMaxLevel(void);
void daliCmdQueryMinLevel(void);
void daliCmdQueryPowerOnLevel(void);
void daliCmdQuerySystemFailureLevel(void);
void daliCmdQueryFadeSettings(void);
void daliCmdQuerySceneLevel(void);
void daliCmdQueryGroups0_7(void);
void daliCmdQueryGroups8_15(void);
void daliCmdQueryRandomAddressH(void);
void daliCmdQueryRandomAddressM(void);
void daliCmdQueryRandomAddressL(void);

// Extended commands
void daliCmdTerminate(void);
void daliCmdDTR(void);
void daliCmdInitialize(void);
void daliCmdRandomise(void);
void daliCmdCompare(void);
void daliCmdWithdraw(void);
void daliCmdSearchAddressH(void);
void daliCmdSearchAddressM(void);
void daliCmdSearchAddressL(void);
void daliCmdProgramShortAddress(void);
void daliCmdVerifyShortAddress(void);
void daliCmdQueryShortAddress(void);
void daliCmdPhysicalSelection(void);
void daliCmdEnableDeviceTypeX(void);

#endif
