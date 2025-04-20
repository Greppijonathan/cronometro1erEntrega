
/*

*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ili9341.h"
#include "digitos.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "cronometro.h"
#include "leds.h"
#include "teclas.h"

// Definiciones generales
#define DIGITO_ANCHO 50
#define DIGITO_ALTO 90
#define DIGITO_ENCENDIDO ILI9341_RED
#define DIGITO_APAGADO 0x3800
#define DIGITO_FONDO ILI9341_BLACK

#define RGB_ROJO GPIO_NUM_1
#define RGB_VERDE GPIO_NUM_8
#define RGB_AZUL GPIO_NUM_10
#define LED_OFF 0

#define TEC1_Pausa GPIO_NUM_4
#define TEC2_Reiniciar GPIO_NUM_6

// Estados del cronómetro
typedef enum
{
    ESTADO_CORRIENDO,
    ESTADO_PAUSADO,
    ESTADO_REINICIAR
} EstadoCronometro;

volatile EstadoCronometro estadoActual = ESTADO_PAUSADO; // Se inicializa en pausa por defecto

// SemaphoreHandle_t semaforoAccesoDigitos;

/*****************************************CODIGO PARA LAS TAREAS*****************************************/

void ManejarEstadoCronometro(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bool estadoLed = true;
    while (1)
    {
        switch (estadoActual)
        {
        case ESTADO_CORRIENDO:
            ActualizarDigitos();
            PrenderLedVerde(estadoLed);
            break;

        case ESTADO_PAUSADO:
            PrenderLedRojo(estadoLed);
            break;

        case ESTADO_REINICIAR:
            PrenderLedRojo(estadoLed);
            ReiniciarCronometro();
            estadoActual = ESTADO_PAUSADO; // Luego de un reinicio se deja por defecto el creonometro pausado
            break;
        }
        estadoLed = !estadoLed;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}
void EscanearPulsadores(void *parametros)
{
    bool estadoAnteriorTEC1 = true;
    bool estadoAnteriorTEC2 = true;

    while (1)
    {
        bool estadoActualTEC1 = gpio_get_level(TEC1_Pausa);
        if (!estadoActualTEC1 && estadoAnteriorTEC1)
        {
            estadoActual = (estadoActual == ESTADO_CORRIENDO) ? ESTADO_PAUSADO : ESTADO_CORRIENDO;
        }
        estadoAnteriorTEC1 = estadoActualTEC1;

        bool estadoActualTEC2 = gpio_get_level(TEC2_Reiniciar);
        if (!estadoActualTEC2 && estadoAnteriorTEC2)
        {
            if (estadoActual == ESTADO_PAUSADO)
            {
                estadoActual = ESTADO_REINICIAR;
            }
        }
        estadoAnteriorTEC2 = estadoActualTEC2;

        vTaskDelay(pdMS_TO_TICKS(50)); // Manejo del rebote
    }
}

void ActualizarPantalla(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    // Crear panel para minutos y segundos (4 dígitos)
    panel_t PanelMinutosSegundos = CrearPanel(30, 70, 4, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    // Crear panel para décimas de segundo (1 dígito)
    panel_t PanelDecimas = CrearPanel(240, 70, 1, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    struct digitos_previos
    {
        int decenasMinutosAnterior;
        int unidadesMinutosAnterior;
        int decenasSegundosAnterior;
        int unidadesSegundosAnterior;
        int decimasSegundosAnterior;
    } digitosPrevios = {-1, -1, -1, -1, -1}; // Inicializamos con valores "inválidos"

    while (1)
    {
        if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Se accede al recurso compartido, se toma el semáforo
        {
            // Lógica para actualizar el panel de minutos y segundos
            if (digitosActuales.decenasMinutos != digitosPrevios.decenasMinutosAnterior)
            {
                DibujarDigito(PanelMinutosSegundos, 0, digitosActuales.decenasMinutos);
                digitosPrevios.decenasMinutosAnterior = digitosActuales.decenasMinutos;
            }
            if (digitosActuales.unidadesMinutos != digitosPrevios.unidadesMinutosAnterior)
            {
                DibujarDigito(PanelMinutosSegundos, 1, digitosActuales.unidadesMinutos);
                digitosPrevios.unidadesMinutosAnterior = digitosActuales.unidadesMinutos;
            }
            if (digitosActuales.decenasSegundos != digitosPrevios.decenasSegundosAnterior)
            {
                DibujarDigito(PanelMinutosSegundos, 2, digitosActuales.decenasSegundos);
                digitosPrevios.decenasSegundosAnterior = digitosActuales.decenasSegundos;
            }
            if (digitosActuales.unidadesSegundos != digitosPrevios.unidadesSegundosAnterior)
            {
                DibujarDigito(PanelMinutosSegundos, 3, digitosActuales.unidadesSegundos);
                digitosPrevios.unidadesSegundosAnterior = digitosActuales.unidadesSegundos;
            }

            // Lógica para actualizar el panel de décimas de segundo
            if (digitosActuales.decimasSegundo != digitosPrevios.decimasSegundosAnterior)
            {
                DibujarDigito(PanelDecimas, 0, digitosActuales.decimasSegundo);
                digitosPrevios.decimasSegundosAnterior = digitosActuales.decimasSegundo;
            }

            xSemaphoreGive(semaforoAccesoDigitos); // Se libera el recurso compartido, se libera el semáforo
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50)); // Tiempo de actualización de pantalla
    }
}

/***************************************** app_main()*****************************************/
void app_main(void)
{
    // Se crea el semaforo que se va a utilizar en las diferentes zonas del codigo
    semaforoAccesoDigitos = xSemaphoreCreateMutex();
    // configuraciones iniciales
    ConfigurarSalidasLed();
    ConfigurarTeclas();
    // MensajeInicio();
    //  Se crean las tareas

    xTaskCreate(ManejarEstadoCronometro, "ManejarEstadoCronometro", 2048, NULL, 2, NULL);
    xTaskCreate(EscanearPulsadores, "EscanearPulsadores", 1024, NULL, 1, NULL);
    xTaskCreate(ActualizarPantalla, "ActualizarPantalla", 4096, NULL, 1, NULL);
}