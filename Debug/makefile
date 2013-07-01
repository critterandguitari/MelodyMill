################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ToneMill.axf

# Tool invocations
ToneMill.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/owen/workspace-rs5/StdPeriphLib_v100_STM32F4xx/Debug" -L"/home/owen/workspace-rs5/CMSISv2p10_STM32F4xx/Debug" -L"/home/owen/workspace-rs5/CMSISv2p10_DSPLIB_CM4/lib" -Xlinker -Map="ToneMill.map" -Xlinker --gc-sections -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -T "ToneMill_Debug.ld" -o "ToneMill.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) ToneMill.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "ToneMill.axf"; # arm-none-eabi-objcopy -O binary "ToneMill.axf" "ToneMill.bin" ; checksum -p STM32F407VE -d "ToneMill.bin";
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
