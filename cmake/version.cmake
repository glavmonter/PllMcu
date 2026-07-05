set(APP_VERSION_MAJOR  3)
set(APP_VERSION_MINOR  1)
set(APP_VERSION_PATCH  0)

set(majorminorpatch "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
set(APP_VERSION_DISPLAY "${majorminorpatch}")

message(STATUS "Slave Application Version: ${APP_VERSION_DISPLAY}")
