#include "cronometro.h"

digitos_t digitosActuales = {0, 0, 0, 0, 0};

void ActualizarCronometro(void)
{
    digitosActuales.decimasSegundo++;

    if (digitosActuales.decimasSegundo == 10)
    {
        digitosActuales.decimasSegundo = 0;
        digitosActuales.unidadesSegundos++;

        if (digitosActuales.unidadesSegundos == 10)
        {
            digitosActuales.unidadesSegundos = 0;
            digitosActuales.decenasSegundos++;

            if (digitosActuales.decenasSegundos == 6)
            {
                digitosActuales.decenasSegundos = 0;
                digitosActuales.unidadesMinutos++;

                if (digitosActuales.unidadesMinutos == 10)
                {
                    digitosActuales.unidadesMinutos = 0;
                    digitosActuales.decenasMinutos++;
                }
            }
        }
    }

    // Reinicio del cronómetro cuando se llega a 59:59.9
    if (digitosActuales.decenasMinutos == 5 && digitosActuales.unidadesMinutos == 9 &&
        digitosActuales.decenasSegundos == 5 && digitosActuales.unidadesSegundos == 9 &&
        digitosActuales.decimasSegundo == 9)
    {
        digitosActuales = (digitos_t){0, 0, 0, 0, 0};
    }
}
