#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define TRIG_PIN  23   // Corrected Ultrasonic Trigger Pin
#define ECHO_PIN  22   // Corrected Ultrasonic Echo Pin
#define SERVO_PIN 26   // Corrected Servo Pin
#define DISTANCE_THRESHOLD 20  // Trigger distance in cm

static const char *TAG = "ESP32_Project";

// Function to get distance from Ultrasonic Sensor
float get_distance() {
    gpio_set_level(TRIG_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    uint32_t pulse_duration = 0;
    while (gpio_get_level(ECHO_PIN) == 0);
    while (gpio_get_level(ECHO_PIN) == 1) {
        pulse_duration++;
    }

    float distance = (pulse_duration * 0.0343) / 2;
    return (distance > 400 || distance == 0) ? -1 : distance;
}

// Function to move the servo
void move_servo(int angle) {
    int duty = (angle * (125 - 25) / 180) + 25;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Task to monitor ultrasonic sensor and move servo
void ultrasonic_task(void *pvParameter) {
    while (1) {
        float distance = get_distance();
        ESP_LOGI(TAG, "Distance: %.2f cm", distance);
        
        if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
            ESP_LOGI(TAG, "Object detected! Moving servo...");
            move_servo(90);  // Move servo to 90 degrees
        } else {
            move_servo(0);   // Move servo back to 0 degrees
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Main function
void app_main() {
    ESP_LOGI(TAG, "Starting ESP32 Ultrasonic & Servo Project...");

    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    // Configure LEDC for servo
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    // Start ultrasonic sensor task
    xTaskCreate(ultrasonic_task, "ultrasonic_task", 2048, NULL, 5, NULL);
}
