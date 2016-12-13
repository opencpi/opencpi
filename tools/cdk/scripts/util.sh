# look for the name $1 in the directory $2 in the project path, and set $3 to the result
# return 0 on found, 1 on not found
function findInProjectPath {
  for p in ${OCPI_PROJECT_PATH//:/ } $OCPI_CDK_DIR ; do
    [ -d $p/exports ] && p=$p/exports
    if [ -e $p/$2/$1 ] ; then
      eval ${3}=$p/$2/$1
      return 0
    fi
  done
  return 1
}

# First arg is .mk file to use
# second arg is Make arg to invoke the right output
#    which can be an assignment or a target
# third arg is verbose
function setVarsFromMake {
  local quiet
  [ -z "$3" ] && quiet=1   
  [ -z $(which make 2> /dev/null) ] && {
    [ -n "$3" ] && echo The '"make"' command is not available. 2>&1
    return 1
  }
  eval $(eval make -n -r -s -f $1 $2 \
	 ${quiet:+2>/dev/null} | grep '^[a-zA-Z_][a-zA-Z_]*=')
}






if [ "$1" == __test__ ] ; then
  if eval findInProjectPath $2 $3 result ; then
    echo good result is $result
  else
    echo bad result
  fi
fi
