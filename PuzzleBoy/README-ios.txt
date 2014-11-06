0. This program is incompatible with App Store because it's GPL licensed.
   So iOS support is completely experimental. Use at your own risk!
0.5. Assuming you are using Xcode 5.1.
1. Compile Xcode-iOS/SDL/SDL.xcodeproj in SDL2 following instructions in README-ios.txt
   in SDL2.
2. Copy libSDL2.a from ~/Library/Developer/Xcode/DerivedData/SDL-******/Build/Products/***/libSDL2.a.
   (highgly experimental)
3. Download ios-cmake from Google Code or GitHub, use it to compile FreeType, get libfreetype.a.
4. Following 'Using the Simple DirectMedia Layer for iOS - more manual method' in
   README-ios.txt in SDL2 to create a new empty project.
   Add libSDL2.a, libfreetype.a, OpenGLES.framework, CoreAudio.framework, AudioToolbox.framework
   to 'Linked Frameworks and Libraries' in 'General' tab of project properties.
   Add SDL header path, FreeType header path, enet header path to 'Header Search Paths'.
   Add '-Wno-c++11-narrowing' to 'Other C++ flags'.
5. Add source files to project, including enet source file.
6. Compile!

TODO: Add data files to project.

Now manually copy data files to ~/Library/Developer/Xcode/DerivedData/puzzleboy-******/Build/Products/***/***.app/,
it should runs in iOS simulator.
