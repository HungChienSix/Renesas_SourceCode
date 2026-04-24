#ifndef KALMAN_H
#define KALMAN_H

#include <stdint.h>

typedef struct {
    float angle;        // 卡尔曼滤波后的角度
    float bias;         // 陀螺仪零偏估计
    float P[2][2];      // 误差协方差矩阵
    float Q_angle;      // 角度过程噪声协方差
    float Q_bias;       // 零偏过程噪声协方差
    float R_measure;    // 测量噪声协方差
} kalman_t;

void kalman_init(kalman_t *kf);
float kalman_update(kalman_t *kf, float new_angle, float new_gyro, float dt);

#endif /* KALMAN_H */
