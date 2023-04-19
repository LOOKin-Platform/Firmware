idf_component_register(
	PRIV_INCLUDE_DIRS
	"${CMAKE_CURRENT_LIST_DIR}/include"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/examples/providers"
	INCLUDE_DIRS
	"${CMAKE_SOURCE_DIR}/partners/drivent"
	"${CMAKE_CURRENT_LIST_DIR}"
	"${CMAKE_CURRENT_LIST_DIR}/Include"
	"${CMAKE_SOURCE_DIR}/main/DataEndpoints/Include"
	"${CMAKE_SOURCE_DIR}/main/Matter/Include"
	"${CMAKE_SOURCE_DIR}/main/Matter/Devices/Include"
	"${CMAKE_SOURCE_DIR}/main/Matter/Override"
	"${CMAKE_SOURCE_DIR}/main/Matter/Connectivity"
	"${CMAKE_SOURCE_DIR}/main/Sensors/Include"
	"${CMAKE_SOURCE_DIR}/main/Commands/Include"
	"$ENV{MATTER_PATH}/src"
	"$ENV{MATTER_PATH}/src/app/util"
	"$ENV{MATTER_PATH}/examples/platform/esp32"
	"$ENV{IDF_PATH}/components/bt/host/nimble/nimble/nimble/host/include"
	SRC_DIRS
	"${CMAKE_SOURCE_DIR}/partners/drivent"
	"${CMAKE_CURRENT_LIST_DIR}"
	"${CMAKE_CURRENT_LIST_DIR}/PoolingHandlers"
	"${CMAKE_CURRENT_LIST_DIR}/Matter"
	"${CMAKE_SOURCE_DIR}/main/Matter/Devices"
	"${CMAKE_SOURCE_DIR}/main/Matter/Override"
	"${CMAKE_SOURCE_DIR}/main/Matter/Connectivity"
	"${CMAKE_SOURCE_DIR}/main/DataEndpoints"
	"${CMAKE_SOURCE_DIR}/main/Sensors"
	"${CMAKE_SOURCE_DIR}/main/Commands"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/zzz_generated/app-common/app-common/zap-generated/attributes"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/zzz_generated/app-common/app-common/zap-generated"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/util"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/reporting"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/access-control-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/administrator-commissioning-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/basic-information"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/bridged-device-basic-information-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/level-control"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/identify-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/diagnostic-logs-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/ethernet-network-diagnostics-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/localization-configuration-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/time-format-localization-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/fixed-label-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/user-label-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/thread-network-diagnostics-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/wifi-network-diagnostics-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/software-diagnostics-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/switch-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/general-diagnostics-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/group-key-mgmt-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/descriptor"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/network-commissioning"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/on-off-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/operational-credentials-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/src/app/clusters/general-commissioning-server"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/examples/platform/esp32/common"
	"${CMAKE_SOURCE_DIR}/third_party/connectedhomeip/examples/providers"
	PRIV_REQUIRES 
	"chip"
	"QRCode"
	"bt"
	"nvs_flash"
	REQUIRES 
	"bt"
	"efuse"
	"Converter"
	"DateTime"
	"esp_timer"
	"esp_hw_support"
	"HTTPClient"
	"libJSON"
	"esp_http_server"
	"WiFi"
	"OTA"
	"HardwareIO"
	"ISR"
	"RMT"
	"IRLib"
	"mqtt"
	"UART"
	"Sleep"
	"mdns"
	"PowerManagement"
	"esp32-ds18b20"
	"esp32-owb"
	"esp-bme280"
	"hdc1080"
	"NimBLE"
	"ds18b20-wrapper"
	"ws2812")


get_filename_component(CHIP_ROOT ${CMAKE_SOURCE_DIR}/third_party/connectedhomeip REALPATH)

include("${CHIP_ROOT}/build/chip/esp32/esp32_codegen.cmake")

chip_app_component_codegen("${CHIP_ROOT}/examples/bridge-app/bridge-common/bridge-app.matter")
chip_app_component_zapgen("${CHIP_ROOT}/examples/bridge-app/bridge-common/bridge-app.zap")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-D CHIP_HAVE_CONFIG_H")
target_compile_options(${COMPONENT_LIB} PUBLIC
           "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>")