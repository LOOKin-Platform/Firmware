#message(WARNING "BUGAGA")
#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")

if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOK.in/Firmware/Source/partitions_4mb.csv")	
else()
	unset(CONFIG_ESPTOOLPY_FLASHSIZE_4MB)
	set(CONFIG_ESPTOOLPY_FLASHSIZE_16MB "y")
	set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOK.in/Firmware/Source/partitions_16mb.csv")
endif()


#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")
