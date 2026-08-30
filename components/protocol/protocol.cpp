#include "protocol.hpp"

#include <array>
#include <string_view>

#include "cJSON.h"
#include "esp_log.h"

#include "device_state.hpp"

namespace protocol {
    namespace {
        constexpr char TAG[] = "protocol";

        const char *filter_type_to_string(device::FilterType type) {
            switch (type) {
                case device::FilterType::PEAK:
                    return "peak";

                default:
                    return "unknown";
            }
        }

        cJSON *serialize_peq_band(const device::PeqBand *band) {
            cJSON *obj = cJSON_CreateObject();
            if (obj == nullptr) return nullptr;

            cJSON_AddBoolToObject(obj, "enabled", band->enabled);
            cJSON_AddNumberToObject(obj, "frequency_hz", band->frequency_hz);
            cJSON_AddNumberToObject(obj, "gain_db", band->gain_db);
            cJSON_AddNumberToObject(obj, "q", band->q);
            cJSON_AddStringToObject(obj, "type", filter_type_to_string(band->type));

            return obj;
        }

        cJSON *serialize_output(const device::OutputState *output) {
            cJSON *obj = cJSON_CreateObject();
            if (obj == nullptr) return nullptr;

            cJSON_AddNumberToObject(obj, "gain_db", output->gain_db);
            cJSON_AddBoolToObject(obj, "muted", output->muted);

            cJSON *peq = cJSON_AddArrayToObject(obj, "peq");
            if (peq == nullptr) {
                cJSON_Delete(obj);
                return nullptr;
            }
            for (const auto &i: output->peq) {
                cJSON *band = serialize_peq_band(&i);

                if (band == nullptr) {
                    cJSON_Delete(obj);
                    return nullptr;
                }

                cJSON_AddItemToArray(peq, band);
            }

            return obj;
        }

        cJSON *serialize_input(const device::InputState *input) {
            cJSON *obj = cJSON_CreateObject();
            if (obj == nullptr) return nullptr;

            cJSON_AddNumberToObject(obj, "gain_db", input->gain_db);
            cJSON_AddBoolToObject(obj, "muted", input->muted);

            cJSON *peq = cJSON_AddArrayToObject(obj, "peq");
            if (peq == nullptr) {
                cJSON_Delete(obj);
                return nullptr;
            }
            for (const auto &i: input->peq) {
                cJSON *band = serialize_peq_band(&i);

                if (band == nullptr) {
                    cJSON_Delete(obj);
                    return nullptr;
                }

                cJSON_AddItemToArray(peq, band);
            }

            return obj;
        }

