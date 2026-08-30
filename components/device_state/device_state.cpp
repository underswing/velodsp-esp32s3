#include "device_state.hpp"
#include <cstring>

#include "esp_err.h"

static device_state_t s_state = {};

static void init_default_peq_band(peq_band_t *band) {
    *band = (peq_band_t){
        .enabled = false,
        .frequency_hz = 1000.0f,
        .gain_db = 0.0f,
        .q = 1.0f,
        .type = EQ_TYPE_PEAK,
    };
}

static void init_default_input_state(input_state_t *state) {
    state->gain_db = 0.0f;
    state->muted = false;

    for (auto & i : state->peq) {
        init_default_peq_band(&i);
    }
}

static void init_default_output_state(output_state_t *state) {
    state->gain_db = 0.0f;
    state->muted = false;

    for (auto & i : state->peq) {
        init_default_peq_band(&i);
    }
}

esp_err_t device_state_init() {
    memset(&s_state, 0, sizeof(s_state));

    for (auto & input : s_state.dsp.inputs) {
        init_default_input_state(&input);
    }

    for (auto & output : s_state.dsp.outputs) {
        init_default_output_state(&output);
    }

    s_state.active_preset = INVALID_PRESET;
    s_state.preset_modified = false;

    return ESP_OK;
}

const device_state_t *device_state_get() {
    return &s_state;
}
