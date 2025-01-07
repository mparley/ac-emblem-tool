message(STATUS "Fetching wxWidgets...")

FetchContent_Declare(
   wxWidgets
   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
   GIT_SHALLOW ON
   FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(wxWidgets)

include(${wxWidgets_USE_FILE})

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

target_link_libraries(acet-gui PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(acet-gui PRIVATE acet)
target_include_directories(acet-gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})