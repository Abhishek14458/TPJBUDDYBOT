#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// GPIO Pin Definitions
#define TOUCH_PIN 4          // Touch Sensor Pin
#define TRIG_PIN 5           // Ultrasonic Sensor Trigger
#define ECHO_PIN 18          // Ultrasonic Sensor Echo
#define SERVO_PIN 16         // Servo Motor Pin

// System State
bool systemOn = false;

// Function to get distance from Ultrasonic Sensor
float get_distance()
{
    gpio_set_level(TRIG_PIN, 0);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    gpio_set_level(TRIG_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    uint32_t pulse_duration = 0;
    while (gpio_get_level(ECHO_PIN) == 0)
        ; // Wait for pulse to start

    while (gpio_get_level(ECHO_PIN) == 1)
        pulse_duration++;

    float distance = (pulse_duration * 0.0343) / 2;
    return (distance > 400 || distance == 0) ? -1 : distance; // Return -1 for invalid readings
}

// Function to move the servo
void move_servo(int angle)
{
    uint32_t duty = (angle * (1250 - 250) / 180) + 250;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Task to continuously check touch sensor
void touch_task(void *pvParameter)
{
    while (1)
    {
        if (gpio_get_level(TOUCH_PIN) == 1)
        {
            systemOn = true;
            printf("System turned ON!\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Task to control ultrasonic and servo
void ultrasonic_servo_task(void *pvParameter)
{
    while (1)
    {
        if (systemOn)
        {
            float distance = get_distance();
            printf("Distance: %.2f cm\n", distance);

            if (distance > 0 && distance < 30) // Object detected within 30cm
            {
                printf("Object detected! Moving servo...\n");
                move_servo(90);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                move_servo(0);
                systemOn = false; // Turn off system after moving servo once
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// Main setup function
void app_main()
{
    // Configure GPIOs
    gpio_pad_select_gpio(TOUCH_PIN);
    gpio_set_direction(TOUCH_PIN, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    // Configure Servo Motor using LED PWM
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 50,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0};
    ledc_channel_config(&ledc_channel);

    printf("System Initialized.\n");

    // Create tasks
    xTaskCreate(&touch_task, "touch_task", 2048, NULL, 5, NULL);
    xTaskCreate(&ultrasonic_servo_task, "ultrasonic_servo_task", 2048, NULL, 5, NULL);
}
