#ifndef MY_TASKS_H
#define MY_TASKS_H

#include "ssd1306.h"
#include "button.h"
#include "matrix.h"
#include "buzzer.h"
#include "led_rgb.h"
#include "joystick.h"
#include "web_server.h"
#include "sensors.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

enum {
    ALERT_TYPE_TEMPERATURE,
    ALERT_TYPE_HUMIDITY,
    ALERT_TYPE_PRESSURE
};

#define ALERT_THRESHOLD_TEMPERATURE 30.0
#define ALERT_THRESHOLD_HUMIDITY 70.0
#define ALERT_THRESHOLD_PRESSURE 1000.0

void init_queues();
void init_semaphores();
void init_tasks();
void vSensorTask(void *params);
void vAlertTask(void *params);
void vDisplayTask(void *params);
void vWebServerTask(void *params);
void stop_alarm_web();

#endif // MY_TASKS_H
