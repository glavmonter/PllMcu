#[=======================================================================[.rst:
add_python_venv
---------------

Ищет и добавляет интерпетатор Python3 к проекту, как виртуальное окружение (venv).

При запуске в контейнере Docker в venv импортрируются системные пакеты Python и при установке зависимостей не обновляются индексы pip, работает без доступа к интернету.
При запуске не в контейнере создаётся пустой venv и pip устанавливает зависимости из интернета.

Аргументы
^^^^^^^^^

- ``REQUIREMENTS`` - полный путь до файла с зависимостями, устанавливаемыми в venv

Зависимости
^^^^^^^^^^^

Интерпретатор Python3, через ``find_package(Python3 COMPONENTS Interpreter REQUIRED)``

Глобальные переменные
^^^^^^^^^^^^^^^^^^^^^

``USE_DOCKER:BOOL`` для указания, что сборка идет в контейнере Docker
``APP_NAME`` - имя приложения, для которого создаётся venv, может быть пустым

Результат работы
^^^^^^^^^^^^^^^^

Макрос устанавливает следующие переменные:

- ``PYTHON_VENV_EXECUTABLE`` - полный путь до интрепретатора Python3 в виртуальном окружение
- ``PYTHON_VENV_TARGET`` - имя target для генерации venv
- ``Python3_FOUND`` - система имеет установленный Python
- ``Python3_venv_FOUND`` - виртуальное окружение создано

Макрос создаёт следующие target:

- ``venv.stamp.${APP_NAME}``

Применение
^^^^^^^^^^

Пример использования макроса:

``add_python(${CMAKE_SOURCE_DIR}/path_to/requirements.txt)
add_dependencies(${TARGET_NAME} ${PYTHON_VENV_TARGET})
``
#]=======================================================================]
macro(add_python_venv REQUIREMENTS)
    if(USE_DOCKER)
        # В Докере может не быть интернета, в venv переносим системные пакеты python и уставаливаем зависимости без обновления online индексов
        set(PYTHON_VENV_SYSTEM "--system-site-packages")
        set(PYTHON_VENV_PIP "--no-index")
    else()
        # При сборке на нормальной системе в venv загружаем зависимости из интернета
        set(PYTHON_VENV_PIP)
    endif()

    # Ищем Python. Необходим для генерации промежуточных файлов
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(PYTHON_VENV_TARGET "venv.stamp.${APP_NAME}")

    if (CMAKE_HOST_WIN32)
        set(PYTHON_VENV_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/venv/Scripts/python)
    else()
        set(PYTHON_VENV_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/python)
    endif()

    add_custom_command(
        OUTPUT venv
        COMMAND ${Python3_EXECUTABLE} -m venv ${PYTHON_VENV_SYSTEM} venv
        COMMENT "Generating ${PYTHON_VENV_TARGET}"
    )
    add_custom_target(
        ${PYTHON_VENV_TARGET}
        DEPENDS venv ${REQUIREMENTS}
        COMMAND ${CMAKE_COMMAND} -E copy ${REQUIREMENTS} requirements.txt
        COMMAND ${PYTHON_VENV_EXECUTABLE} -m pip install -r requirements.txt ${PYTHON_VENV_PIP} 
    )

    set(Python3_venv_FOUND TRUE)

    unset(PYTHON_VENV_SYSTEM)
    unset(PYTHON_VENV_PIP)
endmacro(add_python_venv)
