
/*
Problema al mostrar los datos en pantalla, se crearon dos paneles de 4 dígitos y 1 de 1 dígito, de otra manera no se actualiza correctamente
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ili9341.h"
#include "digitos.h"
#include "stdio.h"
#include "driver/gpio.h"

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

// Estructura de los dígitos
typedef struct
{
    int decenasMinutos;
    int unidadesMinutos;
    int decenasSegundos;
    int unidadesSegundos;
    int decimasSegundo;
} DigitosCronometro;

DigitosCronometro digitosActuales = {0, 0, 0, 0, 0};
SemaphoreHandle_t semaforoAccesoDigitos;

/*****************************************FUNCIONES AUXILIARES**********************************************/
/**
 * @brief  Accede a la estructura en donde se almacenan los digitos del semaforo y se actualiza el conteo del tiempo
 *         La estructura que contiene los dígitos del cronómetro es un recurso compartido, por eso se usa el MUTEX
 */
void ActualizarDigitos()
{
    if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY)) // Se toma recurso, se actualiza el MUTEX
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
        xSemaphoreGive(semaforoAccesoDigitos); // Se libera el MUTEX
    }
}

/**
 * @brief Resetea los dígitos del cronómetro a 00:00
 *
 */
void ReiniciarCronometro()
{
    if (xSemaphoreTake(semaforoAccesoDigitos, portMAX_DELAY))
    {
        digitosActuales.unidadesSegundos = 0;
        digitosActuales.decenasSegundos = 0;
        digitosActuales.decimasSegundo = 0;
        digitosActuales.unidadesMinutos = 0;
        digitosActuales.decenasMinutos = 0;
        xSemaphoreGive(semaforoAccesoDigitos);
    }
}

/**
 * @brief Contiene la logica para prender el color correspondiente del led RGB
 *
 * @param estadoLed recibe el estado del led RGB
 */
void PrenderLedVerde(bool estadoLed)
{
    gpio_set_level(RGB_VERDE, estadoLed);
    gpio_set_level(RGB_ROJO, LED_OFF);
    gpio_set_level(RGB_AZUL, LED_OFF);
}

/**
 * @brief Prende el led rojo del RGB
 *
 * Cambia el estado del led rojo del RGB. Si estadoLed es verdadero, prende el led rojo,
 * si es falso, lo apaga.
 *
 * @param estadoLed estado del led rojo
 */
void PrenderLedRojo(bool estadoLed)
{
    gpio_set_level(RGB_VERDE, LED_OFF);
    gpio_set_level(RGB_ROJO, estadoLed);
    gpio_set_level(RGB_AZUL, LED_OFF);
}

/**
 * @brief Configura los pines del led RGB como salidas
 *
 * Inicializa los pines del led RGB como salidas para poder
 * controlar el color del led.
 */
void ConfigurarSalidasLed(void)
{
    gpio_set_direction(RGB_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_AZUL, GPIO_MODE_OUTPUT);
}

/**
 * @brief Configura los pines de las teclas como entradas con pull-up
 *
 * Inicializa los pines de las teclas como entradas con pull-up.
 * La tecla de pausa se configura en el pin GPIO_NUM_4 y la tecla de reiniciar
 * se configura en el pin GPIO_NUM_6.
 */
void ConfigurarTeclas(void)
{
    gpio_set_direction(TEC1_Pausa, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC1_Pausa, GPIO_PULLUP_ONLY);
    gpio_set_direction(TEC2_Reiniciar, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEC2_Reiniciar, GPIO_PULLUP_ONLY);
}

/**
 * @brief Dibuja un mensaje de inicio en la pantalla y enciende los LEDs RGB
 *
 * Dibuja un mensaje de inicio en la pantalla y enciende los LEDs RGB de manera
 * secuencial. La barra es dibujada en azul y cada 2 incrementos se enciende un
 * LED diferente. La pausa entre cada paso es de 750 ms.
 *
 * @note Esta función debe ser llamada solo una vez al inicio del programa.
 */
