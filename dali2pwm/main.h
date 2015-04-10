#ifndef MAIN_H
#define MAIN_H

#define F_CLKIO     F_CPU / 8   // CPU frequency is divided by 8 (see init() in main.c)

// PWM
#define PWM_DIVIDER_1       (0 << CS12) | (0 << CS11) | (1 << CS10)     // PWM frequency divider :1
#define PWM_DIVIDER_8       (0 << CS12) | (1 << CS11) | (0 << CS10)     // PWM frequency divider :8
#define PWM_DIVIDER_64      (0 << CS12) | (1 << CS11) | (1 << CS10)     // PWM frequency divider :64
#define PWM_DIVIDER_256     (1 << CS12) | (0 << CS11) | (0 << CS10)     // PWM frequency divider :256
#define PWM_DIVIDER_1024    (1 << CS12) | (0 << CS11) | (1 << CS10)     // PWM frequency divider :1024

#define PWM_MODE_FAST_PWM_8BITS_A   (0 << WGM11) | (1 << WGM10)     // Fast PWM - 8bits (mode 5)
#define PWM_MODE_FAST_PWM_8BITS_B   (0 << WGM13) | (1 << WGM12)     // Fast PWM - 8bits (mode 5)
#define PWM_MODE_FAST_PWM_16BITS_A  (1 << WGM11) | (0 << WGM10)     // Fast PWM - 16bits (mode 14)
#define PWM_MODE_FAST_PWM_16BITS_B  (1 << WGM13) | (1 << WGM12)     // Fast PWM - 16bits (mode 14)

#define PWM_OFF     (0 << COM1A1) | (0 << COM1A0)   // OC1A off
#define PWM_NORMAL  (1 << COM1A1) | (0 << COM1A0)   // Clear OC1A on Compare Match, Set OC1A at TOP
#define PWM_INVERT  (1 << COM1A1) | (1 << COM1A0)   // Set OC1A on Compare Match, Clear OC1A at TOP

#endif
