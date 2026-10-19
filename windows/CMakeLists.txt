# Separate SDR# executable; no SDR++ headers or ABI enter the adapter.
if(NOT WIN32)
  message(FATAL_ERROR "SDR# requires a Windows target/toolchain")
endif()
set(CW_UI_SOURCE "${CMAKE_SOURCE_DIR}/../SDRPlusPlusBrown" CACHE PATH "Pinned Brown ImGui and json.hpp source tree")
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(CW_HOST_BROWN=0 CW_HOST_SDRSHARP=1 NOMINMAX WIN32_LEAN_AND_MEAN _POSIX_C_SOURCE=200809L
  CW_VERSION_STRING="${PROJECT_VERSION}" CW_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  CW_VERSION_MINOR=${PROJECT_VERSION_MINOR} CW_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_compile_options(-Wall -Wextra -Wno-misleading-indentation
  "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=." "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build")
find_package(Threads REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
set(SRC "${CMAKE_SOURCE_DIR}/src")
file(READ "${CMAKE_SOURCE_DIR}/resources/cty.dat" CW_CTY_DATA)
file(READ "${CMAKE_SOURCE_DIR}/resources/country-flags.tsv" CW_FLAG_CODES)
configure_file("${SRC}/country_data.h.in" "${CMAKE_BINARY_DIR}/generated/country_data.h" @ONLY)
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor" "${CMAKE_BINARY_DIR}/vendor")
add_library(radio_metadata STATIC "${SRC}/country_lookup.cpp" "${SRC}/radio_text.cpp")
target_include_directories(radio_metadata PUBLIC "${SRC}" PRIVATE "${CMAKE_BINARY_DIR}/generated")
add_library(cw_core STATIC "${SRC}/decoder.cpp" "${SRC}/timing_preview.cpp" "${SRC}/timing_estimator.cpp"
  "${SRC}/engine.cpp" "${SRC}/ai_handoff.cpp" journal_windows.cpp)
target_include_directories(cw_core PUBLIC "${SRC}")
target_link_libraries(cw_core PUBLIC Threads::Threads)
add_library(notebook_fec STATIC "${SRC}/js8_decoder.cpp" "${SRC}/olivia_decoder.cpp" "${SRC}/fec_worker.cpp")
target_compile_features(notebook_fec PRIVATE cxx_std_20)
target_include_directories(notebook_fec PUBLIC "${SRC}" PRIVATE "${CMAKE_SOURCE_DIR}/vendor/gfsk8/src" "${CMAKE_SOURCE_DIR}/vendor/olivia")
target_link_libraries(notebook_fec PUBLIC notebook_js8 Threads::Threads)
add_library(rtty_core STATIC "${SRC}/rtty_decoder.cpp" "${SRC}/rtty_skimmer.cpp" "${SRC}/rtty_engine.cpp"
  "${SRC}/rtty_report.cpp" "${SRC}/psk_decoder.cpp" "${SRC}/psk_monitor.cpp" "${SRC}/fec_monitor.cpp")
target_include_directories(rtty_core PUBLIC "${SRC}")
target_link_libraries(rtty_core PUBLIC notebook_fec radio_metadata Threads::Threads)
set(IMGUI "${CW_UI_SOURCE}/core/src/imgui")
set(BACKEND "${CW_UI_SOURCE}/core/backends/glfw/imgui")
add_executable(cw_sdrsharp main.cpp notebook_windows.cpp sharp_host.cpp host_adapter_windows.cpp
  rtty_receiver_windows.cpp handoff_windows.cpp flag_textures_windows.cpp imgui_host.cpp
  "${SRC}/plugin_ui.cpp" "${SRC}/plugin_rtty.cpp" "${SRC}/plugin_signals.cpp" "${SRC}/plugin_settings.cpp"
  "${SRC}/plugin_callsigns.cpp" "${SRC}/plugin_help.cpp" "${SRC}/plugin_status.cpp"
  "${SRC}/transcript_view.cpp" "${SRC}/transcript_ink.cpp" "${SRC}/notebook_popups.cpp"
  "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp" "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp"
  "${IMGUI}/imgui_impl_opengl3.cpp" "${BACKEND}/imgui_impl_glfw.cpp")
target_include_directories(cw_sdrsharp BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${CMAKE_CURRENT_SOURCE_DIR}"
  "${IMGUI}" "${BACKEND}" "${CW_UI_SOURCE}/core/src")
target_link_libraries(cw_sdrsharp PRIVATE cw_core rtty_core glfw opengl32 shell32 gdiplus)
enable_testing()
add_executable(cw_windows_tests tests/core.cpp)
target_include_directories(cw_windows_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cw_windows_tests PRIVATE cw_core rtty_core)
add_test(NAME cw_windows_core COMMAND cw_windows_tests)
foreach(test IN ITEMS radio_text_tests metadata_tests snr_activity_tests olivia_noise_tests fec_tests psk_tests rtty_tests)
  add_executable(${test} "${CMAKE_SOURCE_DIR}/tests/${test}.cpp")
  target_link_libraries(${test} PRIVATE cw_core rtty_core)
  add_test(NAME ${test} COMMAND ${test})
endforeach()
target_compile_features(fec_tests PRIVATE cxx_std_20)
target_include_directories(fec_tests PRIVATE "${CMAKE_SOURCE_DIR}/vendor/olivia")
add_executable(cw_transcript_tests "${CMAKE_SOURCE_DIR}/tests/transcript_tests.cpp"
  "${SRC}/transcript_view.cpp" "${SRC}/transcript_ink.cpp" "${SRC}/notebook_popups.cpp"
  "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp" "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp")
target_include_directories(cw_transcript_tests PRIVATE "${SRC}" "${IMGUI}")
target_link_libraries(cw_transcript_tests PRIVATE rtty_core)
add_test(NAME cw_transcript_tests COMMAND cw_transcript_tests)
add_executable(cw_sharp_ipc_tests tests/ipc.cpp sharp_host.cpp)
target_include_directories(cw_sharp_ipc_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CW_UI_SOURCE}/core/src")
target_link_libraries(cw_sharp_ipc_tests PRIVATE Threads::Threads)
