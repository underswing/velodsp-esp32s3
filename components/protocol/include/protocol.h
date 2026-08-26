#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t protocol_handle_message(httpd_req_t *req, const char *data, size_t len);

esp_err_t protocol_send_hello(httpd_req_t *req);
esp_err_t protocol_send_state(httpd_req_t *req, int request_id);
