set(srcs 
	"HomeKit.cpp"
	"App.cpp"
	"DB.cpp"
)

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS "include"
                      REQUIRES "port")
                      
set(LOG_LOCAL_LEVEL "ESP_LOG_ERROR")
