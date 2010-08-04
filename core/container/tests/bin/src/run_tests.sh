#!/bin/csh -f
# quick hack to run all tests
# run this in the binary executables directory
foreach i ( `ls test* | grep -v '_main' | grep -v '.obj'`)
  echo Running $i...
  ./$i | grep Test:
end

