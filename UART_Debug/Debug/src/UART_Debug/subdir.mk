################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/UART_Debug/uart_debug.c 

C_DEPS += \
./src/UART_Debug/uart_debug.d 

OBJS += \
./src/UART_Debug/uart_debug.o 

SREC += \
UART_Debug.srec 

MAP += \
UART_Debug.map 


# Each subdirectory must supply rules for building sources it contributes
src/UART_Debug/%.o: ../src/UART_Debug/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra_gen" -I"." -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra_cfg/fsp_cfg/bsp" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra_cfg/fsp_cfg" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/src" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra/fsp/inc" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra/fsp/inc/api" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra/fsp/inc/instances" -I"D:/Git_Repo/Renesas_Repo/UART_Debug/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -Wno-stringop-overflow -Wno-format-truncation --param=min-pagesize=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

