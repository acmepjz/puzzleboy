0. We need a patched version of SDL2.0.0 to make the clipboard work (SDL2.0.3 untested),
   which is derived from https://bugzilla.libsdl.org/show_bug.cgi?id=2266.
   Copy tmp/SDL_android.c to /path-to-SDL2/src/core/android/SDL_android.c.
1. Follow the instruction in README-android.txt in SDL2
2. Copy (or symlink) project/jni/freetype/ forder in CommanderGenius (a.k.a. SDL1.2 Android) to here,
   or you should figure out youtself how to compile FreeType for Android.
3. (Optional) Edit freetype/include/freetype/config/ftoption.h to disable zlib
4. Copy (or symlink) data/ folder to assets/data/
5. (Ignore this step because this project doesn't use any other C++11 features)
   If the GCC version of NDK is >=4.7, then comment the line 'LOCAL_CFLAGS += -Doverride=' in jni/src/Android.mk
6. Compile!
