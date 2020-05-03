
###############################################################################
######################## GLEW #################################################
###############################################################################
SET(SOURCES
  glew/GL/glew.h
  glew/GL/glxew.h
  glew/GL/wglew.h
  glew/glew.c
)
add_library(glew STATIC ${SOURCES})
IF(MSVC)
  SET_PROPERTY(TARGET glew PROPERTY FOLDER 3rdParty)
ENDIF()

set_cpu_arch(glew)

###############################################################################
######################## JPEG #################################################
###############################################################################
SET(SOURCES
  renderer/jpeg-6/jcapimin.c
  renderer/jpeg-6/jcapistd.c
  renderer/jpeg-6/jccoefct.c
  renderer/jpeg-6/jccolor.c
  renderer/jpeg-6/jcdctmgr.c
  renderer/jpeg-6/jchuff.c
  renderer/jpeg-6/jchuff.h
  renderer/jpeg-6/jcinit.c
  renderer/jpeg-6/jcmainct.c
  renderer/jpeg-6/jcmarker.c
  renderer/jpeg-6/jcmaster.c
  renderer/jpeg-6/jcomapi.c
  renderer/jpeg-6/jconfig.h
  renderer/jpeg-6/jcparam.c
  renderer/jpeg-6/jcphuff.c
  renderer/jpeg-6/jcprepct.c
  renderer/jpeg-6/jcsample.c
  renderer/jpeg-6/jctrans.c
  renderer/jpeg-6/jdapimin.c
  renderer/jpeg-6/jdapistd.c
  renderer/jpeg-6/jdatadst.c
  renderer/jpeg-6/jdatasrc.c
  renderer/jpeg-6/jdcoefct.c
  renderer/jpeg-6/jdcolor.c
  renderer/jpeg-6/jdct.h
  renderer/jpeg-6/jddctmgr.c
  renderer/jpeg-6/jdhuff.c
  renderer/jpeg-6/jdhuff.h
  renderer/jpeg-6/jdinput.c
  renderer/jpeg-6/jdmainct.c
  renderer/jpeg-6/jdmarker.c
  renderer/jpeg-6/jdmaster.c
  renderer/jpeg-6/jdmerge.c
  renderer/jpeg-6/jdphuff.c
  renderer/jpeg-6/jdpostct.c
  renderer/jpeg-6/jdsample.c
  renderer/jpeg-6/jdtrans.c
  renderer/jpeg-6/jerror.c
  renderer/jpeg-6/jerror.h
  renderer/jpeg-6/jfdctflt.c
  renderer/jpeg-6/jfdctfst.c
  renderer/jpeg-6/jfdctint.c
  renderer/jpeg-6/jidctflt.c
  renderer/jpeg-6/jidctfst.c
  renderer/jpeg-6/jidctint.c
  renderer/jpeg-6/jidctred.c
  renderer/jpeg-6/jinclude.h
  #renderer/jpeg-6/jload.c
  renderer/jpeg-6/jmemansi.c
  #renderer/jpeg-6/jmemdos.c
  renderer/jpeg-6/jmemmgr.c
  #renderer/jpeg-6/jmemname.c
  #renderer/jpeg-6/jmemnobs.c
  renderer/jpeg-6/jmemsys.h
  renderer/jpeg-6/jmorecfg.h
  renderer/jpeg-6/jpegint.h
  renderer/jpeg-6/jpeglib.h
  #renderer/jpeg-6/jpegtran.c
  renderer/jpeg-6/jquant1.c
  renderer/jpeg-6/jquant2.c
  renderer/jpeg-6/jutils.c
  renderer/jpeg-6/jversion.h
)
add_library(jpeg-6 STATIC ${SOURCES})
SET_PROPERTY(TARGET jpeg-6 PROPERTY FOLDER 3rdParty)
set_cpu_arch(jpeg-6)

