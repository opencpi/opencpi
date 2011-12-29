cd /opt/opencpi/prerequisites
rm -r -f gtest*
curl -O http://googletest.googlecode.com/files/gtest-1.6.0.zip
unzip gtest-1.6.0.zip
ln -s gtest-1.6.0 gtest
cd gtest-1.6.0
mkdir build
cd build
c++ -m64 -fPIC -I../include -I.. -c ../src/gtest-all.cc
ar -rs libgtest.a gtest-all.o
mkdir ../$OCPI_BUILD_HOST
mkdir ../$OCPI_BUILD_HOST/lib
ln -s ../../build/libgtest.a ../$OCPI_BUILD_HOST/lib
