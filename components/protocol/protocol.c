#include "protocol.h"

#include "cJSON.h"
#include "esp_log.h"

#include "device_state.h"

static const char *TAG = "protocol";

static const char *filter_type_to_string(filter_type_t type) {
    switch (type) {
        case EQ_TYPE_PEAK:
            return "peak";

        default:
            return "unknown";
    }
}

static cJSON *serialize_peq_band(const peq_band_t *band) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == nullptr) return nullptr;

    cJSON_AddBoolToObject(obj, "enabled", band->enabled);
    cJSON_AddNumberToObject(obj, "frequency_hz", band->frequency_hz);
    cJSON_AddNumberToObject(obj, "gain_db", band->gain_db);
    cJSON_AddNumberToObject(obj, "q", band->q);
    cJSON_AddStringToObject(obj, "type", filter_type_to_string(band->type));

    return obj;
}

static cJSON *serialize_output(const output_state_t *output) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == nullptr) return nullptr;

    cJSON_AddNumberToObject(obj, "gain_db", output->gain_db);
    cJSON_AddBoolToObject(obj, "muted", output->muted);

    cJSON *peq = cJSON_AddArrayToObject(obj, "peq");
    if (peq == nullptr) {
        cJSON_Delete(obj);
        return nullptr;
    }
    for (int i = 0; i < PEQ_BANDS; i++) {
        cJSON *band = serialize_peq_band(&output->peq[i]);

        if (band == nullptr) {
            cJSON_Delete(obj);
            return nullptr;
        }

        cJSON_AddItemToArray(peq, band);
    }

    return obj;
}

static cJSON *serialize_input(const input_state_t *input) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == nullptr) return nullptr;

    cJSON_AddNumberToObject(obj, "gain_db", input->gain_db);
    cJSON_AddBoolToObject(obj, "muted", input->muted);

    cJSON *peq = cJSON_AddArrayToObject(obj, "peq");
    if (peq == nullptr) {
        cJSON_Delete(obj);
        return nullptr;
    }
    for (int i = 0; i < PEQ_BANDS; i++) {
        cJSON *band = serialize_peq_band(&input->peq[i]);

        if (band == nullptr) {
            cJSON_Delete(obj);
            return nullptr;
        }

        cJSON_AddItemToArray(peq, band);
    }

    return obj;
}

static cJSON *serialize_dsp_config(const dsp_config_t *config) {
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) return nullptr;

    cJSON *inputs = cJSON_AddArrayToObject(root, "inputs");
    if (inputs == nullptr) {
        cJSON_Delete(root);
        return nullptr;
    }
    for (int i = 0; i < INPUT_COUNT; i++) {
        cJSON *input = serialize_input(&config->inputs[i]);
        if (input == nullptr) {
            cJSON_Delete(root);
            return nullptr;
        }

        cJSON_AddItemToArray(inputs, input);
    }

    cJSON *outputs = cJSON_AddArrayToObject(root, "outputs");
    if (outputs == nullptr) {
        cJSON_Delete(root);
        return nullptr;
    }
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        cJSON *output = serialize_output(&config->outputs[i]);
        if (output == nullptr) {
            cJSON_Delete(root);
            return nullptr;
        }

        cJSON_AddItemToArray(outputs, output);
    }

    return root;
}

static cJSON *serialize_device_state(const device_state_t *state) {
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) return nullptr;

    cJSON_AddNumberToObject(root, "active_preset", state->active_preset);
    cJSON_AddBoolToObject(root, "preset_modified", state->preset_modified);

    cJSON *config = serialize_dsp_config(&state->dsp);
    if (config == nullptr) {
        cJSON_Delete(root);
        return nullptr;
    }

    cJSON_AddItemToObject(root, "dsp", config);

    return root;
}

static esp_err_t send_json(httpd_req_t *req, const cJSON *json) {
    char *payload = cJSON_PrintUnformatted(json);
    if (payload == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    httpd_ws_frame_t frame = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)payload,
        .len = strlen(payload)
    };

    const esp_err_t err = httpd_ws_send_frame(req, &frame);

    cJSON_free(payload);

    return err;
}

esp_err_t protocol_send_hello(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "protocol_version", 1);
    cJSON_AddStringToObject(root, "firmware_version", "0.1.0");

    const esp_err_t err = send_json(req, root);

    cJSON_Delete(root);

    return err;
}

esp_err_t protocol_send_state(httpd_req_t *req, const int request_id) {
    const device_state_t *state = device_state_get();

    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "id", request_id);
    cJSON_AddStringToObject(root, "type", "state");

    cJSON *state_json = serialize_device_state(state);

    if (state_json == nullptr) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddItemToObject(root, "state", state_json);

    const esp_err_t err = send_json(req, root);

    cJSON_Delete(root);

    return err;
}
