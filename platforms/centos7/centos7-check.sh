#!/bin/sh --noprofile
if test -f /etc/centos-release; then
  read v0 v1 <<EOF
`sed < /etc/centos-release 's/^\(.\).*release \([0-9]\+\).*/\1 \2/' | tr A-Z a-z`
EOF
  if test "$v0" = "c" -a "$v1" == "7"; then
    echo $1 c7 $2
    exit 0
  fi
fi
exit 1


