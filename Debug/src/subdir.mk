################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CS4344.c \
../src/audio.c \
../src/cr_startup_stm32f4xx.c \
../src/main.c \
../src/midi.c \
../src/oscillator.c \
../src/pp6.c \
../src/pwm.c \
../src/sad.c \
../src/sadsr.c \
../src/sequencer.c \
../src/system_stm32f4xx.c \
../src/timer.c \
../src/uart.c 

OBJS += \
./src/CS4344.o \
./src/audio.o \
./src/cr_startup_stm32f4xx.o \
./src/main.o \
./src/midi.o \
./src/oscillator.o \
./src/pp6.o \
./src/pwm.o \
./src/sad.o \
./src/sadsr.o \
./src/sequencer.o \
./src/system_stm32f4xx.o \
./src/timer.o \
./src/uart.o 

C_DEPS += \
./src/CS4344.d \
./src/audio.d \
./src/cr_startup_stm32f4xx.d \
./src/main.d \
./src/midi.d \
./src/oscillator.d \
./src/pp6.d \
./src/pwm.d \
./src/sad.d \
./src/sadsr.d \
./src/sequencer.d \
./src/system_stm32f4xx.d \
./src/timer.d \
./src/uart.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -DSTM32F4XX -DUSE_STDPERIPH_DRIVER -D__USE_CMSIS=CMSISv2p10_STM32F4xx -D__USE_CMSIS_DSPLIB=CMSISv2p10_DSPLIB_CM4 -I"/home/owen/workspace-rs5/StdPeriphLib_v100_STM32F4xx/inc" -I"/home/owen/workspace-rs5/CMSISv2p10_STM32F4xx/inc" -I"/home/owen/workspace-rs5/CMSISv2p10_DSPLIB_CM4/inc" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/cr_startup_stm32f4xx.o: ../src/cr_startup_stm32f4xx.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -DSTM32F4XX -DUSE_STDPERIPH_DRIVER -D__USE_CMSIS=CMSISv2p10_STM32F4xx -D__USE_CMSIS_DSPLIB=CMSISv2p10_DSPLIB_CM4 -I"/home/owen/workspace-rs5/StdPeriphLib_v100_STM32F4xx/inc" -I"/home/owen/workspace-rs5/CMSISv2p10_STM32F4xx/inc" -I"/home/owen/workspace-rs5/CMSISv2p10_DSPLIB_CM4/inc" -Os -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"src/cr_startup_stm32f4xx.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


