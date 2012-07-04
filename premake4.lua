--This solution will have the filter and it's tests
solution "Threebands"
 configurations {"Debug"}
 
 -- The main project
 project "Threebands"
  targetname "threebands"
  kind "WindowedApp"
  language "C"
  files { "*.h", "*.c" }
  includedirs {"/usr/include", "/usr/local/include/GL", "sb/", "/usr/include/GL"}
  libdirs {"/usr/local/lib"}
  links {"jack", "pthread", "rt", "glut", "GL", "GLU", "m"}
 
  configuration "Debug"
   defines {"DEBUG"}
   flags {"Symbols"}

  configuration {"linux","gmake"}
   buildoptions {"-std=c99"}