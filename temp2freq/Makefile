###############################################################################
# Makefile for the project temp2freq
###############################################################################

## General Flags
PROJECT = temp2freq
MCU = attiny84
TARGET = temp2freq.elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
#CFLAGS += -Wall -gdwarf-2 -DF_CPU=20000000 -O1 -fsigned-char
CFLAGS += -Wall -gdwarf-2 -DF_CPU=500000 -Os -mcall-prologues -fsigned-char
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
OBJECTS = temp2freq.o

## Build
all: $(TARGET) temp2freq.hex temp2freq.eep size

## Compile
temp2freq.o: temp2freq.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET) -lm

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
	-rm -rf $(OBJECTS) temp2freq.elf dep/ temp2freq.hex temp2freq.eep

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

