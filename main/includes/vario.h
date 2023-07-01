#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "core2forAWS.h"

void vario_set_speed(int32_t speed/*cm per second*/);
void vario_start(void);
void vario_stop(void);
void vario_qmp6988_loop(void * argument);
void vario_sht3x_loop(void * argument);
void vario_speaker_loop(void * arguemnt);
void vario_dps310_loop(void * arguments);
void vario_qmc5883l_loop(void * arguments);