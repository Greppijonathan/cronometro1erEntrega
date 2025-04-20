#ifndef TECLAS_H
#define TECLAS_H

#include "driver/gpio.h"

#define TEC1_Pausa GPIO_NUM_4
#define TEC2_Reiniciar GPIO_NUM_6

void ConfigurarTeclas(void);

#endif // TECLAS_H
