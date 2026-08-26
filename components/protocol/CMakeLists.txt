idf_component_register(
        SRCS "protocol.c"
        INCLUDE_DIRS "include"
        REQUIRES
            cjson
            esp_http_server
            device_state
)
