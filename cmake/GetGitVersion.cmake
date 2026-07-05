find_package(Git)
if(__get_git_version)
  return()
endif()
set(__get_git_version INCLUDED)


function(get_git_version major minor patch short_ver full_ver)
    if (Git_FOUND)
        message("Git found: ${GIT_EXECUTABLE}")
        
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*" --abbrev=8
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE status
            OUTPUT_VARIABLE GIT_VERSION
            ERROR_QUIET)
        
        if(${status})
            set(GIT_VERSION "v0.0.0")
        else()
            string(STRIP ${GIT_VERSION} GIT_VERSION)
            string(REGEX REPLACE "-[0-9]+-g" "-" GIT_VERSION ${GIT_VERSION})
        endif()
        
        string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_VERSION}")
        string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${GIT_VERSION}")
        set(GIT_VERSION_SHORT "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

        # Work out if the repository is dirty
        execute_process(COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_QUIET
            ERROR_QUIET)
        execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DIFF_INDEX
            ERROR_QUIET)
        string(COMPARE NOTEQUAL "${GIT_DIFF_INDEX}" "" GIT_DIRTY)
        if (${GIT_DIRTY})
            set(GIT_VERSION "${GIT_VERSION}-dirty")
        endif()
    else()
        set(GIT_VERSION "v0.0.0")
        set(GIT_VERSION_SHORT "v0.0.0")
        set(VERSION_MAJOR "0")
        set(VERSION_MINOR "0")
        set(VERSION_PATCH "0")
    endif()

    message("-- git Version Short: ${GIT_VERSION_SHORT}")
    message("-- git Version  Full: ${GIT_VERSION}")

    set(${short_ver} ${GIT_VERSION_SHORT} PARENT_SCOPE)
    set(${full_ver} ${GIT_VERSION} PARENT_SCOPE)
    set(${major} ${VERSION_MAJOR} PARENT_SCOPE)
    set(${minor} ${VERSION_MINOR} PARENT_SCOPE)
    set(${patch} ${VERSION_PATCH} PARENT_SCOPE)
endfunction(get_git_version major minor patch short_ver full_ver)
