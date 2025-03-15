idf_component_register(SRCS     "espnow_water.c" 
                                "main.c" 
                                "i2c_lcd.c" 
                                "wifi.c" 
                                "sntp.c" 
                                "AM2302.c" 
                                "climate_control.c" 
                                "menu_LCD.c" 
                                "nvs_manager.c" 
                                "server.c"
                                "mqtt.c"
                                
                        EMBED_FILES html/dash.html html/config.html html/riego.html
                        INCLUDE_DIRS ".")
