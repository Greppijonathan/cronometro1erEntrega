#include "cronometro.h"

DigitosCronometro digitosActuales = {0, 0, 0, 0, 0};
SemaphoreHandle_t semaforoAccesoDigitos = NULL; // El semaforo se crea en el main

/**
 * @brief Incrementa en 1 el valor actual del cronometro
 *
 * El cronometro se incrementa en una decima de segundo, si se llena el dígito de decimas de segundo
 * se incrementa en 1 el dígito de unidades de segundo, y así sucesivamente. Si se llena el dígito
 * de decenas de minutos, se reinicia el cronometro a 0.
 *
 * La función utiliza un semaforo para proteger el acceso a los digitos del cronometro.
 */
void ActualizarDigitos(void)
{
    if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Se toma el recurso, se señaliza en el mutex
    {
        digitosActuales.decimasSegundo = (digitosActuales.decimasSegundo + 1) % 10;

        if (digitosActuales.decimasSegundo == 0)
        {
            digitosActuales.unidadesSegundos = (digitosActuales.unidadesSegundos + 1) % 10;
            if (digitosActuales.unidadesSegundos == 0)
            {
                digitosActuales.decenasSegundos = (digitosActuales.decenasSegundos + 1) % 6;
                if (digitosActuales.decenasSegundos == 0)
                {
                    digitosActuales.unidadesMinutos = (digitosActuales.unidadesMinutos + 1) % 10;
                    if (digitosActuales.unidadesMinutos == 0)
                    {
                        digitosActuales.decenasMinutos = (digitosActuales.decenasMinutos + 1) % 6;
                    }
                }
            }
        }
        xSemaphoreGive(semaforoAccesoDigitos); // Se libera el recurso, se libera el mutex
    }
}

/**
 * @brief Reinicia el cronometro a 0
 *
 * La función utiliza un semaforo para proteger el acceso a los digitos del cronometro.
 * El cronometro se reinicia a 0 en todos sus digitos.
 */
void ReiniciarCronometro(void)
{
    if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY))
    {
        digitosActuales = (DigitosCronometro){0, 0, 0, 0, 0}; // Reinicio de los digitos
        xSemaphoreGive(semaforoAccesoDigitos);
    }
}
