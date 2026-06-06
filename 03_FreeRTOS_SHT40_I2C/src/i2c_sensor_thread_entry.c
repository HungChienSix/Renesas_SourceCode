#include "i2c_sensor_thread.h"
#include "UART_debug/uart_debug.h"
#include "BMP280/bmp280.h"
#include "MAX30102/max30102.h"
#include "MAX30102/max30102_algo.h"
#include "SHT40/sht40.h"             /* 三个传感器共用 i2c_port */
#include <stdio.h>
#include <stdlib.h>  // abs()

/* I2C 传感器线程: 周期读 SHT40 + BMP280 + MAX30102,跑算法, UART 打印 */
/* pvParameters contains TaskHandle_t */
void i2c_sensor_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    printf("[I2C] sensor thread started\n");

    /* 关键: 先开 I2C 总线, 再 scan 和 init 各 sensor
     * 否则 scan 和 BMP280/MAX30102 Init 都会因总线未开而失败 */
    if (i2c_port_init() != 0) {
        printf("[I2C] port init failed (busy / wiring issue), will retry per device\n");
    }

    /* 启动时扫一遍 I2C 总线,确认设备挂载情况 */
    i2c_port_scan();

    fsp_err_t err;

    /* BMP280 */
    err = BMP280_Init();
    printf("BMP280 init: %s\n", (err == FSP_SUCCESS) ? "OK" : "FAIL");
    if (err == FSP_SUCCESS) BMP280_DebugDump();

    /* SHT40 */
    err = SHT40_Init();
    printf("SHT40  init: %s\n", (err == FSP_SUCCESS) ? "OK" : "FAIL");

    /* MAX30102 + 算法 */
    err = MAX30102_Init();
    printf("MAX30102 init: %s\n", (err == FSP_SUCCESS) ? "OK" : "FAIL");
    if (err == FSP_SUCCESS) {
        MAX30102_DebugDump();
        MAX30102_AlgoInit();   /* 清算法 buffer,准备接收样本 */
    }

    /* 缓存 */
    MAX30102_Sample_t ppg[MAX30102_FIFO_DEPTH];
    uint8_t ppg_n = 0;
    MAX30102_Vitals_t vitals = {0};

    while (1)
    {
        /* ---- 1) BMP280 ---- */
        BMP280_Data_t bmp; // 温度 压力 海拔
        if (BMP280_ReadForced(&bmp) == BMP280_OK) {
            // printf("BMP280 -> T: %d.%02d C,  P: %u Pa,  Alt: %d.%02d m  (sea=%u Pa)\n",
            //        bmp.temperature_centi_c / 100, abs(bmp.temperature_centi_c % 100),
            //        bmp.pressure_pa,
            //        bmp.altitude_cm / 100, abs(bmp.altitude_cm % 100),
            //        BMP280_GetSeaLevelPa());
        } else {
            printf("BMP280 read error\n");
        }

        /* ---- 2) SHT40 + 露点 ---- */
        SHT40_Data_t sensor;
        if (SHT40_ReadHighPrecision(&sensor) == SHT40_OK) {
            int t_x100  = (int)(sensor.temperature_c * 100.0f); // 温度
            int rh_x100 = (int)(sensor.humidity_pct  * 100.0f); // 湿度
            int dp_x100 = (int)(SHT40_ConvertDewPoint(sensor.temperature_c, sensor.humidity_pct) * 100.0f); // 露点 小于这个温度结露
//            printf("SHT40  -> T: %d.%02d C,  RH: %d.%02d %%,  DewPt: %d.%02d C\n",
//                   t_x100 / 100,  abs(t_x100  % 100),
//                   rh_x100 / 100, abs(rh_x100 % 100),
//                   dp_x100 / 100, abs(dp_x100 % 100));
        } else {
            printf("SHT40 read error\n");
        }

        /* ---- 3) MAX30102:读 FIFO + 喂算法 + 取 vitals ---- */
        if (MAX30102_ReadFIFO(ppg, MAX30102_FIFO_DEPTH, &ppg_n) == MAX30102_OK) {
            if (ppg_n > 0) {
                /* 喂每个样本到算法 */
                for (uint8_t i = 0; i < ppg_n; i++) {
                    MAX30102_AlgoPushSample(ppg[i].red, ppg[i].ir);
                }

                /* 算 vitals */
                MAX30102_AlgoGetVitals(&vitals);

                if (vitals.finger_on) {
                    /* R = r_ratio * 1000,拆成 整数.小数 打印(避免 %f) */
                    int r_x1000 = (int)(vitals.r_ratio * 1000.0f);
                   printf("MAX30102 -> %u smp  HR: %d bpm  SpO2: %d %%  R=%d.%03d\n",
                          ppg_n,
                          (int)(vitals.bpm + 0.5f),
                          (int)(vitals.spo2 + 0.5f),
                          r_x1000 / 1000, abs(r_x1000 % 1000));
                } else {
                    /* 没贴手指,只报样本数,提示等手指 */
                   printf("MAX30102 -> %u smp  (no finger / wait...)\n", ppg_n);
                }
            }
            /* ppg_n == 0: FIFO 空,不打 */
        } else {
            printf("MAX30102 read error\n");
        }

        vTaskDelay(100);
    }
}
