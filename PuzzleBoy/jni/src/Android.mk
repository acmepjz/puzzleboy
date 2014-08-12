LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
FREETYPE_PATH := ../freetype

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
$(LOCAL_PATH)/$(FREETYPE_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := \
../../blake2s-ref.cpp \
../../ChooseLevelScreen.cpp \
../../ConfigScreen.cpp \
../../FileSystem.cpp \
../../GNUGetText.cpp \
../../Level_PathFinding.cpp \
../../LevelRecordScreen.cpp \
../../main.cpp \
../../MFCSerializer.cpp \
../../MT19937.cpp \
../../MultiTouchManager.cpp \
../../MyFormat.cpp \
../../MySerializer.cpp \
../../PushableBlock.cpp \
../../PuzzleBoyApp.cpp \
../../PuzzleBoyLevel.cpp \
../../PuzzleBoyLevelData.cpp \
../../PuzzleBoyLevelFile.cpp \
../../PuzzleBoyLevelUndo.cpp \
../../PuzzleBoyLevelView.cpp \
../../RandomMapScreen.cpp \
../../RecordManager.cpp \
../../SimpleBitmapFont.cpp \
../../SimpleFont.cpp \
../../SimpleListScreen.cpp \
../../SimpleMiscScreen.cpp \
../../SimpleProgressScreen.cpp \
../../SimpleScrollView.cpp \
../../SimpleText.cpp \
../../SimpleTextBox.cpp \
../../TestRandomLevel.cpp \
../../TestSolver.cpp \
../../UTF8-16.cpp \
../../VertexList.cpp \
$(SDL_PATH)/src/main/android/SDL_android_main.c

# If the GCC version of NDK is >=4.7 then comment next line
LOCAL_CFLAGS += -Doverride= 

LOCAL_SHARED_LIBRARIES := SDL2 freetype

LOCAL_LDLIBS := -lGLESv1_CM -llog

include $(BUILD_SHARED_LIBRARY)
