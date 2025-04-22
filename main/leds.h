#ifndef LEDS_H
#define LEDS_H

#include "driver/gpio.h"

#define LED_OFF 0
#define RGB_ROJO GPIO_NUM_1
#define RGB_VERDE GPIO_NUM_8
#define RGB_AZUL GPIO_NUM_10

void PrenderLedVerde(bool estadoLed);
void PrenderLedRojo(bool estadoLed);
void PrenderLedAzul(bool estadoLed);
void ConfigurarSalidasLed(void);

#endif // LEDS_H
