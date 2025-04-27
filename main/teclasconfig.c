#include "teclasconfig.h"

void ConfigurarTeclas(configuracion_pin_t *config, size_t num_pines)
{
    for (size_t i = 0; i < num_pines; i++)
    {
        gpio_set_direction(config[i].pin, config[i].mode);
        gpio_set_pull_mode(config[i].pin, config[i].pull);
    }
}
