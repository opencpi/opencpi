#!/bin/csh -f
# quick hack to run all tests
# run this in the binary executables directory
set t =  `uname -s`-`uname -m`
setenv OCPI_RCC_TARGET `echo $t | tr A-Z a-z`
if $OCPI_RCC_TARGET == darwin-i386 then
  setenv OCPI_RCC_SUFFIX dylib
else
  setenv OCPI_RCC_SUFFIX so
endif
if $#argv == 1 then
  ./$argv[1] | grep Test:
else
foreach i ( `ls test* | grep -v '_main' | grep -v '.obj'`)
  echo Running $i...
  ./$i | grep Test:
end
endif

