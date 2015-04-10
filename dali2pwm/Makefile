###############################################################################
# Makefile for the project dali
###############################################################################

## Fuses (put them in the code)
## -U lfuse:w:0x50:m -U hfuse:w:0xdf:m -U efuse:w:0xf9:m

## General Flags
PROJECT = dali2pwm
MCU = at90pwm216
TARGET = $(PROJECT).elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
#CFLAGS += -Wall -gdwarf-2 -DF_CPU=20000000 -O1 -fsigned-char
CFLAGS += -Wall -gdwarf-2 -DF_CPU=16000000 -Os -mcall-prologues -fsigned-char
CFLAGS += -Wp,-M,-MP,-MT,$(*F).o,-MF,dep/$(@F).d

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0


## Objects that must be built in order to link
OBJECTS = main.o dali.o daliCmd.o daliExecute.o

## Build
all: $(TARGET) $(PROJECT).hex $(PROJECT).eep size

## Compile
dali.o: dali.c main.h dali.h daliCmd.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

daliCmd.o: daliCmd.c daliCmd.h dali.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

daliExecute.o: daliExecute.c daliCmd.h dali.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

main.o: main.c main.h dali.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS) $< $@

%.eep: $(TARGET)
	avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-mem.sh ${TARGET} ${MCU}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(PROJECT).elf dep/ $(PROJECT).hex $(PROJECT).eep

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

