include_directories(
  ${RADIO_SRC_DIR}/gui/colorlcd
  ${RADIO_SRC_DIR}/gui/colorlcd/layouts
  ${CMAKE_CURRENT_BINARY_DIR}/fonts
  ${RADIO_SRC_DIR}/thirdparty/libopenui/src
)

file(GLOB THEMES_SRC RELATIVE ${RADIO_SRC_DIR}/gui/colorlcd ${RADIO_SRC_DIR}/gui/colorlcd/themes/*.cpp)
file(GLOB LAYOUTS_SRC RELATIVE ${RADIO_SRC_DIR}/gui/colorlcd ${RADIO_SRC_DIR}/gui/colorlcd/layouts/*.cpp)
file(GLOB WIDGETS_SRC RELATIVE ${RADIO_SRC_DIR}/gui/colorlcd ${RADIO_SRC_DIR}/gui/colorlcd/widgets/*.cpp)

set(GUI_SRC
  ${GUI_SRC}
  str_functions.cpp
  colors.cpp
  lcd.cpp
  splash.cpp
  fonts.cpp
  curves.cpp
  bitmaps.cpp
  theme.cpp
  theme_manager.cpp
  color_editor_popup.cpp
  color_editor.cpp
  color_list.cpp
  preview_window.cpp
  file_carosell.cpp
  listbox.cpp
  tabsgroup.cpp
  page.cpp
  fab_button.cpp
  select_fab_button.cpp
  select_fab_carousel.cpp
  topbar.cpp
  layout.cpp
  widget.cpp
  widget_settings.cpp
  ${THEMES_SRC}
  ${LAYOUTS_SRC}
  ${WIDGETS_SRC}
  #  popups.cpp
  draw_functions.cpp
  #  menus.cpp
  menu_model.cpp
  model_select.cpp
  model_setup.cpp
  model_flightmodes.cpp
  model_inputs.cpp
  model_mixes.cpp
  model_outputs.cpp
  model_curves.cpp
  model_gvars.cpp
  model_logical_switches.cpp
  special_functions.cpp
  model_telemetry.cpp
  menu_radio.cpp
  radio_setup.cpp
  radio_sdmanager.cpp
  radio_tools.cpp
  radio_trainer.cpp
  radio_version.cpp
  radio_hardware.cpp
  radio_diagkeys.cpp
  radio_diaganas.cpp
  radio_theme.cpp
  radio_calibration.cpp
  view_about.cpp
  view_main.cpp
  view_main_menu.cpp
  view_main_decoration.cpp
  view_channels.cpp
  view_logical_switches.cpp
  view_statistics.cpp
  view_text.cpp
  menu_screen.cpp
  screen_setup.cpp
  switch_warn_dialog.cpp
  )

macro(add_gui_src src)
  set(GUI_SRC
    ${GUI_SRC}
    ${src}
    )
endmacro(add_gui_src)

if(LUA)
  add_gui_src(standalone_lua.cpp)
  set(SRC ${SRC} lua/widgets.cpp)
endif()

if(LUA STREQUAL YES)
  add_gui_src(model_mixer_scripts.cpp)
endif()

if(HELI)
  add_gui_src(model_heli.cpp)
endif()

if(PXX2 OR LUA OR MULTIMODULE)
  add_gui_src(radio_tools.cpp)
endif()

if(PXX2 OR MULTIMODULE)
  add_gui_src(radio_spectrum_analyser.cpp)
endif()

if(PXX2)
  add_gui_src(access_settings.cpp)
endif()

if(MULTIMODULE)
  add_gui_src(multi_rfprotos.cpp)
endif()

if(GHOST)
  add_gui_src(radio_ghost_module_config.cpp)
endif()

set(SRC ${SRC} storage/modelslist.cpp)

option(WINDOWS_INSPECT_BORDERS "Draw bounding boxes around windows" OFF)
if(WINDOWS_INSPECT_BORDERS)
  add_definitions(-DWINDOWS_INSPECT_BORDERS)
endif()

# includes libopenui
include(${RADIO_SRC_DIR}/thirdparty/libopenui/src/CMakeLists.txt)
include_directories(gui/libopenui)

add_definitions(-DLIBOPENUI)

foreach(LIBOPENUI_FILE ${LIBOPENUI_SRC})
  set(SRC ${SRC} thirdparty/libopenui/src/${LIBOPENUI_FILE})
endforeach()

add_gui_src(fullscreen_dialog.cpp)
add_gui_src(message_dialog.cpp)
add_gui_src(confirm_dialog.cpp)
add_gui_src(timeedit.cpp)
add_gui_src(curve.cpp)
add_gui_src(sourcechoice.cpp)
add_gui_src(switchchoice.cpp)
add_gui_src(curveedit.cpp)

if(HARDWARE_TOUCH)
  add_gui_src(keyboard_curve.cpp)
endif()
