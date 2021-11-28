idf_component_register(SRCS "mqtt_publisher.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES mqtt )
component_compile_options(-std=gnu++17)
