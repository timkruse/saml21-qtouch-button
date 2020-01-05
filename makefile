# Makefile for ARM on macos

#	Naming the target and defining the build paths structure
#	Default is the name of the current directory
NAME = $(notdir $(shell pwd))
VERBOSE=@

MCU = SAML21G18B

#	Toolchain Paths (variables defined with := are expanded once, but variables defined with = are expanded whenever they are used)
#	ARM GCC installation path
ARM_GCC_PATH := ~/Applications/arm-dev/gcc-arm-none-eabi-8-2018-q4-major
#	OpenOCD installation path (brew --prefix openocd)
OPENOCD_PATH := /usr/local/Cellar/open-ocd/0.10.0
#	ASF installation path
ASF_PATH := ~/Applications/arm-dev/xdk-asf-3.45.0

BUILD_DIR = build
OBJECT_DIR = $(BUILD_DIR)/obj
DEPENDENCIES_DIR = $(BUILD_DIR)/dep

#	Additional src and include folders
FOLDER = driver qtouch/include

TARGET = $(BUILD_DIR)/$(NAME)

#	generating include flags for each includepath
# 	obtaining compiler default includes via "gcc -xc++ -E -v -"
INCLUDE_DIR = xdk-asf xdk-asf/include xdk-asf/cmsis $(FOLDER)
INCLUDE_FLAGS = $(INCLUDE_DIR:%=-I %)

SHARED_LIBS_PATHS = qtouch
# SYSTEM_LIBS_PATHS = lib/gcc/arm-none-eabi/8.2.1 lib/gcc lib/gcc/arm-none-eabi/8.2.1/thumb/v6-m/nofp arm-none-eabi/lib arm-none-eabi/lib/thumb/v6-m/nofp
SHARED_LIBS_PATHS_FLAGS = $(SYSTEM_LIBS_PATHS:%=-L $(ARM_GCC_PATH)/%) $(SHARED_LIBS_PATHS:%=-L %) # mind the space between -L and %! Important so ~ in paths are expanded

