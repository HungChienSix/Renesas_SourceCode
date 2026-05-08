#ifndef KALMAN_H
#define KALMAN_H

#include <stdint.h>

typedef struct {
    float angle;
    float bias;
    float P[2][2];
    float Q_angle;
    float Q_bias;
    float R_measure;
} kalman_filter_t;

void kalman_filter_init(kalman_filter_t *kf);
float kalman_filter_update(kalman_filter_t *kf, float new_angle, float new_gyro, float dt);

uint8_t Attitude_Init(void);
void Attitude_Update(float dt);
void Attitude_Get(float *roll, float *pitch);

#endif /* KALMAN_H */
