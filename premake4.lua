--This solution will have the filter and it's tests
solution "Threebands"
 configurations {"Debug"}
 
 -- The main project
 project "Threebands"
  targetname "threebands"
  kind "WindowedApp"
  language "C"
  files { "*.h", "*.c" }
  links {"m"}
 
  configuration "Debug"
   defines {"DEBUG"}
   flags {"Symbols"}

  configuration "windows"
   includedirs {"/c/Program Files (x86)/Jack/includes", "/c/mingw/include", "/c/mingw/msys/1.0/include", "freeglut/include"}
   libdirs {"/c/Program Files (x86)/Jack/lib", "freeglut/lib"}
   links {"libjack", "OpenGL32", "libfreeglut", "glu32"}
   
  configuration "linux"
   includedirs {"/usr/include", "/usr/local/include/GL", "/usr/include/GL"}
   libdirs {"/usr/local/lib"}
   links {"pthread", "rt", "GL", "GLU", "glut", "jack"}
  
  configuration "macosx"
   includedirs {"/usr/include", "usr/local/include", "/usr/local/include/GL", "/usr/include/GL"}
   links {"OpenGL.framework", "GLUT.framework", "jack"}

  configuration "gmake"
   buildoptions {"-std=c99"}