#	shared libs
#	scan each dir in SHARED_LIBS_PATHS for *.a shared libs and remove path, extension and lib-prefix
SHARED_LIBS = $(foreach dir, $(SHARED_LIBS_PATHS), $(notdir $(basename $(wildcard $(dir)/*.a))))
# SYSTEM_LIBS = c gcc stdc++ m
SHARED_LIBS_FLAGS = --start-group $(SYSTEM_LIBS:%=-l %) --end-group $(SHARED_LIBS:lib%=-l %)

#	defines
DEFINES = __$(MCU)__
DEFINES_FLAGS = $(DEFINES:%=-D%)

#	collecting all source files in the same directory as this makefile
SRC = $(wildcard *.c) $(wildcard */*.c)
CPPSRC = $(wildcard *.cpp) $(wildcard */*.cpp)
ASRC = $(wildcard *.S)

#	creating a list of all object files (compiled sources, but not linked)
OBJ = $(SRC:%.c=$(OBJECT_DIR)/%.o) $(CPPSRC:%.cpp=$(OBJECT_DIR)/%.o) $(ASRC:%.S=$(OBJECT_DIR)/%.o)
# SYSTEM_OBJECTS = $(ARM_GCC_PATH)/lib/gcc/arm-none-eabi/8.2.1/thumb/v6-m/nofp/crti.o $(ARM_GCC_PATH)/lib/gcc/arm-none-eabi/8.2.1/thumb/v6-m/nofp/crtbegin.o $(ARM_GCC_PATH)/lib/gcc/arm-none-eabi/8.2.1/thumb/v6-m/nofp/crtend.o $(ARM_GCC_PATH)/lib/gcc/arm-none-eabi/8.2.1/thumb/v6-m/nofp/crtn.o $(ARM_GCC_PATH)/arm-none-eabi/lib/thumb/v6-m/nofp/crt0.o

# Info: Prints out a colored string
# Usage: msg string [color] [newline]
#			color = [green]
#			newline = {true,false}
define msg
	@if [ $2 ]; then \
		if [ $2 != true ]; then \
			case "$2" in \
				"green") printf "\033[32;1m$1\033[0m";; \
				*) printf "$1 (Color \"$2\" not implemented)";; \
			esac; \
			if [ -n "$3" ]; then echo; fi \
		else \
			echo $1; \
		fi \
	else \
		printf "$1" ; \
	fi
endef

#	Defining Compiler Tools
PREFIX := $(ARM_GCC_PATH)/bin/arm-none-eabi-
CC := $(PREFIX)gcc
CXX := $(PREFIX)g++
LD := $(PREFIX)ld
GDB := $(PREFIX)gdb
SIZE := $(PREFIX)size
NM := $(PREFIX)nm
RM := rm -f -v
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
OPENOCD := $(OPENOCD_PATH)/bin/openocd

#	compiler flags
WARNINGS = -Wall -Wextra -Wno-unused-parameter
ARCH_FLAGS := -mthumb -mcpu=cortex-m0plus -mfloat-abi=soft
LIBFLAGS = -static $(SHARED_LIBS_PATHS_FLAGS) $(SHARED_LIBS_FLAGS)
CXXFLAGS = -ffreestanding -fdata-sections -ffunction-sections -fno-exceptions -g -O2 $(ARCH_FLAGS) $(INCLUDE_FLAGS) $(WARNINGS) $(DEFINES_FLAGS) -std=c++11 --specs=nano.specs
CFLAGS = -ffreestanding -fdata-sections -ffunction-sections -g -Os $(ARCH_FLAGS) $(INCLUDE_FLAGS) $(WARNINGS) $(DEFINES_FLAGS) --specs=nano.specs
LDFLAGS =  $(LIBFLAGS) --script xdk-asf/flash.ld --demangle -Map $(TARGET).map --cref --gc-sections
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPENDENCIES_DIR)/$*.Td

all: tree $(TARGET).elf executables size
rebuild: clean all

test:
	@echo $(LDFLAGS)

#	Order matters: first put own objects and then the referenced ones (like libraries)
# but runtime error ?! thumb-arm interwork broken ?
# $(TARGET).elf: $(OBJ)
# 	$(call msg,Linking $@,green,true)
# 	$(VERBOSE)$(LD) -o $@ $(OBJ) $(SYSTEM_OBJECTS) $(LDFLAGS)

$(TARGET).elf: $(OBJ)
	$(call msg,Linking $@,green,true)
	$(VERBOSE)$(CXX) -o $@ $(OBJ) $(ARCH_FLAGS) $(CXXFLAGS) -Wl,-static,-L,qtouch,--start-group,--end-group,-l,qtm_acq_saml21_0x0026,-l,qtm_binding_layer_cm0p_0x0005,-l,qtm_touch_key_cm0p_0x0002,--script,xdk-asf/flash.ld,--demangle,-Map,build/saml21-touch.map,--cref,--gc-sections
	
$(OBJECT_DIR)/%.o: %.cpp $(DEPENDENCIES_DIR)/%.d
	$(call msg,Compiling $< and generating Dependencies,green,true)
	$(VERBOSE)$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@ 
ifeq ($(aux),y)
	$(VERBOSE)$(CXX) $(CXXFLAGS) -E $< -o $(OBJECT_DIR)/$*.i
	$(VERBOSE)$(CXX) $(CXXFLAGS) -S $< -o $(OBJECT_DIR)/$*.s
endif
	@mv -f $(DEPENDENCIES_DIR)/$*.Td $(DEPENDENCIES_DIR)/$*.d && touch $@

$(OBJECT_DIR)/%.o: %.c $(DEPENDENCIES_DIR)/%.d
	$(call msg,Compiling $< and generating Dependencies,green,true)
	$(VERBOSE)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@ 
ifeq ($(aux),y)
	$(VERBOSE)$(CC) $(CFLAGS) -E $< -o $(OBJECT_DIR)/$*.i
	$(VERBOSE)$(CC) $(CFLAGS) -S $< -o $(OBJECT_DIR)/$*.s
endif
	@mv -f $(DEPENDENCIES_DIR)/$*.Td $(DEPENDENCIES_DIR)/$*.d && touch $@

$(DEPENDENCIES_DIR)/%.d: ;
.PRECIOUS: $(DEPENDENCIES_DIR)/%.d

-include $(OBJ:%.o=$(DEPENDENCIES_DIR)/%.d)
	

.PHONY: clean tree run debug program server_start server_stop links reset help
run: all program

clean:
	@echo "Removing files:"
	@$(RM) $(TARGET).* 
	@$(RM) $(OBJECT_DIR)/*.*
	@$(RM) $(OBJECT_DIR)/xdk-asf/*.*
	@$(FOLDER:%=$(RM) $(OBJECT_DIR)/%/*.*)
	@$(RM) $(DEPENDENCIES_DIR)/*.*
	@$(FOLDER:%=$(RM) $(DEPENDENCIES_DIR)/%/*.*)

#	create folder structure if not existing
#	"@" in front of line suppresses the output ... I guess
tree:
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir -p $(BUILD_DIR); fi
	@if [ ! -d "$(OBJECT_DIR)/xdk-asf" ]; then mkdir -p $(OBJECT_DIR)/xdk-asf; fi
	@$(FOLDER:%=mkdir -p $(OBJECT_DIR)/%;)
	@if [ ! -d "$(DEPENDENCIES_DIR)/xdk-asf" ]; then mkdir -p $(DEPENDENCIES_DIR)/xdk-asf; fi
	@$(FOLDER:%=mkdir -p $(DEPENDENCIES_DIR)/%;)

#	print final codesize
size: $(TARGET).elf
	$(VERBOSE)$(NM) --numeric-sort --demangle --format=sysv $< > $(TARGET).nm
	$(VERBOSE)$(SIZE) --format=sysv --radix=16 --target=elf32-littlearm $< > $(TARGET).size
	@$(SIZE) $<
	
	
#	create flash files, executable files for flash
executables: $(TARGET).elf
	$(VERBOSE)$(OBJCOPY) -O ihex $< $(TARGET).hex
	$(VERBOSE)$(OBJCOPY) -O binary $< $(TARGET).bin
	@$(OBJDUMP) -h -S $< > $(TARGET).s
	

debug:
	$(GDB) -iex "target extended-remote localhost:3333" -ex load $(TARGET).elf

reset: 
	$(GDB) -iex "target extended-remote localhost:3333" -ex "monitor reset" 


# -iex "target extended-remote localhost:3333": initially connect to target
# -ex load: flash file to mcu
# -ex "monitor reset": reset remote target
# -ex "kill inferiors 1": kill the connection so we can quit without user interaction
# -ex quit: quit gdb
program:
	$(GDB) -iex "target extended-remote localhost:3333" -ex load -ex "monitor reset" -ex "kill inferiors 1" -ex quit $(TARGET).elf

# Execute openocd in the background and redirect all output to /dev/null
# Start only once !!
server_start:
	@echo 'Starting OpenOCD Server'
	@$(OPENOCD) &>/dev/null &

# use = instead of := to make this expand on each read
OPENOCD_PID = $(shell ps | grep openocd | cut -d' ' -f1 | sed 1q)
server_stop:
	@echo 'Stopping OpenOCD Server with pid $(OPENOCD_PID)'
	kill $(OPENOCD_PID)

links:
	@if [ ! -d "xdk-asf" ]; then \
		mkdir -p xdk-asf; \
		ln -s $(ASF_PATH)/thirdparty/CMSIS/Include xdk-asf/cmsis; \
		ln -s $(ASF_PATH)/sam0/utils/cmsis/saml21/include_b xdk-asf/include; \
		ln -s $(ASF_PATH)/sam0/utils/linker_scripts/saml21/gcc/saml21g18b_flash.ld xdk-asf/flash.ld; \
		ln -s $(ASF_PATH)/sam0/utils/cmsis/saml21/source/gcc/startup_saml21.c xdk-asf/ ; \
		ln -s $(ASF_PATH)/sam0/utils/cmsis/saml21/source/system_saml21.c xdk-asf/ ; \
		ln -s $(ASF_PATH)/sam0/utils/cmsis/saml21/source/system_saml21.h xdk-asf/ ; \
		ln -s $(ASF_PATH)/sam0/utils/syscalls/gcc/syscalls.c xdk-asf/;\
	fi

help:
	@echo "Supported commands:"
	@echo "all\t\tBuild project"
	@echo "clean\t\tClean up build directory"
	@echo "run\t\tBuild and flash target"
	@echo "program\t\tFlash target"
	@echo "debug\t\Start gdb, program target and halt debugger"
	@echo "reset\t\tReset target"
	@echo "server_start\tStart GDB Server"
	@echo "server_stop\tStop GDB Server"
	@echo "links\t\tLink project to ASF"
	@echo "tree\t\tCreates folder structure"
	@echo
	@echo "Append \033[32;1maux=y\033[0m to a build target (all, run) to produce auxilliary files. E.g. make all aux=y"
