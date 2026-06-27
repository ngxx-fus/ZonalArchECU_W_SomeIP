#include "All.h"

void IOConfig(uint64_t pin_bit_mask, gpio_mode_t mode, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en, gpio_int_type_t intr_type){
    if(pull_up_en <= 0)          pull_up_en     = GPIO_PULLUP_DISABLE;
    if(pull_down_en <= 0)        pull_down_en   = GPIO_PULLDOWN_DISABLE;
    if(intr_type <= 0)           intr_type      = GPIO_INTR_DISABLE;
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = mode,
        .intr_type = intr_type,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
    };
    gpio_config(&pin);
}

void IOConfigAsOutput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en){
    if(pull_up_en <= 0)          pull_up_en     = GPIO_PULLUP_DISABLE;
    if(pull_down_en <= 0)        pull_down_en   = GPIO_PULLDOWN_DISABLE;
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
    };
    gpio_config(&pin);
}

void IOConfigAsInput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en){
    if(pull_up_en <= 0)          pull_up_en     = GPIO_PULLUP_DISABLE;
    if(pull_down_en <= 0)        pull_down_en   = GPIO_PULLDOWN_DISABLE;
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
    };
    gpio_config(&pin);
}

void IOConfigAsOutputODPullUp(uint64_t pin_bit_mask){
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_OUTPUT_OD,        // Output Open-Drain
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // Force Enable Pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&pin);
}

void IOConfigAsInputOutput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en){
    if(pull_up_en <= 0)          pull_up_en     = GPIO_PULLUP_DISABLE;
    if(pull_down_en <= 0)        pull_down_en   = GPIO_PULLDOWN_DISABLE;
    
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_INPUT_OUTPUT,     // Input + Output
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
    };
    gpio_config(&pin);
}

void IOConfigAsInputOutputODPullUp(uint64_t pin_bit_mask){
    gpio_config_t pin = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  // Input + Output Open-Drain
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // Force Enable Pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&pin);
}

#if (SYSTEM_SAFE_THREAD_LOG_EN == 1)
    /// @brief Spinlock to protect printing resource (UART)
    portMUX_TYPE __LogSpinLock = portMUX_INITIALIZER_UNLOCKED;

    /// @brief Thread-safe logging wrapper function
    /// @param fmt Format string (printf style)
    /// @param ... Variable arguments
    void CoreLog(const char *fmt, ...) {

        /// Enter Critical Section - Disable interrupts, lock other Cores
        portENTER_CRITICAL(&__LogSpinLock);

        va_list args;
        static char log_buffer[256]; /// Temporary buffer

        /// Start argument processing
        va_start(args, fmt);

        /// Format string into buffer first (Avoid holding lock during calculation)
        /// vsnprintf is safer than vsprintf due to length limitation
        vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

 

        /// Print formatted string using low-level function
        /// The string is now continuous, preventing interleaving
        ets_printf("%s", log_buffer);

        /// End argument processing
        va_end(args);

        /// Exit Critical Section
        portEXIT_CRITICAL(&__LogSpinLock);
    }

#endif /// (SYSTEM_SAFE_THREAD_LOG_EN == 1)


