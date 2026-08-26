#include "device_state.h"
#include <stdlib.h>

static device_state_t s_state = {0};

static peq_band_t generate_default_peq_band(void) {
    return (peq_band_t) {
        .gain_db = 0,
            .enabled = false,
            .frequency_hz = 1000,
            .q = 1,
            .type = EQ_TYPE_PEAK
    };
}

static input_state_t generate_default_input_state() {
    input_state_t state = {
        .gain_db = 0.0f,
        .muted = false,
    };

    for (int i = 0; i < PEQ_BANDS; i++) {
        state.peq[i] = generate_default_peq_band();
    }

    return state;
}

static output_state_t generate_default_output_state() {
    output_state_t state = {
        .gain_db = 0.0f,
        .muted = false,
    };

    for (int i = 0; i < PEQ_BANDS; i++) {
        state.peq[i] = generate_default_peq_band();
    }

    return state;
}

static dsp_config_t generate_default_dsp_config() {
    dsp_config_t config = {0};

    for (int i = 0; i < INPUT_COUNT; i++) {
        config.inputs[i] = generate_default_input_state();
    }

    for (int i = 0; i < INPUT_COUNT; i++) {
        config.outputs[i] = generate_default_output_state();
    }

    return config;

}

device_state_t generate_empty_device_state() {
    return (device_state_t) {
        .dsp = generate_default_dsp_config(),
        .active_preset = INVALID_PRESET,
        .preset_modified = false
    };
}

const device_state_t* device_state_get(void) {
    return &s_state;
}
