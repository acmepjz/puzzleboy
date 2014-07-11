1. Follow the instruction in README-android.txt in SDL2
2. Copy (or symlink) project/jni/freetype/ forder in CommanderGenius (a.k.a. SDL1.2 Android) to here
3. Edit freetype/include/freetype/config/ftoption.h to disable zlib
4. Copy (or symlink) data/ folder to assets/data/
5. If the GCC version of NDK is >=4.7, then comment the line 'LOCAL_CFLAGS += -Doverride=' in jni/src/Android.mk
6. Compile!
