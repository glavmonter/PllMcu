#[=======================================================================[.rst:
set_target_options
------------------

Установка параметров линковки, создание hex, bin прошивок и добавление контрольной суммы к hex (если есть python)

Аргументы
^^^^^^^^^

- ``TARGET`` - имя target
- ``LD_SCRIPT`` - путь до скрипта линковки
- ``FLASH_START`` - адрес начала FLASH памяти, например 0x08000000
- ``FLASH_SIZE`` - размер FLASH памяти в байтах

#]=======================================================================]

function(set_target_options TARGET LD_SCRIPT FLASH_START FLASH_SIZE)
    target_link_options(${TARGET}.elf PRIVATE -T ${LD_SCRIPT} -Wl,-Map=${PROJECT_BINARY_DIR}/${TARGET}.map)
    add_custom_command(TARGET ${TARGET}.elf POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET}.elf> ${PROJECT_BINARY_DIR}/${TARGET}.hex
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}.elf> ${PROJECT_BINARY_DIR}/${TARGET}.bin
            COMMENT "Building ${TARGET}.hex Building ${TARGET}.bin")

    add_custom_command(TARGET ${TARGET}.elf POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -S $<TARGET_FILE:${TARGET}.elf> > ${PROJECT_BINARY_DIR}/${TARGET}.S
            COMMENT "Dump listing for ${TARGET}.elf")

    if(Python3_venv_FOUND)

    endif()

    set_property(
            TARGET ${TARGET}.elf
            APPEND
            PROPERTY ADDITIONAL_CLEAN_FILES ${TARGET}.map ${TARGET}.hex ${TARGET}.bin ${TARGET}.S
    )
endfunction(set_target_options)
