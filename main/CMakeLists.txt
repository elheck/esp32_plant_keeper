idf_component_register(SRCS "watering_node.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES mqtt_publisher connect_wifi nvs_flash)

component_compile_options(-std=gnu++17)