        cJSON *serialize_dsp_config(const device::DspConfig *config) {
            cJSON *root = cJSON_CreateObject();
            if (root == nullptr) return nullptr;

            cJSON *inputs = cJSON_AddArrayToObject(root, "inputs");
            if (inputs == nullptr) {
                cJSON_Delete(root);
                return nullptr;
            }
            for (const auto &i: config->inputs) {
                cJSON *input = serialize_input(&i);
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
            for (const auto &i: config->outputs) {
                cJSON *output = serialize_output(&i);
                if (output == nullptr) {
                    cJSON_Delete(root);
                    return nullptr;
                }

                cJSON_AddItemToArray(outputs, output);
            }

            return root;
        }

        cJSON *serialize_device_state(const device::State& state) {
            cJSON *root = cJSON_CreateObject();
            if (root == nullptr) return nullptr;

            cJSON_AddNumberToObject(root, "active_preset", state.active_preset);
            cJSON_AddBoolToObject(root, "preset_modified", state.preset_modified);

            cJSON *config = serialize_dsp_config(&state.dsp);
            if (config == nullptr) {
                cJSON_Delete(root);
                return nullptr;
            }

            cJSON_AddItemToObject(root, "dsp", config);

            return root;
        }

        esp_err_t send_json(httpd_req_t *req, const cJSON *json) {
            char *payload = cJSON_PrintUnformatted(json);
            if (payload == nullptr) {
                return ESP_ERR_NO_MEM;
            }

            httpd_ws_frame_t frame = {
                .final = true,
                .fragmented = false,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = reinterpret_cast<uint8_t *>(payload),
                .len = strlen(payload)
            };

            const esp_err_t err = httpd_ws_send_frame(req, &frame);

            cJSON_free(payload);

            return err;
        }

        esp_err_t send_error(httpd_req_t *req, int request_id, const char *message) {
            cJSON *root = cJSON_CreateObject();
            if (root == nullptr) {
                return ESP_ERR_NO_MEM;
            }

            if (request_id >= 0) cJSON_AddNumberToObject(root, "id", request_id);
            cJSON_AddStringToObject(root, "type", "error");
            cJSON_AddStringToObject(root, "error", message);

            const esp_err_t err = send_json(req, root);
            cJSON_Delete(root);

            return err;
        }

        esp_err_t send_ok(httpd_req_t *req, int request_id) {
            cJSON *root = cJSON_CreateObject();
            if (root == nullptr) {
                return ESP_ERR_NO_MEM;
            }

            cJSON_AddNumberToObject(root, "id", request_id);
            cJSON_AddStringToObject(root, "type", "ok");

            const esp_err_t err = send_json(req, root);
            cJSON_Delete(root);

            return err;
        }

        esp_err_t handle_get_state(httpd_req_t *req, const int request_id, const cJSON *root) {
            (void) root;
            return send_state(req, request_id);
        }

        esp_err_t handle_set_output_gain(httpd_req_t *req, const int request_id, const cJSON *root) {
            const cJSON *output = cJSON_GetObjectItemCaseSensitive(root, "output");
            const cJSON *gain = cJSON_GetObjectItemCaseSensitive(root, "gain_db");

            if (!cJSON_IsNumber(output) || !cJSON_IsNumber(gain)) {
                return send_error(req, request_id, "invalid_arguments");
            }

            const int output_index = output->valueint;
            const auto gain_db = static_cast<float>(gain->valuedouble);

            switch (device::set_output_gain(output_index, gain_db)) {
                case device::DeviceError::Ok:
                    return send_ok(req, request_id);

                case device::DeviceError::InvalidOutput:
                    return send_error(req, request_id, "invalid_output");

                case device::DeviceError::GainOutOfRange:
                    return send_error(req, request_id, "gain_out_of_range");

                case device::DeviceError::InvalidGain:
                    return send_error(req, request_id, "invalid_gain");
            }

            return send_error(req, request_id, "internal_error");
        }


        using ProtocolHandler = esp_err_t (*)(
            httpd_req_t *req,
            int request_id,
            const cJSON *root
        );

        struct ProtocolCommand {
            std::string_view type;
            ProtocolHandler handler;
        };

        constexpr std::array COMMANDS{
            ProtocolCommand{"get_state", handle_get_state},
            ProtocolCommand{"set_output_gain", handle_set_output_gain},
        };

        esp_err_t dispatch_command(httpd_req_t *req, int request_id, const cJSON *root, std::string_view type) {
            for (const auto &command: COMMANDS) {
                if (command.type == type) {
                    return command.handler(req, request_id, root);
                }
            }

            return send_error(req, request_id, "unknown_command");
        }
    }

    esp_err_t send_hello(httpd_req_t *req) {
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

    esp_err_t send_state(httpd_req_t *req, const int request_id) {
        cJSON *root = cJSON_CreateObject();
        if (root == nullptr) {
            return ESP_ERR_NO_MEM;
        }

        cJSON_AddNumberToObject(root, "id", request_id);
        cJSON_AddStringToObject(root, "type", "state");

        cJSON *state_json = serialize_device_state(device::get_state());

        if (state_json == nullptr) {
            cJSON_Delete(root);
            return ESP_ERR_NO_MEM;
        }

        cJSON_AddItemToObject(root, "state", state_json);

        const esp_err_t err = send_json(req, root);

        cJSON_Delete(root);

        return err;
    }

    esp_err_t handle_message(httpd_req_t *req, const char *data, size_t len) {
        cJSON *root = cJSON_ParseWithLength(data, len);

        if (root == nullptr) {
            return send_error(req, -1, "invalid_json");
        }

        const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");

        const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");

        if (!cJSON_IsString(type)) {
            cJSON_Delete(root);
            return send_error(req, -1, "missing_type");
        }

        if (!cJSON_IsNumber(id)) {
            cJSON_Delete(root);
            return send_error(req, -1, "missing_id");
        }

        const int request_id = id->valueint;

        const esp_err_t err = dispatch_command(req, request_id, root, type->valuestring);

        cJSON_Delete(root);

        return err;
    }
}
