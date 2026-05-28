#include "led_effects.h"
#include "gpio.h"
#include "main.h"

#define LED_UPDATE_RATE_MS    1
#define EFFECT_UPDATE_RATE_MS 10
#define BREATHING_CYCLE_MS    3000
#define RAINBOW_CYCLE_MS      5000
#define PULSE_CYCLE_MS        1500
#define FADE_CYCLE_MS         4000
#define SPARKLE_DURATION_MS   2000
#define PWM_PRECISION         32

static LED_Manager led_manager = {
    .current_effect = EFFECT_BREATHING,
    .current_state = LED_STATE_ACTIVE,
    .effect_speed = 1,
    .brightness = 255,
    .effect_index = 0,
    .tick_counter = 0,
    .is_effect_running = 1,
    .pwm_counter = 0
};

static const RGB_Color rainbow_colors[] = {
    {255, 0, 0},
    {255, 127, 0},
    {255, 255, 0},
    {0, 255, 0},
    {0, 0, 255},
    {75, 0, 130},
    {148, 0, 211}
};

static const RGB_Color state_colors[] = {
    {0, 255, 255},
    {0, 255, 0},
    {255, 255, 0},
    {255, 0, 0},
    {0, 0, 255}
};

static const uint8_t sine_table[] = {
    128, 152, 175, 195, 211, 222, 230, 235, 237, 235, 230, 222, 211, 195, 175, 152,
    128, 104,  81,  61,  45,  34,  26,  21,  19,  21,  26,  34,  45,  61,  81, 104
};

static RGB_Color interpolate_color(RGB_Color start, RGB_Color end, uint8_t progress) {
    RGB_Color result;
    result.red = start.red + (int)((end.red - start.red) * progress) / 255;
    result.green = start.green + (int)((end.green - start.green) * progress) / 255;
    result.blue = start.blue + (int)((end.blue - start.blue) * progress) / 255;
    return result;
}

static void set_led_with_brightness(RGB_Color color, uint8_t brightness) {
    uint8_t r = (color.red * brightness) / 255;
    uint8_t g = (color.green * brightness) / 255;
    uint8_t b = (color.blue * brightness) / 255;

    led_manager.target_red = r;
    led_manager.target_green = g;
    led_manager.target_blue = b;
}

static void update_soft_pwm(void) {
    led_manager.pwm_counter++;
    
    if (led_manager.pwm_counter >= PWM_PRECISION) {
        led_manager.pwm_counter = 0;
    }
    
    uint8_t pwm_threshold = led_manager.pwm_counter;
    
    uint8_t red_pwm = (led_manager.target_red * PWM_PRECISION) / 255;
    uint8_t green_pwm = (led_manager.target_green * PWM_PRECISION) / 255;
    uint8_t blue_pwm = (led_manager.target_blue * PWM_PRECISION) / 255;
    
    if (red_pwm > pwm_threshold) {
        LL_GPIO_SetOutputPin(RED_GPIO_Port, RED_Pin);
    } else {
        LL_GPIO_ResetOutputPin(RED_GPIO_Port, RED_Pin);
    }
    
    if (green_pwm > pwm_threshold) {
        LL_GPIO_SetOutputPin(GREN_GPIO_Port, GREN_Pin);
    } else {
        LL_GPIO_ResetOutputPin(GREN_GPIO_Port, GREN_Pin);
    }
    
    if (blue_pwm > pwm_threshold) {
        LL_GPIO_SetOutputPin(LED_GPIO_Port, LED_Pin);
    } else {
        LL_GPIO_ResetOutputPin(LED_GPIO_Port, LED_Pin);
    }
}

static void effect_breathing(void) {
    uint32_t cycle_pos = led_manager.tick_counter % BREATHING_CYCLE_MS;
    uint8_t sine_index = (uint8_t)((cycle_pos * 32) / BREATHING_CYCLE_MS);
    uint8_t brightness = (uint8_t)((uint16_t)sine_table[sine_index] * led_manager.brightness / 255);

    RGB_Color color = state_colors[led_manager.current_state];
    set_led_with_brightness(color, brightness);
}

static void effect_rainbow_cycle(void) {
    uint8_t num_colors = sizeof(rainbow_colors) / sizeof(rainbow_colors[0]);
    uint8_t color_index = (led_manager.tick_counter / (RAINBOW_CYCLE_MS / num_colors)) % num_colors;

    RGB_Color current_color = rainbow_colors[color_index];
    set_led_with_brightness(current_color, led_manager.brightness);
}

static void effect_pulse_wave(void) {
    uint32_t cycle_pos = led_manager.tick_counter % PULSE_CYCLE_MS;
    uint8_t brightness = (uint8_t)((PULSE_CYCLE_MS - cycle_pos) * led_manager.brightness / PULSE_CYCLE_MS);

    RGB_Color color = state_colors[led_manager.current_state];
    set_led_with_brightness(color, brightness);
}

static void effect_color_fade(void) {
    uint8_t num_colors = sizeof(state_colors) / sizeof(state_colors[0]);
    uint32_t sub_cycle = FADE_CYCLE_MS / num_colors;
    uint8_t color_index = led_manager.tick_counter / sub_cycle % num_colors;
    uint8_t next_index = (color_index + 1) % num_colors;

    uint32_t progress = (led_manager.tick_counter % sub_cycle);
    uint8_t fade_progress = (uint8_t)(progress * 255 / sub_cycle);
    RGB_Color faded_color = interpolate_color(state_colors[color_index], state_colors[next_index], fade_progress);

    set_led_with_brightness(faded_color, led_manager.brightness);
}

