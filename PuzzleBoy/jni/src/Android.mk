LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := $(LOCAL_PATH)/../SDL
FREETYPE_PATH := $(LOCAL_PATH)/../freetype
ENET_PATH := $(LOCAL_PATH)/../../external/enet

LOCAL_C_INCLUDES := \
	$(SDL_PATH)/include \
	$(FREETYPE_PATH)/include \
	$(ENET_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/../../*.cpp) \
	$(SDL_PATH)/src/main/android/SDL_android_main.c \
	$(wildcard $(ENET_PATH)/*.c) \
	)

# for enet
LOCAL_CFLAGS += -DHAS_SOCKLEN_T

# If the GCC version of NDK is >=4.7 then comment next line
LOCAL_CFLAGS += -Doverride= 

LOCAL_SHARED_LIBRARIES := SDL2 freetype

LOCAL_LDLIBS := -lGLESv1_CM -llog

include $(BUILD_SHARED_LIBRARY)
