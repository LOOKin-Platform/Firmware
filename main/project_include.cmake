#message(WARNING "BUGAGA")
#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")


if (CONFIG_ESPTOOLPY_FLASHSIZE_4MB)
	set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOKin/Firmware/Source/partitions_4mb.csv")	
elseif (CONFIG_ESPTOOLPY_FLASHSIZE_8MB)
	set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOKin/Firmware/Source/partitions_8mb.csv")
else()
	set(PARTITION_CSV_PATH "/Users/thecashit/Documents/LOOKin/Firmware/Source/partitions_16mb.csv")
endif()


#message(WARNING "PARTITION_CSV_PATH (${PARTITION_CSV_PATH})")
