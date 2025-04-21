#ifndef CRONOMETRO_H
#define CRONOMETRO_H

#include "freertos/FreeRTOS.h"

typedef struct
{
    int decenasMinutos;
    int unidadesMinutos;
    int decenasSegundos;
    int unidadesSegundos;
    int decimasSegundo;
} digitos_t;

extern digitos_t digitosActuales;

void ActualizarCronometro(void);

#endif // CRONOMETRO_H
