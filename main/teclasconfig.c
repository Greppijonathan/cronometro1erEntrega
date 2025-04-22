#include "teclasconfig.h"

void ConfigurarTeclas(void)
{
    gpio_set_direction(TEC1_Pausa, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC1_Pausa, GPIO_PULLUP_ONLY);
    gpio_set_direction(TEC2_Reiniciar, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC2_Reiniciar, GPIO_PULLUP_ONLY);
    gpio_set_direction(TEC3_Parcial, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC3_Parcial, GPIO_PULLUP_ONLY);
}
