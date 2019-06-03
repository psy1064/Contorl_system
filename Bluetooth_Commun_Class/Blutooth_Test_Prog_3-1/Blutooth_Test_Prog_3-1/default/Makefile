###############################################################################
# Makefile for the project Blutooth_Test_Prog_3
###############################################################################

## General Flags
PROJECT = Blutooth_Test_Prog_3
MCU = atmega128
TARGET = Blutooth_Test_Prog_3-1.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99       -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=Blutooth_Test_Prog_3-1.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = lcd.o Blutooth_Test_Prog_3-1.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) Blutooth_Test_Prog_3-1.hex Blutooth_Test_Prog_3-1.eep Blutooth_Test_Prog_3-1.lss## Compile
lcd.o: ../lcd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Blutooth_Test_Prog_3-1.o: ../Blutooth_Test_Prog_3-1.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) Blutooth_Test_Prog_3-1.elf dep/* Blutooth_Test_Prog_3-1.hex Blutooth_Test_Prog_3-1.eep Blutooth_Test_Prog_3-1.lss Blutooth_Test_Prog_3-1.map


## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

