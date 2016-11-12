#!/bin/sh --noprofile
if test -f /etc/redhat-release; then
  read v0 v1 <<EOF
`sed < /etc/redhat-release 's/^\(.\).*release \([0-9]\+\).*/\1 \2/' | tr A-Z a-z`
EOF
  if test "$v0" = "r" -a "$v1" = "5"; then
    echo $1 r5 $2
    exit 0
  fi
  echo Cannot parse redhat release from /etc/redhat-release 1>&2
fi
exit 1


