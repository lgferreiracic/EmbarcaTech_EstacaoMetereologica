#include "my_tasks.h"


QueueHandle_t xQueueSensorData;
SemaphoreHandle_t xAlertMutex;
TimerHandle_t xBuzzerSilenceTimer;

volatile uint32_t last_button_press_time = 0;
volatile bool alarm_active = false;
volatile bool alert_silenced = false; 

bool matrix_alert_active = false;
ssd1306_t ssd;  
bool cor = true;
bool web_server_initialized = false;
bool buzzer_silenced = false;
struct repeating_timer timer;

// Inicializa as filas 
void init_queues() {
    xQueueSensorData = xQueueCreate(1, sizeof(SensorReadings));
    if (xQueueSensorData == NULL) {
        printf("Failed to create sensor data queue\n");
        panic_unsupported();
    }
}

// Inicializa os semáforos
void init_semaphores() {
    xAlertMutex = xSemaphoreCreateMutex();

    if (xAlertMutex == NULL) {
        printf("Failed to create semaphore\n");
        panic_unsupported();
    }
}

// Função para parar o alarme
bool stop_alarm(struct repeating_timer *t) {
    if (xSemaphoreTake(xAlertMutex, 0) == pdTRUE) {
        alarm_active = false; // Desativa o alarme
        buzzer_silenced = true;
        
        if (xTimerIsTimerActive(xBuzzerSilenceTimer)) {
            xTimerStop(xBuzzerSilenceTimer, 0);
        }
        xTimerStart(xBuzzerSilenceTimer, 0);

        printf("\nAlarme desativado, buzzer silenciado por 60 segundos.\n");
        xSemaphoreGive(xAlertMutex);
        return false;
    }
    return true;
}

// Callback para o timer de silenciamento do buzzer
void vBuzzerSilenceCallback(TimerHandle_t xTimer) {
    buzzer_silenced = false;
    alert_silenced = false; // Reativa o alerta visual também
    alert_silenced = false; //er e al Rtaeativa o alerta visual também
    printf("\nSilêncio do buzzer e alerta encerrado.\n");
}

// Função de interrupção para gerenciar os botões
void irq_handler(uint gpio, uint32_t events){
        if (debounce(&last_button_press_time)){
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (gpio == BUTTON_A_PIN){
                ssd1306_clear(&ssd); // Limpa o display
                clear_matrix(); // Limpa a matriz
                reset_usb_boot(0, 0); // Reinicia o dispositivo
            } else if (gpio == BUTTON_B_PIN){
                stop_alarm(&timer);
            } else if (gpio == JOYSTICK_BUTTON_PIN){
                cor = !cor; // Alterna a cor do display
            }
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Troca o contexto da tarefa
        }
}

// Tarefa para leitura dos sensores
void vSensorTask(void *params) {
    init_i2c_sensor();      
    init_bmp280();        
    init_aht20();
    while (1) {
        SensorReadings readings = get_sensor_readings();
        xQueueOverwrite(xQueueSensorData, &readings); // Atualiza a fila com as leituras dos sensores
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay de 1 segundo
    }
}

