set(srcs 
	"IRLib.cpp"
    "ACOperand.cpp"
)

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS .
                      REQUIRES "converter")
                      
set(LOG_LOCAL_LEVEL "ESP_LOG_ERROR")