###############################################################################
######################## OGG VORBIS ###########################################
###############################################################################
SET(SOURCES
  sound/OggVorbis/ogg/ogg.h
  sound/OggVorbis/ogg/os_types.h
  sound/OggVorbis/oggsrc/bitwise.c
  sound/OggVorbis/oggsrc/framing.c

  sound/OggVorbis/vorbis/codec.h
  sound/OggVorbis/vorbis/vorbisenc.h
  sound/OggVorbis/vorbis/vorbisfile.h

  sound/OggVorbis/vorbissrc/analysis.c
  sound/OggVorbis/vorbissrc/backends.h
  sound/OggVorbis/vorbissrc/barkmel.c
  sound/OggVorbis/vorbissrc/bitrate.c
  sound/OggVorbis/vorbissrc/bitrate.h
  sound/OggVorbis/vorbissrc/block.c


  sound/OggVorbis/vorbissrc/books/coupled/res_books_stereo.h
  sound/OggVorbis/vorbissrc/books/floor/floor_books.h
  sound/OggVorbis/vorbissrc/books/uncoupled/res_books_uncoupled.h

  sound/OggVorbis/vorbissrc/codebook.c
  sound/OggVorbis/vorbissrc/codebook.h
  sound/OggVorbis/vorbissrc/codec_internal.h
  sound/OggVorbis/vorbissrc/envelope.c
  sound/OggVorbis/vorbissrc/envelope.h
  sound/OggVorbis/vorbissrc/floor0.c
  sound/OggVorbis/vorbissrc/floor1.c
  sound/OggVorbis/vorbissrc/highlevel.h
  sound/OggVorbis/vorbissrc/info.c
  sound/OggVorbis/vorbissrc/lookup.c
  sound/OggVorbis/vorbissrc/lookup.h
  #sound/OggVorbis/vorbissrc/lookups.pl
  sound/OggVorbis/vorbissrc/lookup_data.h
  sound/OggVorbis/vorbissrc/lpc.c
  sound/OggVorbis/vorbissrc/lpc.h
  sound/OggVorbis/vorbissrc/lsp.c
  sound/OggVorbis/vorbissrc/lsp.h
  sound/OggVorbis/vorbissrc/mapping0.c
  sound/OggVorbis/vorbissrc/masking.h
  sound/OggVorbis/vorbissrc/mdct.c
  sound/OggVorbis/vorbissrc/mdct.h
  sound/OggVorbis/vorbissrc/misc.h

  sound/OggVorbis/vorbissrc/modes/floor_all.h
  sound/OggVorbis/vorbissrc/modes/psych_11.h
  sound/OggVorbis/vorbissrc/modes/psych_16.h
  sound/OggVorbis/vorbissrc/modes/psych_44.h
  sound/OggVorbis/vorbissrc/modes/psych_8.h
  sound/OggVorbis/vorbissrc/modes/residue_16.h
  sound/OggVorbis/vorbissrc/modes/residue_44.h
  sound/OggVorbis/vorbissrc/modes/residue_44u.h
  sound/OggVorbis/vorbissrc/modes/residue_8.h
  sound/OggVorbis/vorbissrc/modes/setup_11.h
  sound/OggVorbis/vorbissrc/modes/setup_16.h
  sound/OggVorbis/vorbissrc/modes/setup_22.h
  sound/OggVorbis/vorbissrc/modes/setup_32.h
  sound/OggVorbis/vorbissrc/modes/setup_44.h
  sound/OggVorbis/vorbissrc/modes/setup_44u.h
  sound/OggVorbis/vorbissrc/modes/setup_8.h
  sound/OggVorbis/vorbissrc/modes/setup_X.h
  sound/OggVorbis/vorbissrc/os.h
  sound/OggVorbis/vorbissrc/psy.c
  sound/OggVorbis/vorbissrc/psy.h
  #sound/OggVorbis/vorbissrc/psytune.c
  sound/OggVorbis/vorbissrc/registry.c
  sound/OggVorbis/vorbissrc/registry.h
  sound/OggVorbis/vorbissrc/res0.c
  sound/OggVorbis/vorbissrc/scales.h
  sound/OggVorbis/vorbissrc/sharedbook.c
  sound/OggVorbis/vorbissrc/smallft.c
  sound/OggVorbis/vorbissrc/smallft.h
  sound/OggVorbis/vorbissrc/synthesis.c
  #sound/OggVorbis/vorbissrc/tone.c
  sound/OggVorbis/vorbissrc/vorbisenc.c
  sound/OggVorbis/vorbissrc/vorbisfile.c
  sound/OggVorbis/vorbissrc/window.h
  sound/OggVorbis/vorbissrc/windowvb.c
)
add_library(oggvorbis STATIC ${SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sound/OggVorbis/vorbissrc")
SET_PROPERTY(TARGET oggvorbis PROPERTY FOLDER 3rdParty)

set_cpu_arch(oggvorbis)

###############################################################################
######################## QT TOOLS #############################################
###############################################################################

if(WIN32)
if (NOT "${QTDIR}" STREQUAL "")
  set(CMAKE_PREFIX_PATH ${QTDIR})
  message("QTDIR from options: ${QTDIR}")
else()
  if("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 12 2013")
    set(QTENVNAME "QTDIR_MSVC2013_X86")
  elseif("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 14 2015")
    set(QTENVNAME "QTDIR_MSVC2015_X86")
  elseif("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 15 2017")
    set(QTENVNAME "QTDIR_MSVC2017_X86")
  else()
    set(QTENVNAME "QTDIR")
  endif()

  if(DEFINED ENV{${QTENVNAME}})
	  set(CMAKE_PREFIX_PATH $ENV{${QTENVNAME}})
	  message("QTDIR from environment: $ENV{${QTENVNAME}}")
  endif()
endif()

set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)
if(Qt5Widgets_FOUND)
  add_definitions(-DID_ALLOW_QT)
  add_subdirectory(ads)
  add_subdirectory(qteditors)

  get_target_property(QtCore_location Qt5::Core LOCATION)
  get_target_property(QtWidgets_location Qt5::Widgets LOCATION)
  get_target_property(QtGui_location Qt5::Gui LOCATION)

  SET(QTDLLS
    ${QtCore_location}
    ${QtWidgets_location}
    ${QtGui_location}
  )


  add_custom_target(Qt5CopyBinaries
      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Debug"
      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Debug/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Cored.dll  ${CMAKE_BINARY_DIR}/bin/Debug/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Guid.dll  ${CMAKE_BINARY_DIR}/bin/Debug/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Widgetsd.dll  ${CMAKE_BINARY_DIR}/bin/Debug/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../plugins/platforms/qwindowsd.dll  ${CMAKE_BINARY_DIR}/bin/Debug/platforms/

      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Release"
      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Release/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Core.dll  ${CMAKE_BINARY_DIR}/bin/Release/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Gui.dll  ${CMAKE_BINARY_DIR}/bin/Release/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Widgets.dll  ${CMAKE_BINARY_DIR}/bin/Release/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../plugins/platforms/qwindows.dll  ${CMAKE_BINARY_DIR}/bin/Release/platforms/

      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Core.dll  ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Gui.dll  ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../bin/Qt5Widgets.dll  ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/
      COMMAND ${CMAKE_COMMAND} -E copy ${Qt5Core_DIR}/../../../plugins/platforms/qwindows.dll  ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/platforms/
  )


else()
  message(WARNING "Qt5 not found. Qt based tools are excluded from the build")
endif()
endif()

###############################################################################
######################## ENGINE ###############################################
###############################################################################
SET(SOURCES
  cm/CollisionModel.h
  cm/CollisionModel_contacts.cpp
  cm/CollisionModel_contents.cpp
  cm/CollisionModel_debug.cpp
  cm/CollisionModel_files.cpp
  cm/CollisionModel_load.cpp
  cm/CollisionModel_local.h
  cm/CollisionModel_rotate.cpp
  cm/CollisionModel_trace.cpp
  cm/CollisionModel_translate.cpp
  framework/async/AsyncClient.cpp
  framework/async/AsyncClient.h
  framework/async/AsyncNetwork.cpp
  framework/async/AsyncNetwork.h
  framework/async/AsyncServer.cpp
  framework/async/AsyncServer.h
  framework/async/MsgChannel.cpp
  framework/async/MsgChannel.h
  framework/async/NetworkSystem.cpp
  framework/async/NetworkSystem.h
  framework/async/ServerScan.cpp
  framework/async/ServerScan.h
  framework/BuildDefines.h
  framework/BuildVersion.h
  framework/BuildVersion.cpp
  framework/CmdSystem.cpp
  framework/CmdSystem.h
  framework/Common.cpp
  framework/Common.h
  framework/Compressor.cpp
  framework/Compressor.h
  framework/Console.cpp
  framework/Console.h
  framework/CVarSystem.cpp
  framework/CVarSystem.h
  framework/DeclAF.cpp
  framework/DeclAF.h
  framework/DeclEntityDef.cpp
  framework/DeclEntityDef.h
  framework/DeclFX.cpp
  framework/DeclFX.h
  framework/DeclManager.cpp
  framework/DeclManager.h
  framework/DeclParticle.cpp
  framework/DeclParticle.h
  framework/DeclPDA.cpp
  framework/DeclPDA.h
  framework/DeclSkin.cpp
  framework/DeclSkin.h
  framework/DeclTable.cpp
  framework/DeclTable.h
  framework/DemoChecksum.h
  framework/DemoFile.cpp
  framework/DemoFile.h
  framework/EditField.cpp
  framework/EditField.h
  framework/EventLoop.cpp
  framework/EventLoop.h
  framework/File.cpp
  framework/File.h
  framework/FileSystem.cpp
  framework/FileSystem.h
  framework/KeyInput.cpp
  framework/KeyInput.h
  framework/Licensee.h
  framework/Session.cpp
  framework/Session.h
  framework/Session_local.h
  framework/Session_menu.cpp
  framework/Unzip.cpp
  framework/Unzip.h
  framework/UsercmdGen.cpp
  framework/UsercmdGen.h

  renderer/Cinematic.cpp
  renderer/Cinematic.h
  renderer/draw_common.cpp
  renderer/draw_glsl.cpp

  renderer/GuiModel.cpp
  renderer/GuiModel.h
  renderer/Image.h
  renderer/Image_files.cpp
  renderer/Image_init.cpp
  renderer/Image_load.cpp
  renderer/Image_process.cpp
  renderer/ImageData.h
  renderer/ImageData.cpp
  renderer/ImageProgram.h
  renderer/ImageProgram.cpp
  renderer/Interaction.cpp
  renderer/Interaction.h

  renderer/Framebuffer.h
  renderer/Framebuffer.cpp

  renderer/Material.cpp
  renderer/Material.h
  renderer/MegaTexture.cpp
  renderer/MegaTexture.h
  renderer/Model.cpp
  renderer/Model.h
  renderer/ModelDecal.cpp
  renderer/ModelDecal.h
  renderer/ModelManager.cpp
  renderer/ModelManager.h
  renderer/ModelOverlay.cpp
  renderer/ModelOverlay.h
  renderer/Model_ase.cpp
  renderer/Model_ase.h
  renderer/Model_beam.cpp
  renderer/Model_liquid.cpp
  renderer/Model_local.h
  renderer/Model_lwo.cpp
  renderer/Model_lwo.h
  renderer/Model_ma.cpp
  renderer/Model_ma.h
  renderer/Model_md3.cpp
  renderer/Model_md3.h
  renderer/Model_md5.cpp
  renderer/Model_obj.cpp
  renderer/Model_obj.h
  renderer/Model_prt.cpp
  renderer/Model_sprite.cpp
  renderer/qgl.h
  renderer/RenderMatrix.h
  renderer/RenderMatrix.cpp
  renderer/RenderList.h
  renderer/RenderList_interaction.cpp
  renderer/RenderList_depth.cpp
  renderer/RenderList_stages.cpp
  renderer/RenderList_shadowmap.cpp
  renderer/RenderEntity.cpp
  renderer/RenderSystem.cpp
  renderer/RenderSystem.h
  renderer/RenderSystem_init.cpp
  renderer/RenderWorld.cpp
  renderer/RenderWorld.h
  renderer/RenderProgram.cpp
  renderer/RenderProgram.h
  renderer/RenderWorld_demo.cpp
  renderer/RenderWorld_load.cpp
  renderer/RenderWorld_local.h
  renderer/RenderWorld_portals.cpp
  renderer/Sampler.h
  renderer/Sampler.cpp
  renderer/simplex.h
  renderer/tr_backend.cpp
  renderer/tr_deform.cpp
  renderer/tr_font.cpp
  renderer/tr_guisurf.cpp
  renderer/tr_light.cpp
  renderer/tr_lightrun.cpp
  renderer/tr_local.h
  renderer/tr_main.cpp
  renderer/tr_orderIndexes.cpp
  renderer/tr_polytope.cpp
  renderer/tr_render.cpp
  renderer/tr_rendertools.cpp
  renderer/tr_shadowbounds.cpp
  renderer/tr_shadowmaps.cpp
  renderer/tr_stencilshadow.cpp
  renderer/tr_subview.cpp
  renderer/tr_trace.cpp
  renderer/tr_trisurf.cpp
  renderer/tr_turboshadow.cpp
  renderer/VertexCache.cpp
  renderer/VertexCache.h
  renderer/wglext.h
  renderer/ImmediateMode.h
  renderer/ImmediateMode.cpp
  renderer/ShadowMapAllocator.h
  renderer/ShadowMapAllocator.cpp

  sound/snd_cache.cpp
  sound/snd_decoder.cpp
  sound/snd_efxfile.cpp
  sound/snd_emitter.cpp
  sound/snd_local.h
  sound/snd_shader.cpp
  sound/snd_system.cpp
  sound/snd_wavefile.cpp
  sound/snd_world.cpp
  sound/sound.h

  #sys/gllog/gl.api
  #sys/gllog/glX.api


  tools/compilers/aas/AASBuild.cpp
  tools/compilers/aas/AASBuild_file.cpp
  tools/compilers/aas/AASBuild_gravity.cpp
  tools/compilers/aas/AASBuild_ledge.cpp
  tools/compilers/aas/AASBuild_local.h
  tools/compilers/aas/AASBuild_merge.cpp
  tools/compilers/aas/AASCluster.cpp
  tools/compilers/aas/AASCluster.h
  tools/compilers/aas/AASFile.cpp
  tools/compilers/aas/AASFile.h
  tools/compilers/aas/AASFileManager.cpp
  tools/compilers/aas/AASFileManager.h
  tools/compilers/aas/AASFile_local.h
  tools/compilers/aas/AASFile_optimize.cpp
  tools/compilers/aas/AASFile_sample.cpp
  tools/compilers/aas/AASReach.cpp
  tools/compilers/aas/AASReach.h
  tools/compilers/aas/Brush.cpp
  tools/compilers/aas/Brush.h
  tools/compilers/aas/BrushBSP.cpp
  tools/compilers/aas/BrushBSP.h
  tools/compilers/compiler_public.h

  tools/compilers/dmap/dmap.cpp
  tools/compilers/dmap/dmap.h
  tools/compilers/dmap/facebsp.cpp
  tools/compilers/dmap/leakfile.cpp
  tools/compilers/dmap/map.cpp
  tools/compilers/dmap/optimize.cpp
  tools/compilers/dmap/optimize_gcc.cpp
  tools/compilers/dmap/output.cpp
  tools/compilers/dmap/portals.cpp
  tools/compilers/dmap/shadowopt3.cpp
  tools/compilers/dmap/tritjunction.cpp
  tools/compilers/dmap/tritools.cpp
  tools/compilers/dmap/ubrush.cpp
  tools/compilers/dmap/usurface.cpp

  tools/compilers/renderbump/renderbump.cpp

  tools/compilers/roqvq/codec.cpp
  tools/compilers/roqvq/codec.h
  tools/compilers/roqvq/gdefs.h
  tools/compilers/roqvq/NSBitmapImageRep.cpp
  tools/compilers/roqvq/quaddefs.h
  tools/compilers/roqvq/roq.cpp
  tools/compilers/roqvq/roq.h
  #tools/compilers/roqvq/roq.m
  tools/compilers/roqvq/roqParam.cpp
  tools/compilers/roqvq/roqParam.h


  ui/BindWindow.cpp
  ui/BindWindow.h
  ui/ChoiceWindow.cpp
  ui/ChoiceWindow.h
  ui/DeviceContext.cpp
  ui/DeviceContext.h
  ui/EditWindow.cpp
  ui/EditWindow.h
  ui/FieldWindow.cpp
  ui/FieldWindow.h
  ui/GameBearShootWindow.cpp
  ui/GameBearShootWindow.h
  ui/GameBustOutWindow.cpp
  ui/GameBustOutWindow.h
  ui/GameSSDWindow.cpp
  ui/GameSSDWindow.h
  ui/GameWindow.cpp
  ui/GameWindow.h
  ui/GuiScript.cpp
  ui/GuiScript.h
  ui/ListGUI.cpp
  ui/ListGUI.h
  ui/ListGUILocal.h
  ui/ListWindow.cpp
  ui/ListWindow.h
  ui/MarkerWindow.cpp
  ui/MarkerWindow.h
  ui/Rectangle.h
  ui/RegExp.cpp
  ui/RegExp.h
  ui/RegExp_old.h
  ui/RenderWindow.cpp
  ui/RenderWindow.h
  ui/SimpleWindow.cpp
  ui/SimpleWindow.h
  ui/SliderWindow.cpp
  ui/SliderWindow.h
  ui/UserInterface.cpp
  ui/UserInterface.h
  ui/UserInterfaceLocal.h
  ui/Window.cpp
  ui/Window.h
  ui/Winvar.cpp
  ui/Winvar.h

  tools/edit_public.h
  tools/edit_stub.cpp

  sys/sys_local.cpp
  sys/sys_local.h
  sys/sys_public.h
)

IF(UNIX)
  SET(SOURCES ${SOURCES}
    sys/posix/posix_net.cpp
    sys/posix/posix_main.cpp
    sys/posix/posix_signal.cpp
    sys/posix/posix_threads.cpp
    sys/linux/stack.cpp
    sys/linux/main.cpp
    sys/stub/util_stub.cpp
    sys/linux/glimp.cpp
    sys/posix/posix_input.cpp
    sys/linux/input.cpp
    sys/linux/libXNVCtrl/NVCtrl.c
    tools/guied/GEWindowWrapper_stub.cpp
    sys/linux/sound_alsa.cpp
    sys/linux/sound.cpp
    )
ENDIF()

IF(WIN32)
  SET(SOURCES ${SOURCES}
    sys/win32/eax.h
    sys/win32/rc/AFEditor_resource.h
    sys/win32/rc/Common_resource.h
    sys/win32/rc/CreateResourceIDs.cpp
    sys/win32/rc/CreateResourceIDs.h
    sys/win32/rc/Debugger_resource.h
    sys/win32/rc/DeclEditor_resource.h    
    sys/win32/rc/doom_resource.h
    sys/win32/rc/GuiEd_resource.h
    sys/win32/rc/MaterialEditor_Resource.h
    sys/win32/rc/ParticleEditor_resource.h
    sys/win32/rc/PDAEditor_resource.h
    sys/win32/rc/PropTree_resource.h
    sys/win32/rc/Radiant_resource.h
    sys/win32/rc/ScriptEditor_resource.h
    sys/win32/rc/SoundEditor_resource.h
    sys/win32/win_cpu.cpp
    sys/win32/win_glimp.cpp
    sys/win32/win_input.cpp
    sys/win32/win_local.h
    sys/win32/win_main.cpp
    sys/win32/win_net.cpp
    sys/win32/win_shared.cpp
    sys/win32/win_snd.cpp
    sys/win32/win_syscon.cpp
    sys/win32/win_taskkeyhook.cpp
    sys/win32/win_wndproc.cpp
  )

  IF(ID_ALLOW_TOOLS)
    SET(SOURCES ${SOURCES}
      tools/af/DialogAF.cpp
      tools/af/DialogAF.h
      tools/af/DialogAFBody.cpp
      tools/af/DialogAFBody.h
      tools/af/DialogAFConstraint.cpp
      tools/af/DialogAFConstraint.h
      tools/af/DialogAFConstraintBallAndSocket.cpp
      tools/af/DialogAFConstraintBallAndSocket.h
      tools/af/DialogAFConstraintFixed.cpp
      tools/af/DialogAFConstraintFixed.h
      tools/af/DialogAFConstraintHinge.cpp
      tools/af/DialogAFConstraintHinge.h
      tools/af/DialogAFConstraintSlider.cpp
      tools/af/DialogAFConstraintSlider.h
      tools/af/DialogAFConstraintSpring.cpp
      tools/af/DialogAFConstraintSpring.h
      tools/af/DialogAFConstraintUniversal.cpp
      tools/af/DialogAFConstraintUniversal.h
      tools/af/DialogAFName.cpp
      tools/af/DialogAFName.h
      tools/af/DialogAFProperties.cpp
      tools/af/DialogAFProperties.h
      tools/af/DialogAFView.cpp
      tools/af/DialogAFView.h

      tools/comafx/CDIB.cpp
      tools/comafx/CDIB.h
      tools/comafx/CPathTreeCtrl.cpp
      tools/comafx/CPathTreeCtrl.h
      tools/comafx/CSyntaxRichEditCtrl.cpp
      tools/comafx/CSyntaxRichEditCtrl.h
      tools/comafx/DialogColorPicker.cpp
      tools/comafx/DialogColorPicker.h
      tools/comafx/DialogGoToLine.cpp
      tools/comafx/DialogGoToLine.h
      tools/comafx/DialogName.cpp
      tools/comafx/DialogName.h
      #tools/comafx/riched20.tlh
      tools/comafx/StdAfx.cpp
      tools/comafx/StdAfx.h
      tools/comafx/VectorCtl.cpp
      tools/comafx/VectorCtl.h

      tools/common/AlphaPopup.cpp
      tools/common/ColorButton.cpp
      tools/common/ColorButton.h
      tools/common/DialogHelpers.h
      tools/common/MaskEdit.cpp
      tools/common/MaskEdit.h
      tools/common/OpenFileDialog.cpp
      tools/common/OpenFileDialog.h
      tools/common/PropertyGrid.cpp
      tools/common/PropertyGrid.h

      tools/common/PropTree/PropTree.cpp
      tools/common/PropTree/PropTree.h
      tools/common/PropTree/PropTreeInfo.cpp
      tools/common/PropTree/PropTreeInfo.h
      tools/common/PropTree/PropTreeItem.cpp
      tools/common/PropTree/PropTreeItem.h
      tools/common/PropTree/PropTreeItemButton.cpp
      tools/common/PropTree/PropTreeItemButton.h
      tools/common/PropTree/PropTreeItemCheck.cpp
      tools/common/PropTree/PropTreeItemCheck.h
      tools/common/PropTree/PropTreeItemColor.cpp
      tools/common/PropTree/PropTreeItemColor.h
      tools/common/PropTree/PropTreeItemCombo.cpp
      tools/common/PropTree/PropTreeItemCombo.h
      tools/common/PropTree/PropTreeItemEdit.cpp
      tools/common/PropTree/PropTreeItemEdit.h
      tools/common/PropTree/PropTreeItemEditButton.cpp
      tools/common/PropTree/PropTreeItemEditButton.h
      tools/common/PropTree/PropTreeItemFileEdit.cpp
      tools/common/PropTree/PropTreeItemFileEdit.h
      tools/common/PropTree/PropTreeItemStatic.cpp
      tools/common/PropTree/PropTreeItemStatic.h
      tools/common/PropTree/PropTreeList.cpp
      tools/common/PropTree/PropTreeList.h
      tools/common/PropTree/PropTreeView.cpp
      tools/common/PropTree/PropTreeView.h
      tools/common/RegistryOptions.cpp
      tools/common/RegistryOptions.h
      #tools/common/RenderBumpFlatDialog.cpp
      #tools/common/RenderBumpFlatDialog.h
      tools/common/RollupPanel.cpp
      tools/common/RollupPanel.h
      tools/common/SpinButton.cpp
      tools/common/SpinButton.h


      #tools/debugger/debugger.cpp
      #tools/debugger/DebuggerApp.cpp
      #tools/debugger/DebuggerApp.h
      #tools/debugger/DebuggerBreakpoint.cpp
      #tools/debugger/DebuggerBreakpoint.h
      #tools/debugger/DebuggerClient.cpp
      #tools/debugger/DebuggerClient.h
      #tools/debugger/DebuggerFindDlg.cpp
      #tools/debugger/DebuggerFindDlg.h
      #tools/debugger/DebuggerMessages.h
      #tools/debugger/DebuggerQuickWatchDlg.cpp
      #tools/debugger/DebuggerQuickWatchDlg.h
      #tools/debugger/DebuggerScript.cpp
      #tools/debugger/DebuggerScript.h
      #tools/debugger/DebuggerServer.cpp
      #tools/debugger/DebuggerServer.h
      #tools/debugger/DebuggerWindow.cpp
      #tools/debugger/DebuggerWindow.h

      tools/decl/DialogDeclBrowser.cpp
      tools/decl/DialogDeclBrowser.h
      tools/decl/DialogDeclEditor.cpp
      tools/decl/DialogDeclEditor.h
      tools/decl/DialogDeclNew.cpp
      tools/decl/DialogDeclNew.h
      #tools/decl/DialogEntityDefEditor.cpp
      #tools/decl/DialogEntityDefEditor.h

      tools/guied/GEApp.cpp
      tools/guied/GEApp.h
      tools/guied/GECheckInDlg.cpp
      tools/guied/GEDeleteModifier.cpp
      tools/guied/GEDeleteModifier.h
      tools/guied/GEHideModifier.cpp
      tools/guied/GEHideModifier.h
      tools/guied/GEInsertModifier.cpp
      tools/guied/GEInsertModifier.h
      tools/guied/GEItemPropsDlg.cpp
      tools/guied/GEItemPropsDlg.h
      tools/guied/GEItemScriptsDlg.cpp
      tools/guied/GEItemScriptsDlg.h
      tools/guied/GEKeyValueModifier.cpp
      tools/guied/GEKeyValueModifier.h
      tools/guied/GEModifier.cpp
      tools/guied/GEModifier.h
      tools/guied/GEModifierGroup.cpp
      tools/guied/GEModifierGroup.h
      tools/guied/GEModifierStack.cpp
      tools/guied/GEModifierStack.h
      tools/guied/GEMoveModifier.cpp
      tools/guied/GEMoveModifier.h
      tools/guied/GENavigator.cpp
      tools/guied/GENavigator.h
      tools/guied/GEOptions.cpp
      tools/guied/GEOptions.h
      tools/guied/GEOptionsDlg.cpp
      tools/guied/GEOptionsDlg.h
      tools/guied/GEProperties.cpp
      tools/guied/GEProperties.h
      tools/guied/GEPropertyPage.cpp
      tools/guied/GEPropertyPage.h
      tools/guied/GESelectionMgr.cpp
      tools/guied/GESelectionMgr.h
      tools/guied/GESizeModifier.cpp
      tools/guied/GESizeModifier.h
      tools/guied/GEStateModifier.cpp
      tools/guied/GEStateModifier.h
      tools/guied/GEStatusBar.cpp
      tools/guied/GEStatusBar.h
      tools/guied/GETransformer.cpp
      tools/guied/GETransformer.h
      tools/guied/GEViewer.cpp
      tools/guied/GEViewer.h
      tools/guied/GEWindowWrapper.cpp
      tools/guied/GEWindowWrapper.h
      #tools/guied/GEWindowWrapper_stub.cpp
      tools/guied/GEWorkspace.cpp
      tools/guied/GEWorkspace.h
      tools/guied/GEWorkspaceFile.cpp
      tools/guied/GEZOrderModifier.cpp
      tools/guied/GEZOrderModifier.h
      tools/guied/guied.cpp

      tools/materialeditor/ConsoleView.cpp
      tools/materialeditor/ConsoleView.h
      tools/materialeditor/FindDialog.cpp
      tools/materialeditor/FindDialog.h
      tools/materialeditor/MaterialDef.cpp
      tools/materialeditor/MaterialDef.h
      tools/materialeditor/MaterialDoc.cpp
      tools/materialeditor/MaterialDoc.h
      tools/materialeditor/MaterialDocManager.cpp
      tools/materialeditor/MaterialDocManager.h
      tools/materialeditor/MaterialEditor.cpp
      tools/materialeditor/MaterialEditor.h
      tools/materialeditor/MaterialEditView.cpp
      tools/materialeditor/MaterialEditView.h
      tools/materialeditor/MaterialModifier.cpp
      tools/materialeditor/MaterialModifier.h
      tools/materialeditor/MaterialPreviewPropView.cpp
      tools/materialeditor/MaterialPreviewPropView.h
      tools/materialeditor/MaterialPreviewView.cpp
      tools/materialeditor/MaterialPreviewView.h
      tools/materialeditor/MaterialPropTreeView.cpp
      tools/materialeditor/MaterialPropTreeView.h
      tools/materialeditor/MaterialTreeView.cpp
      tools/materialeditor/MaterialTreeView.h
      tools/materialeditor/MaterialView.cpp
      tools/materialeditor/MaterialView.h
      tools/materialeditor/MEMainFrame.cpp
      tools/materialeditor/MEMainFrame.h
      tools/materialeditor/MEOptions.cpp
      tools/materialeditor/MEOptions.h
      tools/materialeditor/StageView.cpp
      tools/materialeditor/StageView.h
      tools/materialeditor/ToggleListView.cpp
      tools/materialeditor/ToggleListView.h

      tools/particle/DialogParticleEditor.cpp
      tools/particle/DialogParticleEditor.h

      tools/pda/DialogPDAEditor.cpp
      tools/pda/DialogPDAEditor.h

      tools/radiant/autocaulk.cpp
      tools/radiant/autocaulk.h
      tools/radiant/CameraDrawable.cpp
      tools/radiant/CameraDrawable.h      
      tools/radiant/CameraTargetDlg.cpp
      tools/radiant/CameraTargetDlg.h
      tools/radiant/CamWnd.cpp
      tools/radiant/CamWnd.h
      tools/radiant/CapDialog.cpp
      tools/radiant/CapDialog.h
      tools/radiant/cmdlib.cpp
      tools/radiant/cmdlib.h
      tools/radiant/CommandsDlg.cpp
      tools/radiant/CommandsDlg.h
      tools/radiant/CommentsDlg.cpp
      tools/radiant/CommentsDlg.h
      tools/radiant/ConsoleDlg.cpp
      tools/radiant/ConsoleDlg.h
      tools/radiant/CSG.CPP
      tools/radiant/CurveDlg.cpp
      tools/radiant/CurveDlg.h
      tools/radiant/DialogInfo.cpp
      tools/radiant/DialogInfo.h
      tools/radiant/DialogTextures.cpp
      tools/radiant/DialogTextures.h
      tools/radiant/DialogThick.cpp
      tools/radiant/DialogThick.h
      tools/radiant/DlgCamera.cpp
      tools/radiant/DlgCamera.h
      tools/radiant/DlgEvent.cpp
      tools/radiant/DlgEvent.h
      tools/radiant/DRAG.CPP
      tools/radiant/ECLASS.CPP
      tools/radiant/EditorBrush.cpp
      tools/radiant/EditorBrush.h
      tools/radiant/EditorBrushPrimit.cpp
      tools/radiant/EditorEntity.cpp
      tools/radiant/EditorEntity.h
      tools/radiant/EditorMap.cpp
      tools/radiant/EditorMap.h
      tools/radiant/EditViewDlg.cpp
      tools/radiant/EditViewDlg.h
      tools/radiant/EntityDlg.cpp
      tools/radiant/EntityDlg.h
      tools/radiant/EntityListDlg.cpp
      tools/radiant/EntityListDlg.h
      tools/radiant/EntKeyFindReplace.cpp
      tools/radiant/EntKeyFindReplace.h
      tools/radiant/FindTextureDlg.cpp
      tools/radiant/FindTextureDlg.h
      tools/radiant/GetString.cpp
      tools/radiant/GetString.h
      tools/radiant/GLWidget.cpp
      tools/radiant/GLWidget.h
      tools/radiant/InspectorDialog.cpp
      tools/radiant/InspectorDialog.h
      tools/radiant/LightDlg.cpp
      tools/radiant/LightDlg.h
      tools/radiant/MainFrm.cpp
      tools/radiant/MainFrm.h
      tools/radiant/MapInfo.cpp
      tools/radiant/MapInfo.h
      tools/radiant/MediaPreviewDlg.cpp
      tools/radiant/MediaPreviewDlg.h
      tools/radiant/MRU.CPP
      tools/radiant/MRU.H
      tools/radiant/NewTexWnd.cpp
      tools/radiant/NewTexWnd.h
      tools/radiant/PARSE.CPP
      tools/radiant/PARSE.H
      tools/radiant/PatchDensityDlg.cpp
      tools/radiant/PatchDensityDlg.h
      tools/radiant/PatchDialog.cpp
      tools/radiant/PatchDialog.h
      tools/radiant/PMESH.CPP
      tools/radiant/PMESH.H
      tools/radiant/PointFile.cpp
      tools/radiant/PrefsDlg.cpp
      tools/radiant/PrefsDlg.h
      tools/radiant/PreviewDlg.cpp
      tools/radiant/PreviewDlg.h
      tools/radiant/PropertyList.cpp
      tools/radiant/PropertyList.h
      tools/radiant/QE3.CPP
      tools/radiant/QE3.H
      tools/radiant/QEDEFS.H
      tools/radiant/QERTYPES.H
      tools/radiant/Radiant.cpp
      tools/radiant/Radiant.h
      tools/radiant/RenderCamera.cpp
      tools/radiant/RenderCamera.h      
      tools/radiant/RotateDlg.cpp
      tools/radiant/RotateDlg.h
      tools/radiant/ScaleDialog.cpp
      tools/radiant/ScaleDialog.h
      tools/radiant/SELECT.CPP
      tools/radiant/SELECT.H
      tools/radiant/splines.cpp
      tools/radiant/splines.h
      tools/radiant/SurfaceDlg.cpp
      tools/radiant/SurfaceDlg.h
      tools/radiant/TabsDlg.cpp
      tools/radiant/TabsDlg.h
      tools/radiant/TearoffContainerWindow.cpp
      tools/radiant/TearoffContainerWindow.h
      tools/radiant/TextureBar.cpp
      tools/radiant/TextureBar.h
      tools/radiant/Textures.h
      tools/radiant/Undo.cpp
      tools/radiant/Undo.h
      tools/radiant/VERTSEL.CPP
      tools/radiant/WaitDlg.cpp
      tools/radiant/WaitDlg.h
      tools/radiant/WIN_DLG.CPP
      tools/radiant/WIN_QE3.CPP
      #tools/radiant/WIN_QE3.RC2
      tools/radiant/XYWnd.cpp
      tools/radiant/XYWnd.h
      tools/radiant/Z.CPP
      tools/radiant/Z.H
      tools/radiant/ZClip.cpp
      tools/radiant/ZClip.h
      tools/radiant/ZWnd.cpp
      tools/radiant/ZWnd.h
      tools/radiant/rendertools.h
      tools/radiant/rendertools.cpp
      tools/radiant/GLDrawable.h
      tools/radiant/GLDrawable.cpp
      tools/script/DialogScriptEditor.cpp
      tools/script/DialogScriptEditor.h
      tools/sound/DialogSound.cpp
      tools/sound/DialogSound.h
      tools/sound/DialogSoundGroup.cpp
      tools/sound/DialogSoundGroup.h
    )
  ENDIF()
ENDIF()



IF(MSVC)
  source_group(CM cm/.*)
  source_group(Framework framework/.*)
  source_group(Framework\\async framework/async/.*)
  source_group(Renderer renderer/.*)
  source_group(Renderer\\jpeg6 renderer/jpeg-6/.*)
  source_group(Renderer\\GL renderer/GL/.*)
  source_group(Sound sound/.*)
  source_group(Sound\\ogg sound/OggVorbis/ogg/.*)
  source_group(Sound\\oggsrc sound/OggVorbis/oggsrc/.*)
  source_group(Sound\\vorbis sound/OggVorbis/vorbis/.*)
  source_group(Sound\\vorbissrc sound/OggVorbis/vorbissrc/.*)
  source_group(Sys sys/.*)
  source_group(Sys\\win32 sys/win32/.*)
  source_group(Tools tools/.*)
  source_group(Tools\\AF tools/af/.*)
  source_group(Tools\\ComAfx tools/comafx/.*)
  source_group(Tools\\Common tools/common/.*)
  source_group(Tools\\Compilers tools/compilers/.*)
  source_group(Tools\\Debugger tools/debugger/.*)
  source_group(Tools\\Decl tools/decl/.*)
  source_group(Tools\\Guied tools/guied/.*)
  source_group(Tools\\MaterialEditor tools/materialeditor/.*)
  source_group(Tools\\Particle tools/particle/.*)
  source_group(Tools\\PDA tools/pda/.*)
  source_group(Tools\\Radiant tools/radiant/.*)
  source_group(Tools\\Script tools/script/.*)
  source_group(Tools\\Sound tools/sound/.*)
  source_group(Ui ui/.*)

  SET(WINSDK_LIB_DIR "$ENV{ProgramFiles}/Windows Kits/8.1/Lib/winv6.3/um/x86")
  IF(NOT IS_DIRECTORY ${WINSDK_LIB_DIR})
	message("Windows 8.1 SDK not found: ${WINSDK_LIB_DIR}")
  SET(PROGRAM_FILES_X86 "ProgramFiles(x86)")
	SET(WINSDK_LIB_DIR "$ENV{${PROGRAM_FILES_X86}}/Windows Kits/8.1/Lib/winv6.3/um/x86")

	IF(NOT IS_DIRECTORY ${WINSDK_LIB_DIR})
		message("Windows 8.1 SDK not found either: ${WINSDK_LIB_DIR}")
		message( FATAL_ERROR "Please install Windows 8.1 SDK" )
	ENDIF()
  ENDIF()


  SET(EXTERNAL_LIBS
    opengl32.lib
    Glu32.lib
    Dbghelp.lib
    wsock32.lib
    iphlpapi.lib
    "${CMAKE_CURRENT_SOURCE_DIR}/openal-soft/common.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/openal-soft/openal32.lib"
    "${WINSDK_LIB_DIR}/dinput8.lib"
    "${WINSDK_LIB_DIR}/dsound.lib"
    "${WINSDK_LIB_DIR}/dxguid.lib")
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glew)
add_force_include(precompiled.h SOURCES)
add_msvc_precompiled_header(precompiled.h SOURCES)

