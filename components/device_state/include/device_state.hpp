#pragma once

#include <cstdint>

#include "esp_err.h"

#define INPUT_COUNT  4
#define OUTPUT_COUNT 8
#define PEQ_BANDS    10
#define INVALID_PRESET UINT8_MAX

typedef enum {
    EQ_TYPE_PEAK,
} filter_type_t;

typedef struct {
    bool enabled;
    float frequency_hz;
    float gain_db;
    float q;
    filter_type_t type;
} peq_band_t;

typedef struct {
    float gain_db;
    bool muted;

    peq_band_t peq[PEQ_BANDS];
} input_state_t;

typedef struct {
    float gain_db;
    bool muted;

    peq_band_t peq[PEQ_BANDS];
} output_state_t;

typedef struct {
    input_state_t inputs[INPUT_COUNT];
    output_state_t outputs[OUTPUT_COUNT];
} dsp_config_t;

typedef struct {
    char name[32];
    dsp_config_t config;
} preset_t;

typedef struct {
    dsp_config_t dsp;

    uint8_t active_preset;
    bool preset_modified;
} device_state_t;

const device_state_t* device_state_get();

esp_err_t device_state_init();
