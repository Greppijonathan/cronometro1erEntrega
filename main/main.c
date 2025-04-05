
/*
Implementar un cronómetro utilizando más de una tarea de FreeRTOS que muestre en pantalla el valor de la cuenta actual
con una resolución de décimas de segundo.El cronómetro debe iniciar y detener la cuenta al presionar un pulsador conectado
a la placa. Si está detenido, al presionar un segundo contador debe volver a cero.Mientras la cuenta está activa un led RGB
debe parpadear en verde y cuando está detenida debe permanecer en rojo.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ili9341.h"
#include "digitos.h"
#include "stdio.h"
#include "driver/gpio.h"

// Definiciones para el manejo de los dígitos de la pantalla TFT
#define DIGITO_ANCHO 60
#define DIGITO_ALTO 100
#define DIGITO_ENCENDIDO ILI9341_BLUE2
#define DIGITO_APAGADO 0x3800
#define DIGITO_FONDO ILI9341_BLACK

// Pines del LED RGB
#define RGB_ROJO GPIO_NUM_1
#define RGB_VERDE GPIO_NUM_8
#define RGB_AZUL GPIO_NUM_10

// Pines conectados a pulsadores
#define TEC1_Pausa GPIO_NUM_4
#define TEC2_Reiniciar GPIO_NUM_6

// Estructura para el manejo de los dígitos del cronómetro
struct digitosCronometro
{
    int unidadesSegundos;
    int decenasSegundos;
    int unidadesMinutos;
    int decenasMinutos;
} *digitosActuales_t;

// Semáforo para proteger el acceso a los dígitos, es el recuerso compartido
SemaphoreHandle_t semaforoAccesoDigitos;

// Variables globales para el control del cronómetro y LED
volatile bool pausaCronometro = false;
volatile bool reiniciarCuenta = false;

void ControlTiempo(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount(); // Inicialización para vTaskDelayUntil
    bool estadoLed = false;
    // Lógica para el manejo de los dígitos del cronómetro
    while (1)
    {
        if (!pausaCronometro)
        {
            if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Acceso al recurso compartido
            {

                digitosActuales_t->unidadesSegundos = (digitosActuales_t->unidadesSegundos + 1) % 10;

                if (digitosActuales_t->unidadesSegundos == 0)
                {
                    digitosActuales_t->decenasSegundos = (digitosActuales_t->decenasSegundos + 1) % 6;

                    if (digitosActuales_t->decenasSegundos == 0)
                    {

                        digitosActuales_t->unidadesMinutos = (digitosActuales_t->unidadesMinutos + 1) % 10;

                        if (digitosActuales_t->unidadesMinutos == 0)
                        {
                            digitosActuales_t->decenasMinutos = (digitosActuales_t->decenasMinutos + 1) % 6;
                        }
                    }
                }
                xSemaphoreGive(semaforoAccesoDigitos); // Liberar el recurso compartido
            }
        }
        else if (reiniciarCuenta)
        {
            if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Acceso al recurso nuevamente
            {
                // Reiniciar los valores del cronómetro
                digitosActuales_t->unidadesSegundos = 0;
                digitosActuales_t->decenasSegundos = 0;
                digitosActuales_t->unidadesMinutos = 0;
                digitosActuales_t->decenasMinutos = 0;
                xSemaphoreGive(semaforoAccesoDigitos); // Liberar el recurso compartido
            }
            reiniciarCuenta = false; // Limpiar bandera
        }
        if (!pausaCronometro)
        {
            gpio_set_level(RGB_VERDE, estadoLed);
            gpio_set_level(RGB_ROJO, 0);
        }
        if (pausaCronometro)
        {
            gpio_set_level(RGB_ROJO, estadoLed);
            gpio_set_level(RGB_VERDE, 0);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000)); // Espero 1 s para los incrementos
        estadoLed = !estadoLed;
    }
}

void EscanearPulsadores(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount(); // Inicialización para vTaskDelayUntil
    bool estadoAnteriorPulsadorTEC1_Pausa = true;
    bool estadoAnteriorPulsadorTEC2_Reiniciar = true;
    // Escaneo del teclado
    while (1)
    {
        bool estadoActualTEC1_Pausa = gpio_get_level(TEC1_Pausa);
        if (!estadoActualTEC1_Pausa && estadoAnteriorPulsadorTEC1_Pausa)
        {
            pausaCronometro = !pausaCronometro;
        }
        estadoAnteriorPulsadorTEC1_Pausa = estadoActualTEC1_Pausa;

        bool estadoActualTEC2_Reiniciar = gpio_get_level(TEC2_Reiniciar);
        if (!estadoActualTEC2_Reiniciar && estadoAnteriorPulsadorTEC2_Reiniciar)
        {
            if (pausaCronometro)
            {
                reiniciarCuenta = true;
            }
        }
        estadoAnteriorPulsadorTEC2_Reiniciar = estadoActualTEC2_Reiniciar;

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

void ActualizarPantalla(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount(); // Inicialización para vTaskDelayUntil
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    panel_t horas = CrearPanel(30, 60, 2, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_t minutos = CrearPanel(170, 60, 2, DIGITO_ALTO, DIGITO_ANCHO, DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    struct digitos_previos
    {
        int decenasAnterior;
        int unidadesAnterior;
        int decenasSegundosAnterior;
        int unidadesSegundosAnterior;
    }

    digitosPrevios = {-1, -1, -1, -1}; // Valores iniciales incorrectos para garantizar que se inicialice la cuenta

    while (1)
    {
        if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Verifico disponibilidad del recuerso compartido
        {                                                         // Logica para solamente actualizar el digito que cambio
            if (digitosActuales_t->decenasMinutos != digitosPrevios.decenasAnterior)
            {
                DibujarDigito(horas, 0, digitosActuales_t->decenasMinutos);
                digitosPrevios.decenasAnterior = digitosActuales_t->decenasMinutos;
            }
            if (digitosActuales_t->unidadesMinutos != digitosPrevios.unidadesAnterior)
            {
                DibujarDigito(horas, 1, digitosActuales_t->unidadesMinutos);
                digitosPrevios.unidadesAnterior = digitosActuales_t->unidadesMinutos;
            }
            if (digitosActuales_t->decenasSegundos != digitosPrevios.decenasSegundosAnterior)
            {
                DibujarDigito(minutos, 0, digitosActuales_t->decenasSegundos);
                digitosPrevios.decenasSegundosAnterior = digitosActuales_t->decenasSegundos;
            }
            if (digitosActuales_t->unidadesSegundos != digitosPrevios.unidadesSegundosAnterior)
            {
                DibujarDigito(minutos, 1, digitosActuales_t->unidadesSegundos);
                digitosPrevios.unidadesSegundosAnterior = digitosActuales_t->unidadesSegundos;
            }
            ILI9341DrawFilledCircle(160, 90, 5, DIGITO_ENCENDIDO);
            ILI9341DrawFilledCircle(160, 130, 5, DIGITO_ENCENDIDO);

            xSemaphoreGive(semaforoAccesoDigitos); // Liberar semáforo
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250)); // Delay para el manejo de la pantalla
    }
}
void app_main(void)
{
    // Inicialización de los dígitos del cronómetro
    digitosActuales_t = malloc(sizeof(struct digitosCronometro));
    digitosActuales_t->unidadesSegundos = 0;
    digitosActuales_t->decenasSegundos = 0;
    digitosActuales_t->unidadesMinutos = 0;
    digitosActuales_t->decenasMinutos = 0;

    // Configuración de los pines del LED RGB
    gpio_set_direction(RGB_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_AZUL, GPIO_MODE_OUTPUT);

    // Apagar el LED RGB
    gpio_set_level(RGB_ROJO, 0);
    gpio_set_level(RGB_VERDE, 0);
    gpio_set_level(RGB_AZUL, 0);

    // Configuración de los pines de los pulsadores
    gpio_set_direction(TEC1_Pausa, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC1_Pausa, GPIO_PULLUP_ONLY); // Resistencia interna de pull-up

    gpio_set_direction(TEC2_Reiniciar, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC2_Reiniciar, GPIO_PULLUP_ONLY); // Resistencia interna de pull-up

    // Se crea el semáforo para proteger el acceso a los dígitos
    semaforoAccesoDigitos = xSemaphoreCreateMutex();

    // Crear las tareas
    xTaskCreate(ControlTiempo, "ControlTiempo", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(EscanearPulsadores, "EscanearPulsadores", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(ActualizarPantalla, "ActualizarPantalla", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);

    while (1)
    {
    }
}
