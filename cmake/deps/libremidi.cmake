block()
  set(LIBREMIDI_EXAMPLES OFF CACHE "" INTERNAL)
  set(LIBREMIDI_TESTS OFF CACHE "" INTERNAL)

  # Still does not build with clang
  set(LIBREMIDI_NO_WINUWP ON)
  set(LIBREMIDI_NO_WINUWP ON CACHE "" INTERNAL)
  if(EMSCRIPTEN)
    set(LIBREMIDI_HEADER_ONLY ON CACHE "" INTERNAL)
  else()
    set(LIBREMIDI_HEADER_ONLY OFF CACHE "" INTERNAL)
  endif()
  set(WEAKJACK_FOLDER "${OSSIA_3RDPARTY_FOLDER}")
  set(BUILD_SHARED_LIBS 0)
  add_definitions(
    BOOST_NO_RTTI=1
    BOOST_MATH_DISABLE_FLOAT128=1
  )

  if(WIN32)
    if(MSVC)
      add_definitions(BOOST_ASIO_SEPARATE_COMPILATION=1)
    endif()
  endif()
  add_subdirectory("${OSSIA_3RDPARTY_FOLDER}/libremidi" EXCLUDE_FROM_ALL)
endblock()
