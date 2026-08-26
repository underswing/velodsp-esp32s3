#include "device_state.h"
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

static device_state_t s_state = {0};

static void init_default_peq_band(peq_band_t *band) {
    *band = (peq_band_t){
        .gain_db = 0.0f,
        .enabled = false,
        .frequency_hz = 1000.0f,
        .q = 1.0f,
        .type = EQ_TYPE_PEAK,
    };
}

static void init_default_input_state(input_state_t *state) {
    state->gain_db = 0.0f;
    state->muted = false;

    for (int i = 0; i < PEQ_BANDS; i++) {
        init_default_peq_band(&state->peq[i]);
    }
}

static void init_default_output_state(output_state_t *state) {
    state->gain_db = 0.0f;
    state->muted = false;

    for (int i = 0; i < PEQ_BANDS; i++) {
        init_default_peq_band(&state->peq[i]);
    }
}

esp_err_t device_state_init(void) {
    memset(&s_state, 0, sizeof(s_state));

    for (int i = 0; i < INPUT_COUNT; i++) {
        init_default_input_state(&s_state.dsp.inputs[i]);
    }

    for (int i = 0; i < OUTPUT_COUNT; i++) {
        init_default_output_state(&s_state.dsp.outputs[i]);
    }

    s_state.active_preset = INVALID_PRESET;
    s_state.preset_modified = false;

    return ESP_OK;
}

const device_state_t *device_state_get(void) {
    return &s_state;
}
