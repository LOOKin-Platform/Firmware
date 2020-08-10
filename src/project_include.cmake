#message(WARNING "BUGAGA")
#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")

if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_NONE)
		set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOK.in/Firmware/Source/partitions_4mb.csv")	
	else()
		set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOK.in/Firmware/Source/partitions_4mb_homekit.csv")
	endif()
endif()

#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")
