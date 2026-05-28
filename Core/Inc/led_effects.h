#ifndef __LED_EFFECTS_H__
#define __LED_EFFECTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum {
    EFFECT_BREATHING = 0,
    EFFECT_RAINBOW_CYCLE,
    EFFECT_PULSE_WAVE,
    EFFECT_COLOR_FADE,
    EFFECT_SPARKLE,
    EFFECT_STATUS_INDICATOR,
    EFFECT_DUAL_ALTERNATE,
    EFFECT_RAINBOW_FADE,
    EFFECT_MAX
} LED_Effect_TypeDef;

typedef enum {
    LED_STATE_IDLE = 0,
    LED_STATE_ACTIVE,
    LED_STATE_WARNING,
    LED_STATE_ERROR,
    LED_STATE_SUCCESS,
    LED_STATE_MAX
} LED_State_TypeDef;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB_Color;

typedef struct {
    LED_Effect_TypeDef current_effect;
    LED_State_TypeDef current_state;
    uint32_t effect_speed;
    uint8_t brightness;
    uint8_t effect_index;
    uint32_t tick_counter;
    uint8_t is_effect_running;
    uint8_t pwm_counter;
    uint8_t target_red;
    uint8_t target_green;
    uint8_t target_blue;
} LED_Manager;

void LED_Effects_Init(void);
void LED_Effects_Process(void);
void LED_Effects_SetEffect(LED_Effect_TypeDef effect);
void LED_Effects_SetState(LED_State_TypeDef state);
LED_Effect_TypeDef LED_Effects_GetCurrentEffect(void);
void LED_Effects_NextEffect(void);
void LED_Effects_SetSpeed(uint32_t speed);

#ifdef __cplusplus
}
#endif

#endif
