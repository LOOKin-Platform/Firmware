idf_component_register(SRCS "owb.c" "owb_gpio.c" "owb_rmt.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "driver esp_rom soc")