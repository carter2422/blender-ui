MACRO(BLENDERLIB_NOLIST
  name
  sources
  includes)

  # Gather all headers
  FILE(GLOB_RECURSE INC_ALL *.h)
     
  INCLUDE_DIRECTORIES(${includes})
  ADD_LIBRARY(${name} ${INC_ALL} ${sources})

  # Group by location on disk
  SOURCE_GROUP(Files FILES CMakeLists.txt)
  SET(ALL_FILES ${sources} ${INC_ALL})
  FOREACH(SRC ${ALL_FILES})
    STRING(REGEX REPLACE ${CMAKE_CURRENT_SOURCE_DIR} "Files" REL_DIR "${SRC}")
    STRING(REGEX REPLACE "[\\\\/][^\\\\/]*$" "" REL_DIR "${REL_DIR}")
    STRING(REGEX REPLACE "^[\\\\/]" "" REL_DIR "${REL_DIR}")
    IF(REL_DIR)
      SOURCE_GROUP(${REL_DIR} FILES ${SRC})
    ELSE(REL_DIR)
      SOURCE_GROUP(Files FILES ${SRC})
    ENDIF(REL_DIR)
  ENDFOREACH(SRC)

  MESSAGE(STATUS "Configuring library ${name}")
ENDMACRO(BLENDERLIB_NOLIST)

MACRO(BLENDERLIB
  name
  sources
  includes)

  BLENDERLIB_NOLIST(${name} "${sources}" "${includes}")

  # Add to blender's list of libraries
  FILE(APPEND ${CMAKE_BINARY_DIR}/cmake_blender_libs.txt "${name};")
ENDMACRO(BLENDERLIB)

MACRO(SETUP_LIBDIRS)
  # see "cmake --help-policy CMP0003"
  if(COMMAND cmake_policy)
    CMAKE_POLICY(SET CMP0003 NEW)
  endif(COMMAND cmake_policy)
  
  LINK_DIRECTORIES(${JPEG_LIBPATH} ${PNG_LIBPATH} ${ZLIB_LIBPATH} ${FREETYPE_LIBPATH} ${LIBSAMPLERATE_LIBPATH})
  
  IF(WITH_PYTHON)
    LINK_DIRECTORIES(${PYTHON_LIBPATH})
  ENDIF(WITH_PYTHON)
  IF(WITH_INTERNATIONAL)
    LINK_DIRECTORIES(${ICONV_LIBPATH})
    LINK_DIRECTORIES(${GETTEXT_LIBPATH})
  ENDIF(WITH_INTERNATIONAL)
  IF(WITH_SDL)
    LINK_DIRECTORIES(${SDL_LIBPATH})
  ENDIF(WITH_SDL)
  IF(WITH_FFMPEG)
    LINK_DIRECTORIES(${FFMPEG_LIBPATH})
  ENDIF(WITH_FFMPEG)
  IF(WITH_OPENEXR)
    LINK_DIRECTORIES(${OPENEXR_LIBPATH})
  ENDIF(WITH_OPENEXR)
  IF(WITH_QUICKTIME)
    LINK_DIRECTORIES(${QUICKTIME_LIBPATH})
  ENDIF(WITH_QUICKTIME)
  IF(WITH_OPENAL)
    LINK_DIRECTORIES(${OPENAL_LIBPATH})
  ENDIF(WITH_OPENAL)
  IF(WITH_JACK)
    LINK_DIRECTORIES(${JACK_LIBPATH})
  ENDIF(WITH_JACK)
  IF(WITH_SNDFILE)
    LINK_DIRECTORIES(${SNDFILE_LIBPATH})
  ENDIF(WITH_SNDFILE)
  IF(WITH_FFTW3)
    LINK_DIRECTORIES(${FFTW3_LIBPATH})
  ENDIF(WITH_FFTW3)

  IF(WIN32)
    LINK_DIRECTORIES(${PTHREADS_LIBPATH})
  ENDIF(WIN32)
ENDMACRO(SETUP_LIBDIRS)

MACRO(SETUP_LIBLINKS
  target)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINKFLAGS} ")
  #TARGET_LINK_LIBRARIES(${target} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${PYTHON_LIB} ${PYTHON_LINKFLAGS} ${JPEG_LIB} ${PNG_LIB} ${ZLIB_LIB} ${SDL_LIBRARY} ${LLIBS})

  TARGET_LINK_LIBRARIES(${target} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${PYTHON_LINKFLAGS} ${JPEG_LIBRARY} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${LLIBS})

  # since we are using the local libs for python when compiling msvc projects, we need to add _d when compiling debug versions
  IF(WIN32)
    TARGET_LINK_LIBRARIES(${target} debug ${PYTHON_LIB}_d)
    TARGET_LINK_LIBRARIES(${target} optimized ${PYTHON_LIB})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES(${target} ${PYTHON_LIB})
  ENDIF(WIN32)
  
  TARGET_LINK_LIBRARIES(${target} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${PYTHON_LINKFLAGS} ${JPEG_LIB} ${PNG_LIB} ${ZLIB_LIB} ${LLIBS})
  TARGET_LINK_LIBRARIES(${target} ${FREETYPE_LIBRARY} ${LIBSAMPLERATE_LIB})

  # since we are using the local libs for python when compiling msvc projects, we need to add _d when compiling debug versions

  IF(WIN32)
    TARGET_LINK_LIBRARIES(${target} debug ${PYTHON_LIB}_d)
    TARGET_LINK_LIBRARIES(${target} optimized ${PYTHON_LIB})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES(${target} ${PYTHON_LIB})
  ENDIF(WIN32)

  IF(WITH_INTERNATIONAL)
    TARGET_LINK_LIBRARIES(${target} ${GETTEXT_LIB})
  ENDIF(WITH_INTERNATIONAL)
  IF(WITH_OPENAL)
    TARGET_LINK_LIBRARIES(${target} ${OPENAL_LIBRARY})
  ENDIF(WITH_OPENAL)
  IF(WITH_FFTW3)  
    TARGET_LINK_LIBRARIES(${target} ${FFTW3_LIB})
  ENDIF(WITH_FFTW3)
  IF(WITH_JACK)
    TARGET_LINK_LIBRARIES(${target} ${JACK_LIB})
  ENDIF(WITH_JACK)
  IF(WITH_SNDFILE)
    TARGET_LINK_LIBRARIES(${target} ${SNDFILE_LIB})
  ENDIF(WITH_SNDFILE)
  IF(WITH_SDL)
    TARGET_LINK_LIBRARIES(${target} ${SDL_LIBRARY})
  ENDIF(WITH_SDL)
  IF(WIN32)
    TARGET_LINK_LIBRARIES(${target} ${ICONV_LIB})
  ENDIF(WIN32)
  IF(WITH_QUICKTIME)
    TARGET_LINK_LIBRARIES(${target} ${QUICKTIME_LIB})
  ENDIF(WITH_QUICKTIME)
  IF(WITH_OPENEXR)
    TARGET_LINK_LIBRARIES(${target} ${OPENEXR_LIB})
  ENDIF(WITH_OPENEXR)
  IF(WITH_FFMPEG)
    TARGET_LINK_LIBRARIES(${target} ${FFMPEG_LIB})
  ENDIF(WITH_FFMPEG)
  IF(WIN32)
    TARGET_LINK_LIBRARIES(${target} ${PTHREADS_LIB})
  ENDIF(WIN32)
ENDMACRO(SETUP_LIBLINKS)