// Tarefa para gerenciar os alertas
void vAlertTask(void *params) {
    // Inicializa os componentes necessários
    buzzer_init_all(); 
    led_init_all(); 
    matrix_init(); 

    SensorReadings readings;
    xBuzzerSilenceTimer = xTimerCreate(
        "SilenceTimer",
        pdMS_TO_TICKS(60000),  // 60 segundos
        pdFALSE,
        NULL,
        vBuzzerSilenceCallback
    );

    while(!web_server_initialized) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay de 1 segundo
    }

    while (1) {
        if (xQueuePeek(xQueueSensorData, &readings, portMAX_DELAY) == pdTRUE) {
            float average_temp = (readings.bmp_temp + readings.aht_temp) / 2;
            bool alert_triggered = (average_temp > ALERT_THRESHOLD_TEMPERATURE ||
                                    readings.humidity > ALERT_THRESHOLD_HUMIDITY ||
                                    readings.pressure_kpa > ALERT_THRESHOLD_PRESSURE);
            if (alert_triggered) {
                if (xSemaphoreTake(xAlertMutex, portMAX_DELAY) == pdTRUE) {
                    alarm_active = true; // Ativa o alarme
                    printf("\nAlarme ativado\n");
                    xSemaphoreGive(xAlertMutex);
                }
                if(!buzzer_silenced) {
                    play_denied_sound(); // Toca o som de alarme
                }
                if(!matrix_alert_active){
                    matrix_alert(); // Exibe alerta na matriz de LEDs RGB
                    matrix_alert_active = true;
                }
                red(); // Acende o LED RGB vermelho
            } else {
                if (xSemaphoreTake(xAlertMutex, portMAX_DELAY) == pdTRUE) {
                    alarm_active = false; // Desativa o alarme
                    printf("\nAlarme desativado\n");
                    xSemaphoreGive(xAlertMutex);
                }
                clear_matrix(); // Limpa a matriz de LEDs RGB
                green(); // Acende o LED RGB verde
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay de 1000 ms
    }
}

// Tarefa para gerenciar o display
void vDisplayTask(void *params) {
    display_init(&ssd); // Inicializa o display
    SensorReadings readings;
    
    while (!web_server_initialized) {
        start_display(&ssd); // Inicia o display
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay de 1 segundo
    }

    while (1) {
        if (xQueuePeek(xQueueSensorData, &readings, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xAlertMutex, portMAX_DELAY) == pdTRUE) {
                if (alarm_active) {
                    display_alert(&ssd, readings.bmp_temp, readings.altitude_m, readings.aht_temp, readings.humidity, web_server_initialized, cor);
                } else {
                    display_weather(&ssd, readings.bmp_temp, readings.altitude_m, readings.aht_temp, readings.humidity, web_server_initialized, cor);
                }
                xSemaphoreGive(xAlertMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay de 100 ms
    }
}

// Tarefa para gerenciar o servidor web
void vWebServerTask(void *params) {
    server_init(); // Inicializa o servidor web
    web_server_initialized = true; // Marca o servidor como inicializado
    while (1) {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo  
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay de 100 ms
    }
    cyw43_arch_deinit(); // Desativa o Wi-Fi ao finalizar a tarefa
}

// Função para lidar com as requisições HTP
bool handle_http_request(const char *request, size_t request_size, char *response, size_t response_size){
    SensorReadings readings;
    if (request_size >= 9 && strncmp(request, "GET /data", 9) == 0){
        if (xQueuePeek(xQueueSensorData, &readings, 0) == pdTRUE) {
            snprintf(response, response_size, "{\"temp\":%.2f,\"hum\":%.2f,\"alt\":%.2f}", readings.bmp_temp, readings.humidity, readings.altitude_m);
            return true;
        }
    } else if (strstr(request, "POST /stop") != NULL){
        stop_alarm_web();
        snprintf(response, response_size, "{\"alert\":\"false\"}");
        return true;
    } else if (request_size >= 10 && strncmp(request, "GET /alarm", 10) == 0){
        if(xSemaphoreTake(xAlertMutex, 0) == pdTRUE) {
            bool show_alert = alarm_active && !alert_silenced;
            snprintf(response, response_size, "{\"alert\":\"%d\"}", show_alert ? 1 : 0);
            xSemaphoreGive(xAlertMutex);
        } else 
            snprintf(response, response_size, "{\"error\":\"mutex_unavailable\"}");
        return true;
    }
    return false;
}

// Função para encerrar o alarme pela página
void stop_alarm_web() {
    if (xSemaphoreTake(xAlertMutex, 0) == pdTRUE) {
        alarm_active = false;
        buzzer_silenced = true;
        alert_silenced = true; // Silencia o alerta visual também
        
        if (xTimerIsTimerActive(xBuzzerSilenceTimer)) {
            xTimerStop(xBuzzerSilenceTimer, 0);
        }
        xTimerStart(xBuzzerSilenceTimer, 0);

        printf("\nAlarme desativado via web, buzzer e alerta silenciados por 60 segundos.\n");
        xSemaphoreGive(xAlertMutex);
    }
}

// Função para inicializar as tarefas
void init_tasks() {
    xTaskCreate(vSensorTask, "Sensor Task", 256, NULL, 1, NULL);
    xTaskCreate(vAlertTask, "Alert Task", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vWebServerTask, "Web Server Task", 512, NULL, 1, NULL);
}



