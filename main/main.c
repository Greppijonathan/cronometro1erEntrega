/*

*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "ili9341.h"
#include "digitos.h"
#include "teclasconfig.h"
#include "leds.h"
#include "cronometro.h"

#define DIGITO_ANCHO 50
#define DIGITO_ALTO 90
#define DIGITO_ENCENDIDO ILI9341_RED
#define DIGITO_APAGADO 0x3800
#define DIGITO_FONDO ILI9341_BLACK

QueueHandle_t colaDigitos;
QueueHandle_t colaEstadosCronometro;
SemaphoreHandle_t semaforoAccesoDigitos;

bool enPausa = true;
bool tiempoParcial = false;

digitos_t digitosParciales = {0, 0, 0, 0, 0};
// Estados del cronometro para trabajar con la cola
typedef enum
{
    PAUSAR,
    REINICIAR,
    PARCIAL
} estadosCronometro_t;

/**
 * @brief Función que se encarga de leer el estado de los botones
 *        y enviar eventos a la cola de eventos del cronometro
 *        dependiendo del estado de los botones
 *
 * @param p Puntero a un void, no se utiliza
 *
 *
 * @return Nada
 */
void leerBotones(void *p)
{
    ConfigurarTeclas();

    bool estadoanteriorPausa = true;
    bool estadoanteriorReiniciar = true;
    bool estaoanteriorTiempoParcial = true;

    while (1)
    {
        if ((gpio_get_level(TEC1_Pausa) == 0) && estadoanteriorPausa)
        {
            estadosCronometro_t evento = PAUSAR;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            estadoanteriorPausa = false;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (gpio_get_level(TEC1_Pausa) != 0)
        {
            estadoanteriorPausa = true;
        }

        if ((gpio_get_level(TEC2_Reiniciar) == 0) && estadoanteriorReiniciar)
        {
            estadosCronometro_t evento = REINICIAR;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            estadoanteriorReiniciar = false;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (gpio_get_level(TEC2_Reiniciar) != 0)
        {
            estadoanteriorReiniciar = true;
        }
        if ((gpio_get_level(TEC3_Parcial) == 0) && estaoanteriorTiempoParcial)
        {
            estadosCronometro_t evento = PARCIAL;
            xQueueSend(colaEstadosCronometro, &evento, portMAX_DELAY);
            estaoanteriorTiempoParcial = false;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (gpio_get_level(TEC3_Parcial) != 0)
        {
            estaoanteriorTiempoParcial = true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Tarea para manejar los estados del cronometro. Recibe la cola que se va llenando con la tarea del teclado
 * modifica banderas globales, que indican en que estado esta el cronometro (luego las uso para hacer parapadear el led RGB)
 * Utiliza  "colaDigitos" para enviar el valor a la tarea que actualiza pantalla
 * @param p Pointer to a void, not used.
 */

void manejoEstadosCronometro(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        estadosCronometro_t eventoRecibido;
        if (xQueueReceive(colaEstadosCronometro, &eventoRecibido, 0))
        {
            if (eventoRecibido == PAUSAR)
            {
                enPausa = !enPausa;
            }
            if (eventoRecibido == REINICIAR)
            {
                digitosActuales = (digitos_t){0, 0, 0, 0, 0}; // reinicio la cuenta
                enPausa = true;
                tiempoParcial = false;
                xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);
            }
            if (eventoRecibido == PARCIAL)
            {
                if (!tiempoParcial)
                {
                    digitosParciales = digitosActuales; // Asigno los digitos actuales a los parciales para mostrar en pantalla el resultado parcial
                }
                tiempoParcial = !tiempoParcial;
            }
        }
        if (!enPausa)
        {
            ActualizarCronometro(); // Logica para incrementar los digitos del cronometro
            xQueueSend(colaDigitos, &digitosActuales, portMAX_DELAY);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Parpadea el led RGB en diferentes colores, segun en que estado este el cronometro.
 *
 */
void manejoLedRGB(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bool estadoLed = false;
    ConfigurarSalidasLed();

    while (1)
    {
        if (tiempoParcial)
        {
            PrenderLedAzul(estadoLed);
        }
        else if (enPausa)
        {
            PrenderLedRojo(estadoLed);
        }
        else
        {
            PrenderLedVerde(estadoLed);
        }
        estadoLed = !estadoLed;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Inicializa pantalla , crea los paneles y recibe la cola con los digitos actualizados
 * Tambien compara cual fue el digito que cambio y lo actualiza en la pantalla
 * @param p Pointer to a void, not used.
 */
void actualizarPantalla(void *p)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    panel_t PanelMinutosSegundos = CrearPanel(30, 70, 4, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t PanelDecimas = CrearPanel(240, 70, 1, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    DibujarDigito(PanelMinutosSegundos, 0, 0);
    DibujarDigito(PanelMinutosSegundos, 1, 0);
    DibujarDigito(PanelMinutosSegundos, 2, 0);
    DibujarDigito(PanelMinutosSegundos, 3, 0);
    DibujarDigito(PanelDecimas, 0, 0);

    digitos_t digitosPrevios = {-1, -1, -1, -1, -1};

    while (1)
    {
        if (xQueueReceive(colaDigitos, &digitosActuales, portMAX_DELAY))
        {
            if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY))
            {
                digitos_t *digitosAMostrar = tiempoParcial ? &digitosParciales : &digitosActuales;

                if (digitosAMostrar->decenasMinutos != digitosPrevios.decenasMinutos)
                    DibujarDigito(PanelMinutosSegundos, 0, digitosAMostrar->decenasMinutos);

                if (digitosAMostrar->unidadesMinutos != digitosPrevios.unidadesMinutos)
                    DibujarDigito(PanelMinutosSegundos, 1, digitosAMostrar->unidadesMinutos);

                if (digitosAMostrar->decenasSegundos != digitosPrevios.decenasSegundos)
                    DibujarDigito(PanelMinutosSegundos, 2, digitosAMostrar->decenasSegundos);

                if (digitosAMostrar->unidadesSegundos != digitosPrevios.unidadesSegundos)
                    DibujarDigito(PanelMinutosSegundos, 3, digitosAMostrar->unidadesSegundos);

                if (digitosAMostrar->decimasSegundo != digitosPrevios.decimasSegundo)
                    DibujarDigito(PanelDecimas, 0, digitosAMostrar->decimasSegundo);

                digitosPrevios = *digitosAMostrar;
                xSemaphoreGive(semaforoAccesoDigitos);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

void app_main()
{
    colaDigitos = xQueueCreate(5, sizeof(digitos_t));
    colaEstadosCronometro = xQueueCreate(5, sizeof(estadosCronometro_t));
    semaforoAccesoDigitos = xSemaphoreCreateMutex();

    xTaskCreate(leerBotones, "LecturaBotonera", 2048, NULL, 1, NULL);
    xTaskCreate(manejoEstadosCronometro, "Tiempo100ms", 2048, NULL, 2, NULL);
    xTaskCreate(actualizarPantalla, "ActualizarPantalla", 4096, NULL, 3, NULL);
    xTaskCreate(manejoLedRGB, "LedsTestigos", 4096, NULL, 1, NULL);
}