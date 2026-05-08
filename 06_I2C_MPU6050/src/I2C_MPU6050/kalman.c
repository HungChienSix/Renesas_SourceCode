#include "kalman.h"
#include "mpu6050.h"
#include <math.h>

void kalman_filter_init(kalman_filter_t *kf)
{
    kf->angle = 0.0f;
    kf->bias  = 0.0f;

    kf->P[0][0] = 1.0f;
    kf->P[0][1] = 0.0f;
    kf->P[1][0] = 0.0f;
    kf->P[1][1] = 1.0f;

    kf->Q_angle   = 0.001f;
    kf->Q_bias    = 0.003f;
    kf->R_measure = 0.03f;
}

float kalman_filter_update(kalman_filter_t *kf, float new_angle, float new_gyro, float dt)
{
    /* 预测: 先验估计 */
    float rate = new_gyro - kf->bias;
    kf->angle += dt * rate;

    /* 预测: 先验误差协方差 */
    kf->P[0][0] += dt * (dt * kf->P[1][1] - kf->P[0][1] - kf->P[1][0] + kf->Q_angle);
    kf->P[0][1] -= dt * kf->P[1][1];
    kf->P[1][0] -= dt * kf->P[1][1];
    kf->P[1][1] += kf->Q_bias * dt;

    /* 更新: 卡尔曼增益 */
    float S = kf->P[0][0] + kf->R_measure;
    float K[2];
    K[0] = kf->P[0][0] / S;
    K[1] = kf->P[1][0] / S;

    /* 更新: 后验估计 */
    float y = new_angle - kf->angle;
    kf->angle += K[0] * y;
    kf->bias  += K[1] * y;

    /* 更新: 后验误差协方差 */
    float P00_temp = kf->P[0][0];
    float P01_temp = kf->P[0][1];
    kf->P[0][0] -= K[0] * P00_temp;
    kf->P[0][1] -= K[0] * P01_temp;
    kf->P[1][0] -= K[1] * P00_temp;
    kf->P[1][1] -= K[1] * P01_temp;

    return kf->angle;
}

/* ---- attitude 封装层 ---- */

static kalman_filter_t kf_roll;
static kalman_filter_t kf_pitch;
static float cur_roll;
static float cur_pitch;

uint8_t Attitude_Init(void)
{
    uint8_t res = MPU_Init();
    if (res != 0)
        return res;

    kalman_filter_init(&kf_roll);
    kalman_filter_init(&kf_pitch);
    cur_roll  = 0.0f;
    cur_pitch = 0.0f;
    return 0;
}

void Attitude_Update(float dt)
{
    short ax, ay, az;
    short gx, gy, gz;

    MPU_Get_Accelerometer(&ax, &ay, &az);
    MPU_Get_Gyroscope(&gx, &gy, &gz);

    float acc_roll  = atan2f((float)ay, (float)az) * 57.29578f;
    float acc_pitch = atan2f(-(float)ax, (float)sqrtf((float)ay * ay + (float)az * az)) * 57.29578f;

    float gyro_roll  = (float)gx / 16.4f;
    float gyro_pitch = (float)gy / 16.4f;

    cur_roll  = kalman_filter_update(&kf_roll,  acc_roll,  gyro_roll,  dt);
    cur_pitch = kalman_filter_update(&kf_pitch, acc_pitch, gyro_pitch, dt);
}

void Attitude_Get(float *roll, float *pitch)
{
    *roll  = cur_roll;
    *pitch = cur_pitch;
}
