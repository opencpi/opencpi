
# First test whether we are running as the dhcp script
if test "$OCPIDHCPSCRIPT" = "yes"; then
  case $1 in
      (deconfig) echo DHCP is up on $interface; exit 0;;
      (bound) echo DHCP address is now $ip on $interface;;
      (renew) echo DHCP address is renewed as $ip; exit 0;;
      (nak) echo Bad "(nak)" udhcpc response: $message; exit 1;;
      (*) echo Unexpected udhcpc command: $1; exit 1;;
  esac
  # New address is bound
  ifconfig $interface $ip
  echo IP address $ip has been applied to interface $interface.
  exit 0
fi

# Test that we are sourcing and not running in a subshell
case $0 in
 (-*) echo This script is being sourced by the top level shell.  That\'s good.;;
 (*)  echo This script must be sourced by the top level shell, and it\'s not.; exit 1;;
esac

(export OCPIDHCPSCRIPT=yes; udhcpc -n -q -s /mnt/sd/setup.sh)

mkdir -p /mnt/nfs
mount -t nfs -o udp,nolock $1:$2 /mnt/nfs
zed=/mnt/nfs/$3/platforms/zed
cp $zed/libstdc++.so.6 /lib
mknod /dev/xdevcfg c 259 0
source /mnt/nfs/$3/ocpi/ocpisetup.sh /mnt/nfs/$3/ocpi/ocpisetup.sh
