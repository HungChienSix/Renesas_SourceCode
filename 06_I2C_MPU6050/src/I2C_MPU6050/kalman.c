#include "kalman.h"

void kalman_init(kalman_t *kf)
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

float kalman_update(kalman_t *kf, float new_angle, float new_gyro, float dt)
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
