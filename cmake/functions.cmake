function(set_options TARGET LSCRIPT)
    target_link_options(${TARGET}.elf PRIVATE -T ${LSCRIPT} -Wl,-Map=${PROJECT_BINARY_DIR}/${TARGET}.map)
    add_custom_command(TARGET ${TARGET}.elf POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET}.elf> ${PROJECT_BINARY_DIR}/${TARGET}.hex
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}.elf> ${PROJECT_BINARY_DIR}/${TARGET}.bin
            COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/${TARGET}.hex  ${PROJECT_BINARY_DIR}/${TARGET}_${VERSION_FULL}.hex
            COMMENT "Building ${TARGET}.hex Building ${TARGET}.bin")

    add_custom_command(TARGET ${TARGET}.elf POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -S $<TARGET_FILE:${TARGET}.elf> > ${PROJECT_BINARY_DIR}/${TARGET}.S
            COMMENT "Dump listing for ${TARGET}.elf")

    set_property(
            TARGET ${TARGET}.elf
            APPEND
            PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}.map ${TARGET}.hex ${TARGET}.bin ${TARGET}.S ${TARGET}.crc.txt
    )
endfunction()
