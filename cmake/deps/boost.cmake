set(BOOST_MINOR_MINIMAL 87)
set(BOOST_MINOR_LATEST 87)

find_package(Boost 1.${BOOST_MINOR_MINIMAL} EXACT QUIET GLOBAL)
if (NOT Boost_FOUND AND NOT OSSIA_USE_SYSTEM_LIBRARIES)
  set(OSSIA_MUST_INSTALL_BOOST 1 CACHE INTERNAL "")
  set(BOOST_VERSION "boost_1_${BOOST_MINOR_LATEST}_0" CACHE INTERNAL "")

  if(NOT EXISTS "${OSSIA_3RDPARTY_FOLDER}/${BOOST_VERSION}/")
    message(STATUS "Downloading boost to ${OSSIA_3RDPARTY_FOLDER}/${BOOST_VERSION}.tar.gz")
    set(BOOST_URL https://github.com/ossia/sdk/releases/download/sdk31/${BOOST_VERSION}.tar.gz)
    set(BOOST_ARCHIVE ${BOOST_VERSION}.tar.gz)

    file(DOWNLOAD "${BOOST_URL}" "${OSSIA_3RDPARTY_FOLDER}/${BOOST_ARCHIVE}")

    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E tar xzf "${BOOST_ARCHIVE}"
      WORKING_DIRECTORY "${OSSIA_3RDPARTY_FOLDER}"
      COMMAND_ERROR_IS_FATAL ANY
    )
  endif()
  set(BOOST_ROOT "${OSSIA_3RDPARTY_FOLDER}/${BOOST_VERSION}")
  set(BOOST_ROOT "${OSSIA_3RDPARTY_FOLDER}/${BOOST_VERSION}" CACHE INTERNAL "")
  set(Boost_INCLUDE_DIR "${BOOST_ROOT}")
  list(PREPEND CMAKE_FIND_ROOT_PATH "${BOOST_ROOT}")

  find_package(Boost 1.${BOOST_MINOR_LATEST} REQUIRED GLOBAL)
endif()

add_library(boost INTERFACE IMPORTED GLOBAL)
set_property(TARGET boost PROPERTY
             INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIR}")
