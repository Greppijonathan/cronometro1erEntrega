#include "cronometro.h"

digitos_t digitosActuales = {0, 0, 0, 0, 0};
digitos_t digitosParciales = {0, 0, 0, 0, 0};

void ActualizarCronometro(digitos_t *digitos)
{
    digitos->decimasSegundo++;

    if (digitos->decimasSegundo == 10)
    {
        digitos->decimasSegundo = 0;
        digitos->unidadesSegundos++;

        if (digitos->unidadesSegundos == 10)
        {
            digitos->unidadesSegundos = 0;
            digitos->decenasSegundos++;

            if (digitos->decenasSegundos == 6)
            {
                digitos->decenasSegundos = 0;
                digitos->unidadesMinutos++;

                if (digitos->unidadesMinutos == 10)
                {
                    digitos->unidadesMinutos = 0;
                    digitos->decenasMinutos++;
                }
            }
        }
    }
    if (digitos->decenasMinutos == 5 && digitos->unidadesMinutos == 9 &&
        digitos->decenasSegundos == 5 && digitos->unidadesSegundos == 9 &&
        digitos->decimasSegundo == 9)
    {
        *digitos = (digitos_t){0, 0, 0, 0, 0};
    }
}
