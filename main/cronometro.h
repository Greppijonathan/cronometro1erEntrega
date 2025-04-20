#ifndef CRONOMETRO_H
#define CRONOMETRO_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct
{
    int decenasMinutos;
    int unidadesMinutos;
    int decenasSegundos;
    int unidadesSegundos;
    int decimasSegundo;
} DigitosCronometro;

extern DigitosCronometro digitosActuales;
extern SemaphoreHandle_t semaforoAccesoDigitos;

void ActualizarDigitos(void);
void ReiniciarCronometro(void);

#endif // CRONOMETRO_H
