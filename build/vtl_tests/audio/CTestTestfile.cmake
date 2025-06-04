# CMake generated Testfile for 
# Source directory: C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio
# Build directory: C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/build/vtl_tests/audio
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[VTL_tests_audio_Run]=] "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/app/Debug/VTL_tests_Audio.exe")
  set_tests_properties([=[VTL_tests_audio_Run]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;13;add_test;C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[VTL_tests_audio_Run]=] "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/app/Release/VTL_tests_Audio.exe")
  set_tests_properties([=[VTL_tests_audio_Run]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;13;add_test;C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[VTL_tests_audio_Run]=] "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/app/MinSizeRel/VTL_tests_Audio.exe")
  set_tests_properties([=[VTL_tests_audio_Run]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;13;add_test;C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[VTL_tests_audio_Run]=] "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/app/RelWithDebInfo/VTL_tests_Audio.exe")
  set_tests_properties([=[VTL_tests_audio_Run]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;13;add_test;C:/Users/kl3wd/OneDrive/Рабочий стол/VTL_Tim_build/vtl_tests/audio/CMakeLists.txt;0;")
else()
  add_test([=[VTL_tests_audio_Run]=] NOT_AVAILABLE)
endif()
