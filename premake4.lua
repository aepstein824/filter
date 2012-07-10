--This solution will have the filter and it's tests
solution "Threebands"
 configurations {"Debug"}
 
 -- The main project
 project "Threebands"
  targetname "threebands"
  kind "WindowedApp"
  language "C"
  files { "*.h", "*.c" }
  links {"jack",  "m"}
 
  configuration "Debug"
   defines {"DEBUG"}
   flags {"Symbols"}
   
  configuration "linux"
   includedirs {"/usr/include", "/usr/local/include/GL", "/usr/include/GL"}
   libdirs {"/usr/local/lib"}
   links {"pthread", "rt", "GL", "GLU", "glut"}
  
  configuration "macosx"
   includedirs {"/usr/include", "usr/local/include", "/usr/local/include/GL", "/usr/include/GL"}
   links {"OpenGL.framework", "GLUT.framework"}

  configuration "gmake"
   buildoptions {"-std=c99"}