IF(MSVC)
  SET(SOURCES ${SOURCES} sys/win32/rc/doom.rc)
ENDIF()

if(UNIX)
   include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sys/linux/oss/include)
   add_definitions(-Dlinux)
   add_definitions(-DXTHREADS)
   FIND_PACKAGE(OpenGL REQUIRED)
   FIND_PACKAGE(OpenAL REQUIRED)

   SET(EXTERNAL_LIBS
    pthread
    dl
    X11
    Xext
    Xxf86vm
    ${OPENAL_LIBRARY}
    ${OPENGL_gl_LIBRARY})

  set(LINK_LIBS
    idlib
#    curl
    glew
    jpeg-6
    oggvorbis
    ${EXTERNAL_LIBS})
else()
  set(LINK_LIBS
    idlib
    curl
    glew
    jpeg-6
    oggvorbis
    ${EXTERNAL_LIBS})
endif()

if(Qt5Widgets_FOUND)
  set(LINK_LIBS ${LINK_LIBS} qteditors)
endif()

set(TARGET_NAME "fhDOOM")
add_definitions(-DGLEW_STATIC -D__DOOM__ -D__DOOM_DLL__)
add_executable(${TARGET_NAME} ${SOURCES})
target_link_libraries(${TARGET_NAME} ${LINK_LIBS})
add_dependencies(${TARGET_NAME} fhGame-x86 fhGame-roe-x86)

