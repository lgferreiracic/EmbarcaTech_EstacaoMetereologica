#include <stdio.h>
#include "pico/stdlib.h"
#include "my_tasks.h"

int main(){
    stdio_init_all(); 
    button_init_all(); 
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
    
    init_queues();
    init_semaphores();
    init_tasks();

    vTaskStartScheduler(); // Inicia o agendador do FreeRTOS
    panic_unsupported();
    return 0;
}