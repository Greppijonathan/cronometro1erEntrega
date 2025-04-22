#include "leds.h"

void PrenderLedVerde(bool estadoLed)
{
    gpio_set_level(RGB_VERDE, estadoLed);
    gpio_set_level(RGB_ROJO, LED_OFF);
    gpio_set_level(RGB_AZUL, LED_OFF);
}

void PrenderLedRojo(bool estadoLed)
{
    gpio_set_level(RGB_VERDE, LED_OFF);
    gpio_set_level(RGB_ROJO, estadoLed);
    gpio_set_level(RGB_AZUL, LED_OFF);
}

void PrenderLedAzul(bool estadoLed)
{
    gpio_set_level(RGB_VERDE, LED_OFF);
    gpio_set_level(RGB_ROJO, LED_OFF);
    gpio_set_level(RGB_AZUL, estadoLed);
}

void ConfigurarSalidasLed(void)
{
    gpio_set_direction(RGB_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_AZUL, GPIO_MODE_OUTPUT);
}