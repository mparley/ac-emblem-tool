# The wxWidgets fetching stuff is taken from LukesDevTutorials:
# https://github.com/lszl84/wx_cmake_fetchcontent_template/blob/main/CMakeLists.txt
# Though I'd really recommend just installing wxWidgets instead of building
# it everytime
message(STATUS "Fetching or finding wxWidgets...")
set(wxBUILD_SHARED OFF)
FetchContent_Declare(
   wxWidgets
   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
   GIT_SHALLOW ON
   FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(wxWidgets)

set(SRCS acet-gui.cpp ImagePanel.cpp)
if(APPLE)
    # create bundle on apple compiles
    add_executable(acet-gui MACOSX_BUNDLE ${SRCS})

    # Set a custom plist file for the app bundle - needed for Mac OS Retina display
    set_target_properties(acet-gui PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
else()
    # the WIN32 is needed for Windows in order for it to look for WinMain
    # instead of the main function. This is ignored on other systems,
    # so it works on all platforms
    add_executable(acet-gui WIN32 ${SRCS} main.exe.manifest)
endif()

if(wxWidgets_FOUND)
  message("wxWidgets found on system")
  include(${wxWidgets_USE_FILE})
  target_link_libraries(acet-gui PRIVATE ${wxWidgets_LIBRARIES})
else()
  target_link_libraries(acet-gui PRIVATE wxcore wxnet)
endif()

target_link_libraries(acet-gui PRIVATE acet)
target_include_directories(acet-gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