void MensajeInicio(void)
{
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    ILI9341Fill(ILI9341_BLACK);

    // Para controlar la barra
    int barraXInicio = 55;                     // Coordenada X inicial de la barra
    int barraY = 141;                          // Coordenada Y de la barra
    int barraAnchoTotal = 209;                 // Ancho total de la barra
    int barraAltura = 10;                      // Altura de la barra
    int barraIncremento = barraAnchoTotal / 6; // Incremento en 6 pasos, ajustado para evitar espacios

    // Dibujar el texto centrado
    ILI9341DrawString(55, 111, "Iniciando Cronometro", &font_11x18, ILI9341_BLUE2, ILI9341_BLACK);

    // Dibujar el contorno de la barra en rojo
    ILI9341DrawRectangle(barraXInicio, barraY, barraXInicio + barraAnchoTotal, barraY + barraAltura, ILI9341_BLUE2);

    // Configurar GPIOs de LEDs
    gpio_set_direction(RGB_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_AZUL, GPIO_MODE_OUTPUT);

    // Apagar LEDs al inicio
    gpio_set_level(RGB_ROJO, 0);
    gpio_set_level(RGB_VERDE, 0);
    gpio_set_level(RGB_AZUL, 0);

    // Dibujar la barra interior en azul y encender LEDs secuencialmente
    for (int i = 1; i <= 6; i++)
    {
        // Incrementar la barra
        int barraXFinal = barraXInicio + (barraIncremento * i);
        if (i == 6)
            barraXFinal = barraXInicio + barraAnchoTotal;
        ILI9341DrawFilledRectangle(barraXInicio, barraY, barraXFinal, barraY + barraAltura, ILI9341_BLUE2);

        // Encender un LED cada 2 incrementos de la barra
        if (i == 2)
        {
            gpio_set_level(RGB_ROJO, 1); // Enciende LED rojo
        }
        else if (i == 4)
        {
            gpio_set_level(RGB_ROJO, 0);  // Apaga LED rojo
            gpio_set_level(RGB_VERDE, 1); // Enciende LED verde
        }
        else if (i == 6)
        {
            gpio_set_level(RGB_VERDE, 0); // Apaga LED verde
            gpio_set_level(RGB_AZUL, 1);  // Enciende LED azul
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(750)); // Pausa de 750 ms entre cada paso
    }

    // Antes de salir, limpiar la pantalla para que quede lista para el cronómetro
    ILI9341Fill(ILI9341_BLACK);
}

/*****************************************CODIGO PARA LAS TAREAS*****************************************/

/**
 * @brief Maneja el estado actual del cronómetro
 *
 * Tarea que se encarga de manejar el estado actual del cronómetro,
 * actualizando los dígitos y cambiando el estado de los LEDs según el caso.
 * La tarea utiliza un semáforo para acceder a los valores actuales del cronómetro y
 * se ejecuta a una tasa de 1 Hz.
 * @param parametros No se utiliza
 */
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

/**
 * @brief Función que se encarga de escanear los pulsadores y de
 *        realizar las acciones correspondientes según el estado
 *        actual del cronómetro.
 *
 * @param parametros No se utiliza
 */
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

/**
 * @brief Actualiza la pantalla con los valores actuales del cronómetro
 *
 * Tarea que se encarga de actualizar la pantalla TFT con los valores actuales del
 * cronómetro, comparando con los valores previamente mostrados y actualizando solo
 * los dígitos que cambiaron. Adicionalmente, dibuja los separadores estáticos.
 * La tarea utiliza un semáforo para acceder a los valores actuales del cronómetro y
 * actualiza la pantalla a una tasa de 4 Hz.
 *@param parametros No se utiliza
 * */
void ActualizarPantalla(void *parametros)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //    ILI9341Init();
    //    ILI9341Rotate(ILI9341_Landscape_1);

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
    // Se crean las tareas
    MensajeInicio();
    xTaskCreate(ManejarEstadoCronometro, "ManejarEstadoCronometro", 2048, NULL, 2, NULL);
    xTaskCreate(EscanearPulsadores, "EscanearPulsadores", 1024, NULL, 1, NULL);
    xTaskCreate(ActualizarPantalla, "ActualizarPantalla", 4096, NULL, 1, NULL);
}