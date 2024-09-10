set(LIB_NAME cc_imgui_editor)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d-x-imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/invoke.hpp/headers
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COCOS2DX_ROOT_PATH}/cocos
    ${COCOS2DX_ROOT_PATH}/cocos/base
    ${COCOS2DX_ROOT_PATH}/cocos/platform
    ${COCOS2DX_ROOT_PATH}/external
    ${COCOS2DX_ROOT_PATH}/external/glfw3/include/win32
)

if(WINDOWS)
    include_directories(${COCOS2DX_ROOT_PATH}/external/win32-specific/gles/include/OGLES)
endif()

file(GLOB_RECURSE SOURCE
    ${CMAKE_CURRENT_LIST_DIR}/Editor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ImGuiHelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/NodeFactory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/NodeImDrawer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/PropertyImDrawer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Widget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/WidgetFactory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Node2D.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Node3D.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Sprite3D.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/BaseLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/DirectionLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/PointLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/SpotLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Sprite.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Label.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Geometry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets/ImGuiDemo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets/NodeTree.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets/NodeProperties.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets/Viewport.cpp
    ${CMAKE_CURRENT_LIST_DIR}/FileDialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/CommandHistory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/commands/AddNode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/commands/RemoveNode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/commands/CustomCommand.cpp
)
file(GLOB_RECURSE HEADER
    ${CMAKE_CURRENT_LIST_DIR}/Editor.h
    ${CMAKE_CURRENT_LIST_DIR}/ImGuiHelper.h
    ${CMAKE_CURRENT_LIST_DIR}/NodeFactory.h
    ${CMAKE_CURRENT_LIST_DIR}/NodeImDrawer.h
    ${CMAKE_CURRENT_LIST_DIR}/PropertyImDrawer.h
    ${CMAKE_CURRENT_LIST_DIR}/Widget.h
    ${CMAKE_CURRENT_LIST_DIR}/WidgetFactory.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Node2D.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Node3D.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Sprite3D.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/BaseLight.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/DirectionLight.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/PointLight.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/SpotLight.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Sprite.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Label.h
    ${CMAKE_CURRENT_LIST_DIR}/nodes/Geometry.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets/ImGuiDemo.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets/NodeTree.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets/NodeProperties.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets/Viewport.h
    ${CMAKE_CURRENT_LIST_DIR}/FileDialog.h
    ${CMAKE_CURRENT_LIST_DIR}/Command.h
    ${CMAKE_CURRENT_LIST_DIR}/CommandHistory.h
    ${CMAKE_CURRENT_LIST_DIR}/commands/AddNode.h
    ${CMAKE_CURRENT_LIST_DIR}/commands/RemoveNode.h
    ${CMAKE_CURRENT_LIST_DIR}/commands/CustomCommand.h
)

add_library(${LIB_NAME} STATIC ${SOURCE} ${HEADER})
add_dependencies(${LIB_NAME} cocos2d)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cocos2d-x-imgui)
target_link_libraries(${LIB_NAME} cc_imgui)

use_cocos2dx_compile_define(${LIB_NAME})
