# Pinned receive DSP sources; see UPSTREAM.md and the project notices.
add_library(notebook_js8 STATIC
  gfsk8/src/JS8.cpp gfsk8/src/JS8Submode.cpp gfsk8/src/Varicode.cpp
  gfsk8/src/JSC.cpp gfsk8/src/JSC_map.cpp gfsk8/src/JSC_list.cpp
  gfsk8/src/DecodedText.cpp gfsk8/src/api.cpp
  gfsk8/vendor/kissfft/kiss_fft.c gfsk8/vendor/kissfft/kiss_fftr.c)
target_compile_features(notebook_js8 PRIVATE cxx_std_20)
target_include_directories(notebook_js8 PUBLIC gfsk8/include PRIVATE gfsk8/src gfsk8/vendor)
set_target_properties(notebook_js8 PROPERTIES POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden C_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(SANITIZE)
  target_compile_options(notebook_js8 PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(notebook_js8 PUBLIC -fsanitize=address,undefined)
endif()
