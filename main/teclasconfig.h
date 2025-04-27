#ifndef TECLASCONFIG_H
#define TECLASCONFIG_H

#include "driver/gpio.h"
#include <stddef.h>

#define TEC1_Pausa GPIO_NUM_4
#define TEC2_Reiniciar GPIO_NUM_6
#define TEC3_Parcial GPIO_NUM_2

typedef struct
{
    gpio_num_t pin;
    gpio_mode_t mode;
    gpio_pull_mode_t pull;
} configuracion_pin_t;

void ConfigurarTeclas(configuracion_pin_t *config, size_t num_pines);

#endif // TECLASCONFIG_H
