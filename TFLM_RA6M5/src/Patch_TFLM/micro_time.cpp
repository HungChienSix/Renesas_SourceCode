// micro_time.cpp — 使用DWT周期计数器为TFLM提供微秒级计时
#include "tensorflow/lite/micro/micro_time.h"
#include "sys_time/sys_time.h"

uint32_t tflite::GetCurrentTimeTicks() {
    return SysTime_Get_us();
}

uint32_t tflite::ticks_per_second() {
    return 1000000;
}