if(Qt5Widgets_FOUND)
  add_dependencies(${TARGET_NAME} Qt5CopyBinaries)
  set_property(TARGET ${TARGET_NAME} PROPERTY AUTOMOC OFF)
endif()

if(MSVC)
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")

  set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER exes)

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Debug"
	  COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/Release"
	  COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/neo/openal-soft/OpenAL32.dll  ${CMAKE_BINARY_DIR}/bin/Debug/
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/neo/openal-soft/OpenAL32.dll  ${CMAKE_BINARY_DIR}/bin/Release/
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/neo/openal-soft/OpenAL32.dll  ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/
  )
endif()

set_cpu_arch(${TARGET_NAME})

IF(WIN32)

  SET(DIST_NOPATCH_DIR ${CMAKE_BINARY_DIR}/dist/fhDOOM-nopatch)

  add_custom_target(dist_nopatch
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "fhDOOM-nopatch-$<CONFIG>.zip" --format=zip "fhDOOM-nopatch"
    DEPENDS maya2md5
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
  )

  add_custom_command(TARGET dist_nopatch
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_NOPATCH_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory  $<TARGET_FILE_DIR:fhDOOM> "${DIST_NOPATCH_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${DIST_NOPATCH_DIR}/base"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${DIST_NOPATCH_DIR}/d3xp"
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${DIST_NOPATCH_DIR}/base/pak100fhdoom.pk4" --format=zip "glsl" "guis" "maps/game"

    COMMAND ${CMAKE_COMMAND} -E copy  "${DIST_NOPATCH_DIR}/fhGame-x86.dll" "${DIST_NOPATCH_DIR}/base/fhGame-x86.dll"
    COMMAND ${CMAKE_COMMAND} -E copy  "${DIST_NOPATCH_DIR}/fhGame-roe-x86.dll" "${DIST_NOPATCH_DIR}/d3xp/fhGame-x86.dll"

    COMMAND ${CMAKE_COMMAND} -E remove  "${DIST_NOPATCH_DIR}/fhGame-roe-x86.dll"
    COMMAND ${CMAKE_COMMAND} -E remove  "${DIST_NOPATCH_DIR}/fhGame-x86.dll"

    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/base"
  )



  SET(DIST_DIR ${CMAKE_BINARY_DIR}/dist/fhDOOM)

  add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "fhDOOM-$<CONFIG>.zip" --format=zip "fhDOOM"
    DEPENDS dist_nopatch
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
  )

  add_custom_command(TARGET dist
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory  "${DIST_NOPATCH_DIR}" "${DIST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_SOURCE_DIR}/base/pak005.pk4" "${DIST_DIR}/base/"
    COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_SOURCE_DIR}/base/pak006.pk4" "${DIST_DIR}/base/"
    COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_SOURCE_DIR}/base/pak007.pk4" "${DIST_DIR}/base/"
    COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_SOURCE_DIR}/base/pak008.pk4" "${DIST_DIR}/base/"
    COMMAND ${CMAKE_COMMAND} -E copy  "${CMAKE_SOURCE_DIR}/base/default.cfg" "${DIST_DIR}/base/"    
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/base"
  )


  SET(SDK_DIR ${CMAKE_BINARY_DIR}/dist/fhDOOM-SDK)

  add_custom_target(sdk
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "fhDOOM-SDK-$<CONFIG>.zip" --format=zip "fhDOOM-SDK"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
  )

  add_custom_command(TARGET sdk
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${CMAKE_BINARY_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${SDK_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/game"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/d3xp"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/cm"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/framework/async"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/MayaImport"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/sound"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/sys"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/tools/compilers/aas"
    COMMAND ${CMAKE_COMMAND} -E make_directory   "${SDK_DIR}/src/ui"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "game" "${SDK_DIR}/src/game"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "d3xp" "${SDK_DIR}/src/d3xp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "idlib" "${SDK_DIR}/src/idlib"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/cm/CollisionModel.h" "${SDK_DIR}/src/cm"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/BuildDefines.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/BuildVersion.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/BuildVersion.cpp" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/CmdSystem.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/Common.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/CVarSystem.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclAF.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclEntityDef.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclFX.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclManager.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclParticle.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclPDA.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclSkin.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/DeclTable.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/File.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/FileSystem.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/Licensee.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/UsercmdGen.h" "${SDK_DIR}/src/framework"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/framework/async/NetworkSystem.h" "${SDK_DIR}/src/framework/async"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/MayaImport/maya_main.h" "${SDK_DIR}/src/MayaImport"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/Cinematic.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/Material.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/Model.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/ModelManager.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/RenderSystem.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/renderer/RenderWorld.h" "${SDK_DIR}/src/renderer"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/sound/sound.h" "${SDK_DIR}/src/sound"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/sys/sys_public.h" "${SDK_DIR}/src/sys"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/tools/compilers/aas/AASFile.h" "${SDK_DIR}/src/tools/compilers/aas"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/tools/compilers/aas/AASFileManager.h" "${SDK_DIR}/src/tools/compilers/aas"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/ui/ListGUI.h" "${SDK_DIR}/src/ui"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/neo/ui/UserInterface.h" "${SDK_DIR}/src/ui"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/setup.cmake" "${SDK_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/cmake_msvc2013.cmd" "${SDK_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/cmake_msvc2015.cmd" "${SDK_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/cmake_msvc2017.cmd" "${SDK_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/CMakeLists.sdk.txt" "${SDK_DIR}/src/CMakeLists.txt"

    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/neo"
  )

ENDIF()
