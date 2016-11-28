# For now this script needs to know where it is, and on some circa 2002 bash versions,
# it can't, hence the extra argument.  This sets up the CDK VARS
source exports/scripts/ocpisetup.sh exports/scripts/ocpisetup.sh
echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_ | sort
