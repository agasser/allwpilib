project(ntcore)

include(CompileWarnings)
include(AddTest)

execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/generate_topics.py ${WPILIB_BINARY_DIR}/ntcore RESULT_VARIABLE generateResult)
if(NOT (generateResult EQUAL "0"))
  # Try python
  execute_process(COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generate_topics.py ${WPILIB_BINARY_DIR}/ntcore RESULT_VARIABLE generateResult)
  if(NOT (generateResult EQUAL "0"))
    message(FATAL_ERROR "python and python3 generate_topics.py failed")
  endif()
endif()

file(GLOB ntcore_native_src
    src/main/native/cpp/*.cpp
    ${WPILIB_BINARY_DIR}/ntcore/generated/main/native/cpp/*.cpp
    src/main/native/cpp/net/*.cpp
    src/main/native/cpp/net3/*.cpp
    src/main/native/cpp/networktables/*.cpp
    src/main/native/cpp/tables/*.cpp)
add_library(ntcore ${ntcore_native_src})
set_target_properties(ntcore PROPERTIES DEBUG_POSTFIX "d")
target_include_directories(ntcore
                PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/native/cpp
                PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/native/include>
                $<BUILD_INTERFACE:${WPILIB_BINARY_DIR}/ntcore/generated/main/native/include>
                            $<INSTALL_INTERFACE:${include_dest}/ntcore>)
wpilib_target_warnings(ntcore)
target_compile_features(ntcore PUBLIC cxx_std_20)
target_link_libraries(ntcore PUBLIC wpinet wpiutil)

set_property(TARGET ntcore PROPERTY FOLDER "libraries")

install(TARGETS ntcore EXPORT ntcore)
install(DIRECTORY src/main/native/include/ DESTINATION "${include_dest}/ntcore")
install(DIRECTORY ${WPILIB_BINARY_DIR}/ntcore/generated/main/native/include/ DESTINATION "${include_dest}/ntcore")

if (WITH_FLAT_INSTALL)
    set (ntcore_config_dir ${wpilib_dest})
else()
    set (ntcore_config_dir share/ntcore)
endif()

configure_file(ntcore-config.cmake.in ${WPILIB_BINARY_DIR}/ntcore-config.cmake )
install(FILES ${WPILIB_BINARY_DIR}/ntcore-config.cmake DESTINATION ${ntcore_config_dir})
install(EXPORT ntcore DESTINATION ${ntcore_config_dir})

# Java bindings
if (WITH_JAVA)
    find_package(Java REQUIRED)
    find_package(JNI REQUIRED)
    include(UseJava)
    set(CMAKE_JAVA_COMPILE_FLAGS "-encoding" "UTF8" "-Xlint:unchecked")

    file(GLOB QUICKBUF_JAR
        ${WPILIB_BINARY_DIR}/wpiutil/thirdparty/quickbuf/*.jar)

    set(CMAKE_JAVA_INCLUDE_PATH wpimath.jar ${QUICKBUF_JAR})

    file(GLOB ntcore_jni_src
        src/main/native/cpp/jni/*.cpp
        ${WPILIB_BINARY_DIR}/ntcore/generated/main/native/cpp/jni/*.cpp)

    file(GLOB_RECURSE JAVA_SOURCES src/main/java/*.java ${WPILIB_BINARY_DIR}/ntcore/generated/*.java)
    set(CMAKE_JNI_TARGET true)

    add_jar(ntcore_jar ${JAVA_SOURCES} INCLUDE_JARS wpiutil_jar OUTPUT_NAME ntcore GENERATE_NATIVE_HEADERS ntcore_jni_headers)

    get_property(NTCORE_JAR_FILE TARGET ntcore_jar PROPERTY JAR_FILE)
    install(FILES ${NTCORE_JAR_FILE} DESTINATION "${java_lib_dest}")

    set_property(TARGET ntcore_jar PROPERTY FOLDER "java")

    add_library(ntcorejni ${ntcore_jni_src})
    wpilib_target_warnings(ntcorejni)
    target_link_libraries(ntcorejni PUBLIC ntcore wpiutil)

    set_property(TARGET ntcorejni PROPERTY FOLDER "libraries")

    if (MSVC)
        install(TARGETS ntcorejni RUNTIME DESTINATION "${jni_lib_dest}" COMPONENT Runtime)
    endif()

    target_link_libraries(ntcorejni PRIVATE ntcore_jni_headers)
    add_dependencies(ntcorejni ntcore_jar)

    install(TARGETS ntcorejni EXPORT ntcorejni)

endif()

if (WITH_JAVA_SOURCE)
    find_package(Java REQUIRED)
    include(UseJava)
    file(GLOB NTCORE_SOURCES src/main/java/edu/wpi/first/networktables/*.java ${WPILIB_BINARY_DIR}/ntcore/generated/*.java)
    add_jar(ntcore_src_jar
    RESOURCES NAMESPACE "edu/wpi/first/networktables" ${NTCORE_SOURCES}
    OUTPUT_NAME ntcore-sources)

    get_property(NTCORE_SRC_JAR_FILE TARGET ntcore_src_jar PROPERTY JAR_FILE)
    install(FILES ${NTCORE_SRC_JAR_FILE} DESTINATION "${java_lib_dest}")

    set_property(TARGET ntcore_src_jar PROPERTY FOLDER "java")
endif()

add_executable(ntcoredev src/dev/native/cpp/main.cpp)
wpilib_target_warnings(ntcoredev)
target_link_libraries(ntcoredev ntcore)

if (WITH_TESTS)
    wpilib_add_test(ntcore src/test/native/cpp)
    target_include_directories(ntcore_test PRIVATE src/main/native/cpp)
    target_link_libraries(ntcore_test ntcore gmock_main wpiutil_testlib)
endif()
