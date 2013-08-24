LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
../../blake2s-ref.cpp \
../../FileSystem.cpp \
../../GNUGetText.cpp \
../../Level_PathFinding.cpp \
../../main.cpp \
../../MFCSerializer.cpp \
../../MyFormat.cpp \
../../MySerializer.cpp \
../../PushableBlock.cpp \
../../PuzzleBoyApp.cpp \
../../PuzzleBoyLevel.cpp \
../../PuzzleBoyLevelData.cpp \
../../PuzzleBoyLevelFile.cpp \
../../PuzzleBoyLevelUndo.cpp \
../../RecordManager.cpp \
../../SimpleBitmapFont.cpp \
../../UTF8-16.cpp \
../../VertexList.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -llog

include $(BUILD_SHARED_LIBRARY)
