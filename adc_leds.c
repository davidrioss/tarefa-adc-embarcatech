#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Definições de pinos e constantes
#define LED_RED_PIN 13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12

#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define JOYSTICK_BUTTON_PIN 22

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDRESS 0x3C

#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27

#define PWM_WRAP 2048
#define DEBOUNCE_DELAY_MS 200
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Variáveis globais
int16_t fine_tune_x = 0;
int16_t fine_tune_y = 0;

volatile uint8_t led_mode = 0; // 0:off, 1:red, 2:blue, 3:both
volatile bool double_border = false;

ssd1306_t display;
volatile uint32_t last_interrupt_time = 0;

// Função para desenhar a borda do display
void draw_display_border() {
    ssd1306_rect(&display, 3, 3, DISPLAY_WIDTH - 5, DISPLAY_HEIGHT - 5, true, false);
    if (double_border) {
        ssd1306_rect(&display, 1, 1, DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3, true, false);
        ssd1306_rect(&display, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, true, false);
    }
    ssd1306_send_data(&display);
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if ((current_time - last_interrupt_time) > DEBOUNCE_DELAY_MS) {
        last_interrupt_time = current_time;
        if (gpio == BUTTON_A_PIN) {
            led_mode = (led_mode + 1) % 4;
        }
        if (gpio == BUTTON_B_PIN) {
            reset_usb_boot(0, 0);
        }
        if (gpio == JOYSTICK_BUTTON_PIN) {
            gpio_put(LED_GREEN_PIN, !gpio_get(LED_GREEN_PIN));
            double_border = !double_border;
            draw_display_border();
        }
    }
}

// Função para calcular a intensidade do LED
uint16_t calculate_led_intensity(int32_t value) {
    return (value * PWM_WRAP) / 4095;
}

// Função para desenhar o quadrado no display
void draw_square(float x, float y) {
    ssd1306_fill(&display, false);
    draw_display_border();

    // Calcula coordenadas centralizadas
    uint8_t pixel_x = (uint8_t)(x * (DISPLAY_WIDTH - 8));
    uint8_t pixel_y = (uint8_t)((1.0f - y) * (DISPLAY_HEIGHT - 8));

    // Limita as coordenadas
    pixel_x = pixel_x < 0 ? 0 : (pixel_x > (DISPLAY_WIDTH - 8)) ? DISPLAY_WIDTH - 8 : pixel_x;
    pixel_y = pixel_y < 0 ? 0 : (pixel_y > (DISPLAY_HEIGHT - 8)) ? DISPLAY_HEIGHT - 8 : pixel_y;

    ssd1306_rect(&display, pixel_y, pixel_x, 8, 8, true, true);
    ssd1306_send_data(&display);
}

int main() {
    stdio_init_all();
    
    // Inicialização dos GPIOs
    gpio_init(LED_GREEN_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_init(JOYSTICK_BUTTON_PIN);

    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);

    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);

    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_put(LED_GREEN_PIN, false);

    // Inicialização do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&display, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, DISPLAY_ADDRESS, I2C_PORT);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);

    // Inicialização do ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Configuração do PWM
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);

    uint slice_red = pwm_gpio_to_slice_num(LED_RED_PIN);
    pwm_set_wrap(slice_red, PWM_WRAP);
    pwm_set_clkdiv(slice_red, 4);
    pwm_set_enabled(slice_red, true);

    uint slice_blue = pwm_gpio_to_slice_num(LED_BLUE_PIN);
    pwm_set_wrap(slice_blue, PWM_WRAP);
    pwm_set_clkdiv(slice_blue, 4);
    pwm_set_enabled(slice_blue, true);

    // Calibração inicial
    adc_select_input(1);
    fine_tune_x = 2048 - adc_read();
    adc_select_input(0);
    fine_tune_y = 2048 - adc_read();

    while (true) {
        // Leitura do joystick
        adc_select_input(1);
        uint16_t x_velho = 0;
        uint16_t x_value = adc_read() + fine_tune_x;
        // correção pois o joystick defeituoso pulava de 0 para valores acima de 4095
        if (x_value > 4095) {
            x_value = x_velho;
        }  else {
            x_velho = x_value;
        }

        float x_normalized = (float)x_value / 4095.0;

        adc_select_input(0);
        uint16_t y_value = adc_read() + fine_tune_y;
        float y_normalized = (float)y_value / 4095.0;

        // Atualiza o display
        draw_square(x_normalized, y_normalized);

        // Calcula as intensidades
        uint16_t red_intensity = calculate_led_intensity(x_value);
        uint16_t blue_intensity = calculate_led_intensity(y_value);

        // Aplica o modo de operação
        switch(led_mode) {
            case 0: // LEDs desligados
                pwm_set_gpio_level(LED_RED_PIN, 0);
                pwm_set_gpio_level(LED_BLUE_PIN, 0);
                break;
            case 1: // Vermelho ligado
                pwm_set_gpio_level(LED_RED_PIN, red_intensity);
                pwm_set_gpio_level(LED_BLUE_PIN, 0);
                break;
            case 2: // Azul ligado
                pwm_set_gpio_level(LED_RED_PIN, 0);
                pwm_set_gpio_level(LED_BLUE_PIN, blue_intensity);
                break;
            case 3: // Ambos ligados
                pwm_set_gpio_level(LED_RED_PIN, red_intensity);
                pwm_set_gpio_level(LED_BLUE_PIN, blue_intensity);
                break;
        }
        printf("X: %d, Y: %d\n", x_value, y_value);

        sleep_ms(100);
    }
    return 0;
}