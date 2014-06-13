trap "trap - ERR; break" ERR; for i in 1; do
. env/start.sh

# Build statically, which is sometimes preferable
export OCPI_BUILD_SHARED_LIBRARIES=0
. env/centos6.sh

# If install_opencv.sh has run successfully:
# export OCPI_OPENCV_HOME=/opt/opencpi/prerequisites/opencv/linux-c6-x86_64

. env/finish.sh
done; trap - ERR
