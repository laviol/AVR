#ifndef _DALI_LIB_H_
#define _DALI_LIB_H_

#include <inttypes.h>

// Timer0 confirguration
#define F_DALI_TICK     1000    // 1kHz -> tick every 1ms

#define TIMER0_DIVIDER_1    (0 << CS02) | (0 << CS01) | (1 << CS00)     // Timer0 frequency devider :1
#define TIMER0_DIVIDER_8    (0 << CS02) | (1 << CS01) | (0 << CS00)     // Timer0 frequency devider :8
#define TIMER0_DIVIDER_64   (0 << CS02) | (1 << CS01) | (1 << CS00)     // Timer0 frequency devider :64

// EUSART configuration and macros
#define DALI_BAUD_RATE  1200
#define MUBRR           (F_CLKIO / DALI_BAUD_RATE)
#define UBRR            (F_CLKIO / (16 * DALI_BAUD_RATE) - 1)

#define DALI_ENABLE_RX()    (UCSRB |= 1 << RXEN )
#define DALI_DISABLE_RX()   (UCSRB &= ~(1 << RXEN))
#define DALI_RX()           ((PIND & (1 << PIND4)) == 0 ? 0 : 1)

// General
#define DOWN                    0
#define UP                      1
#define FW_FW_DELAY             84      // (84ms + FW frame duration) = 100ms
#define BUS_FAILURE_TIMEOUT     500     // 500ms
#define MASK                    0xff

#define PHYSICAL_SELECTION_DISABLED     0
#define PHYSICAL_SELECTION_REQUESTED    1
#define PHYSICAL_SELECTION_ENABLED      2

// EEPROM Addresses of stored DALI Registers
#define ADD_EEPROM_STATUS           0
#define ADD_POWER_ON_LEVEL          1
#define ADD_SYSTEM_FAILURE_LEVEL    2
#define ADD_MIN_LEVEL               3
#define ADD_MAX_LEVEL               4
#define ADD_FADE_RATE               5
#define ADD_FADE_TIME               6
#define ADD_SHORT_ADD               7
#define ADD_RANDOM_ADDH             8
#define ADD_RANDOM_ADDM             9
#define ADD_RANDOM_ADDL             10
#define ADD_GROUPH                  11
#define ADD_GROUPL                  12
#define ADD_SCENE_0                 13

#define EEPROM_INITIALIZED          0xAA    // if eeprom(0) == 0xAA : eeprom has been written at least once

#ifndef DALI_PHYSICAL_LEVEL
    #define DALI_PHYSICAL_LEVEL     0
#endif
#ifndef DALI_VERSION_NUMBER
    #define DALI_VERSION_NUMBER     0x00
#endif

#define DALI_VERSION_NUMBER         0x00
#define DALI_PHYSICAL_MIN_LEVEL     50      // Why not 0?
#define DALI_DEVICE_TYPE            0


// Type of DALI Command received
typedef enum {
    DALI_CMD_TYPE_NONE,
    DALI_CMD_TYPE_DIRECT_ARC_POWER,
    DALI_CMD_TYPE_CONFIG_CMD,
    DALI_CMD_TYPE_QUERY_CMD,
    DALI_CMD_TYPE_SPECIAL_CMD,
    DALI_CMD_TYPE_INDIRECT_ARC_POWER
} DaliCmdType;

// 'STATUS REGISTER' uint8_ts
typedef union
{
    struct {
        unsigned ballastFailure       :1;
        unsigned lampFailure          :1;
        unsigned lampOn               :1;
        unsigned limitError           :1;
        unsigned fadeRunning          :1;
        unsigned resetState           :1;
        unsigned missingShortAddress  :1;
        unsigned powerFailure         :1;
    };
    char statusInformation;
} DaliStatus;

// DALI Registers
typedef struct {
    DaliCmdType     cmdType;
    uint8_t         addressByte;            // 1st byte of received frame
    uint8_t         commandByte;            // 2nd byte of received frame
    uint8_t         dtr;                    // Data Transfer Register
    uint8_t         actualDimLevel;         // See DALI Standard
    uint8_t         powerOnLevel;
    uint8_t         systemFailureLevel;
    uint8_t         minLevel;
    uint8_t         maxLevel;
    uint8_t         fadeRate;
    uint8_t         fadeTime;
    uint8_t         shortAddress;
    uint8_t         searchAddressH;
    uint8_t         searchAddressM;
    uint8_t         searchAddressL;
    uint8_t         randomAddressH;
    uint8_t         randomAddressM;
    uint8_t         randomAddressL;
    uint16_t        group;                  // MSB : group 15    LSB : group 0. If set, device belongs to group x
    uint8_t         scene[16];
    DaliStatus      status;
} DaliRegisters;


// General functions
void daliInitEUSART(void);
void daliInit(void);
void daliTick(void);
void daliAnalyse(void);
void daliStepOutputPower(uint8_t sense);
void daliChangeOutputWithFadeTime(void);
void daliUpOutputWithFadeRate(void);
void daliDownOutputWithFadeRate(void);
void daliAnswer(uint8_t answer);
void daliExecute(void);
uint8_t daliOutputPower(void);
uint8_t daliControlGear(uint8_t);
uint8_t isDaliRunning(void);

#endif
