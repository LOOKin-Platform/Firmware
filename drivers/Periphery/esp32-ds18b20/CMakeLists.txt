set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "ds18b20.c")
set(COMPONENT_PRIV_REQUIRES "esp32-owb driver esp_timer")

register_component()