idf_component_register(
    SRCS
        main.c
        dht11_task.c
        global_data.c
        oled_task.c
        wifi_config.c
        alarm_task.c
        mqtt_task.c
    INCLUDE_DIRS "."
)
set(COMPONENT_KCONFIG Kconfig.projbuild)
