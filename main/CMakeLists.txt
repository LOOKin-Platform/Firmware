idf_component_register(
	PRIV_INCLUDE_DIRS
	"$ENV{MATTER_PATH}/zzz_generated/bridge-app"
	"${CMAKE_SOURCE_DIR}/main/include"
	"${CMAKE_SOURCE_DIR}/main/Matter/include"
	"$ENV{MATTER_PATH}/examples/providers"
	INCLUDE_DIRS
	"${CMAKE_SOURCE_DIR}/main/include"
	"${CMAKE_SOURCE_DIR}/main/Matter/include"
	"${CMAKE_SOURCE_DIR}/main/Matter/Apps"
	"$ENV{MATTER_PATH}/src"
	"$ENV{MATTER_PATH}/src/app/util"
	"$ENV{MATTER_PATH}/third_party/nlio/repo/include"
	"$ENV{MATTER_PATH}/third_party/nlassert/repo/include"
	"$ENV{MATTER_PATH}/examples/platform/esp32"
	SRC_DIRS
	"${CMAKE_CURRENT_LIST_DIR}"
	"${CMAKE_CURRENT_LIST_DIR}/Matter"
	"$ENV{MATTER_PATH}/zzz_generated/bridge-app/zap-generated"
	"$ENV{MATTER_PATH}/zzz_generated/app-common/app-common/zap-generated/attributes"
	"$ENV{MATTER_PATH}/zzz_generated/app-common/app-common/zap-generated"
	"$ENV{MATTER_PATH}/src/app/server"
	"$ENV{MATTER_PATH}/src/app/util"
	"$ENV{MATTER_PATH}/src/app/reporting"
	"$ENV{MATTER_PATH}/src/app/clusters/access-control-server"
	"$ENV{MATTER_PATH}/src/app/clusters/administrator-commissioning-server"
	"$ENV{MATTER_PATH}/src/app/clusters/basic"
	"$ENV{MATTER_PATH}/src/app/clusters/bridged-device-basic-information-server"
	"$ENV{MATTER_PATH}/src/app/clusters/level-control"
	"$ENV{MATTER_PATH}/src/app/clusters/identify-server"
	"$ENV{MATTER_PATH}/src/app/clusters/diagnostic-logs-server"
	"$ENV{MATTER_PATH}/src/app/clusters/ethernet-network-diagnostics-server"
	"$ENV{MATTER_PATH}/src/app/clusters/localization-configuration-server"
	"$ENV{MATTER_PATH}/src/app/clusters/time-format-localization-server"
	"$ENV{MATTER_PATH}/src/app/clusters/fixed-label-server"
	"$ENV{MATTER_PATH}/src/app/clusters/user-label-server"
	"$ENV{MATTER_PATH}/src/app/clusters/thread-network-diagnostics-server"
	"$ENV{MATTER_PATH}/src/app/clusters/wifi-network-diagnostics-server"
	"$ENV{MATTER_PATH}/src/app/clusters/software-diagnostics-server"
	"$ENV{MATTER_PATH}/src/app/clusters/switch-server"
	"$ENV{MATTER_PATH}/src/app/clusters/general-diagnostics-server"
	"$ENV{MATTER_PATH}/src/app/clusters/group-key-mgmt-server"
	"$ENV{MATTER_PATH}/src/app/clusters/descriptor"
	"$ENV{MATTER_PATH}/src/app/clusters/network-commissioning"
	"$ENV{MATTER_PATH}/src/app/clusters/on-off-server"
	"$ENV{MATTER_PATH}/src/app/clusters/operational-credentials-server"
	"$ENV{MATTER_PATH}/src/app/clusters/general-commissioning-server"
	"$ENV{MATTER_PATH}/examples/platform/esp32/route_hook"
	"$ENV{MATTER_PATH}/examples/platform/esp32/common"
	"$ENV{MATTER_PATH}/examples/providers"
	PRIV_REQUIRES chip QRCode bt
	REQUIRES 
	"bt Converter DateTime HTTPClient libJSON esp_http_server WiFi OTA HardwareIO ISR RMT IRLib mqtt UART Sleep mdns PowerManagement esp32-ds18b20 esp32-owb esp-bme280 hdc1080 NimBLE ds18b20-wrapper ws2812")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
target_compile_options(${COMPONENT_LIB} PUBLIC
           "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>")

#flashing_script()