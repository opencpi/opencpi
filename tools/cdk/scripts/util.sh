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

function isPresent {
    local key=$1
    shift
    local vals=($*)
    for i in $*; do if [ "$key" = "$i" ]; then return 0; fi; done
    return 1
}

# Is $1 ok with $2 being "only" and $3 being "exclude"
function onlyExclude {
  local key=$1
  local only=$2
  local exclude=$3
  shift
  if ! isPresent $key $exclude && ( [ -z "$only" ] || isPresent $key $only ) then
     return 0
  fi
  return 1	 
}

# This is a copy of a function from makeExportLinks.sh, due to bootstrapping issues
# FIXME: allow makeExportLinks.sh to use this one
function makeRelativeLink {
  # echo make_relative_link $1 $2
  # Figure out what the relative prefix should be
  up=$(echo $2 | sed 's-[^/]*$--' | sed 's-[^/]*/-../-g')
  link=${up}$1
  if [ -L $2 ]; then
    L=$(ls -l $2|sed 's/^.*-> *//')
    if [ "$L" = "$link" ]; then
      # echo Symbolic link already correct from $2 to $1.
      return 0
    else
      echo Symbolic link wrong from $2 to $1 wrong \(was $L\), replacing it.
      rm $2
    fi
  elif [ -e $2 ]; then
    if [ -d $2 ]; then
      echo Link $2 already exists, as a directory.
    else
      echo Link $2 already exists, as a regular file.
    fi
    echo '   ' when trying to link to $1
    exit 1
  fi
  mkdir -p $(dirname $2)
  # echo ln -s $link $2
  ln -s $link $2
}



if [ "$1" == __test__ ] ; then
  if eval findInProjectPath $2 $3 result ; then
    echo good result is $result
  else
    echo bad result
  fi
fi