static void effect_sparkle(void) {
    if ((led_manager.tick_counter % 100) < 50) {
        uint8_t num_colors = sizeof(rainbow_colors) / sizeof(rainbow_colors[0]);
        uint8_t color_index = (led_manager.tick_counter / 100) % num_colors;
        set_led_with_brightness(rainbow_colors[color_index], led_manager.brightness);
    } else {
        RGB_Color off = {0, 0, 0};
        set_led_with_brightness(off, 0);
    }
}

static void effect_status_indicator(void) {
    uint32_t progress = led_manager.tick_counter % 1000;
    uint8_t brightness = led_manager.brightness;

    if (progress < 200) {
        brightness = (uint8_t)(progress * led_manager.brightness / 200);
    } else if (progress < 400) {
        brightness = led_manager.brightness;
    } else {
        brightness = 0;
    }

    RGB_Color color = state_colors[led_manager.current_state];
    set_led_with_brightness(color, brightness);
}

static void effect_dual_alternate(void) {
    uint8_t num_colors = sizeof(state_colors) / sizeof(state_colors[0]);
    uint8_t color_index = led_manager.tick_counter / 500 % num_colors;

    RGB_Color primary_color = state_colors[color_index];
    RGB_Color secondary_color = state_colors[(color_index + 1) % num_colors];

    if ((led_manager.tick_counter / 500) % 2 == 0) {
        set_led_with_brightness(primary_color, led_manager.brightness);
    } else {
        set_led_with_brightness(secondary_color, led_manager.brightness);
    }
}

static void effect_rainbow_fade(void) {
    uint8_t num_colors = sizeof(rainbow_colors) / sizeof(rainbow_colors[0]);
    uint32_t sub_cycle = FADE_CYCLE_MS / num_colors;
    uint8_t color_index = (uint8_t)((led_manager.tick_counter % FADE_CYCLE_MS) / sub_cycle) % num_colors;
    uint8_t next_index = (color_index + 1) % num_colors;

    uint32_t progress = led_manager.tick_counter % sub_cycle;
    uint8_t fade_progress = (uint8_t)(progress * 255 / sub_cycle);
    RGB_Color faded_color = interpolate_color(rainbow_colors[color_index], rainbow_colors[next_index], fade_progress);

    set_led_with_brightness(faded_color, led_manager.brightness);
}

void LED_Effects_Init(void) {
    led_manager.tick_counter = 0;
    led_manager.pwm_counter = 0;
    led_manager.current_effect = EFFECT_BREATHING;
    led_manager.current_state = LED_STATE_ACTIVE;
    led_manager.is_effect_running = 1;
    led_manager.target_red = 0;
    led_manager.target_green = 0;
    led_manager.target_blue = 0;
}

void LED_Effects_Process(void) {
    if (!led_manager.is_effect_running) {
        RGB_Color off = {0, 0, 0};
        set_led_with_brightness(off, 0);
        update_soft_pwm();
        return;
    }

    static uint8_t effect_tick_accum = 0;
    effect_tick_accum += LED_UPDATE_RATE_MS;

    if (effect_tick_accum >= EFFECT_UPDATE_RATE_MS) {
        effect_tick_accum = 0;
        led_manager.tick_counter += EFFECT_UPDATE_RATE_MS * led_manager.effect_speed;

        switch (led_manager.current_effect) {
            case EFFECT_BREATHING:
                effect_breathing();
                break;
            case EFFECT_RAINBOW_CYCLE:
                effect_rainbow_cycle();
                break;
            case EFFECT_PULSE_WAVE:
                effect_pulse_wave();
                break;
            case EFFECT_COLOR_FADE:
                effect_color_fade();
                break;
            case EFFECT_SPARKLE:
                effect_sparkle();
                break;
            case EFFECT_STATUS_INDICATOR:
                effect_status_indicator();
                break;
            case EFFECT_DUAL_ALTERNATE:
                effect_dual_alternate();
                break;
            case EFFECT_RAINBOW_FADE:
                effect_rainbow_fade();
                break;
            default:
                effect_breathing();
                break;
        }
    }

    update_soft_pwm();
}

void LED_Effects_SetEffect(LED_Effect_TypeDef effect) {
    if (effect < EFFECT_MAX) {
        led_manager.current_effect = effect;
        led_manager.tick_counter = 0;
    }
}

void LED_Effects_SetState(LED_State_TypeDef state) {
    if (state < LED_STATE_MAX) {
        led_manager.current_state = state;
    }
}

LED_Effect_TypeDef LED_Effects_GetCurrentEffect(void) {
    return led_manager.current_effect;
}

void LED_Effects_NextEffect(void) {
    led_manager.current_effect = (LED_Effect_TypeDef)((led_manager.current_effect + 1) % EFFECT_MAX);
    led_manager.tick_counter = 0;
}

void LED_Effects_SetSpeed(uint32_t speed) {
    if (speed >= 1 && speed <= 10) {
        led_manager.effect_speed = (uint8_t)speed;
    }
}
