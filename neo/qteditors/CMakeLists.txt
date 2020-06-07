set(CMAKE_AUTORCC ON)

set(SOURCES
  qteditors_public.h
  precompiled.h
  fhradiant.qrc
  common/common.cpp  
  dialogs/EditorSession.h
  dialogs/EditorSession.cpp
  dialogs/EfxEditor.h
  dialogs/EfxEditor.cpp
  dialogs/EntityBrowser.h
  dialogs/EntityBrowser.cpp
  dialogs/EntityTreeModel.h
  dialogs/EntityTreeModel.cpp
  dialogs/LightEditor.h
  dialogs/LightEditor.cpp  
  dialogs/MaterialBrowser.h
  dialogs/MaterialBrowser.cpp
  dialogs/MaterialTreeModel.h
  dialogs/MaterialTreeModel.cpp
  dialogs/MaterialWindow.h
  dialogs/MaterialWindow.cpp
  dialogs/MaterialsView.h
  dialogs/MaterialsView.cpp
  dialogs/OrthographicCamera.h
  dialogs/OrthographicCamera.cpp
  dialogs/OrthographicRenderPass.h
  dialogs/OrthographicRenderPass.cpp
  dialogs/OrthographicView.h
  dialogs/OrthographicView.cpp
  dialogs/OrthographicWindow.h
  dialogs/OrthographicWindow.cpp
  dialogs/Radiant.h
  dialogs/Radiant.cpp  
  dialogs/RenderPreview.h
  dialogs/RenderPreview.cpp  
  widgets/RenderWidget.h
  widgets/RenderWidget.cpp  
  widgets/RenderWindow.h
  widgets/RenderWindow.cpp  
  widgets/NumEdit.h
  widgets/NumEdit.cpp
  widgets/SliderNumEdit.h
  widgets/SliderNumEdit.cpp  
  widgets/ColorEdit.h
  widgets/ColorEdit.cpp  
  widgets/Vec3Edit.h
  widgets/Vec3Edit.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ads/src)
add_force_include(precompiled.h SOURCES)
add_msvc_precompiled_header(precompiled.h SOURCES)

set(TARGET_NAME "qteditors")
add_definitions(-DGLEW_STATIC -D__DOOM__ -D__DOOM_DLL__)
add_library(${TARGET_NAME} STATIC ${SOURCES})
add_dependencies(${TARGET_NAME} fhGame-x86)
target_link_libraries(${TARGET_NAME} qtadvanceddocking)
qt5_use_modules(${TARGET_NAME} Widgets Svg Xml)

if(MSVC)
  set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER libs)

  source_group(common common/.*)
  source_group(dialogs dialogs/.*)
  source_group(widgets widgets/.*)
endif()

set_cpu_arch(${TARGET_NAME}) 