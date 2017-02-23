#!/bin/bash --noprofile

# Script to determine what platforms are available, both local and remote,
# and prepare per-platform scripts to run test cases appropriate that platform
# One argument is the relative pathname from the current project to here (cwd)
#set -vx
source $OCPI_CDK_DIR/scripts/util.sh
function getRemote {
  set -o pipefail
  $OCPI_CDK_DIR/scripts/testrunremote.sh \
      $1 $2 $3 "echo ==START== && ocpirun -C --only-platforms" | sed '1,/==START==/d'
  R=$?
  set +o pipefail
  [ $R != 0 ] && echo '    'Failed trying to reach remote system at $1 with user $2 >&2 
  return 0
}

mkdir -p run
ToolsDir=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR
echo 'Probing for available local platforms:'
localplatforms=(`$ToolsDir/ocpirun -C --only-platforms`)
echo '  Local platforms are: '${localplatforms[@]}
if [ -n "${OCPI_REMOTE_TEST_SYSTEMS}" ]; then
  remotesystems=(${OCPI_REMOTE_TEST_SYSTEMS//:/ })
  echo 'Probing for remote platforms with systems specified via OCPI_REMOTE_TEST_SYSTEMS:'
  for s in ${remotesystems[@]}; do
    echo '  Contacting '$s
    x=(${s//=/ })
    plats=($(getRemote ${x[0]} ${x[1]} ${x[2]} 2>run/${x[0]}.log)) || exit 1
    if [ ${#plats[@]} = 0 ]; then
      echo '    No platforms received: probing log is in run/'${x[0]}.log
    else
      echo '    Received: '${plats[@]}
    fi
    remoteplatforms+=(${plats[@]})
    for p in ${plats[@]##*-}; do
      eval ${p}_host=${x[0]} ${p}_user=${x[1]} ${p}_passwd=${x[2]} ${p}_dir=${x[3]}
     done
  done
  if [ -z "$remoteplatforms" ]; then
    echo '  None of the specified (via OCPI_REMOTE_TEST_SYSTEMS) remote systems are available.'
  else
    echo '  Remote platforms are: '${remoteplatforms[@]}
  fi
fi
export OCPI_LIBRARY_PATH=../lib/rcc:gen/assemblies:$OCPI_LIBRARY_PATH
$ToolsDir/ocpigen -v -C ${localplatforms[@]} ${remoteplatforms[@]}
(echo '#!/bin/bash --noprofile'
 echo 'source $OCPI_CDK_DIR/scripts/util.sh'
 for p in ${localplatforms[@]##*-} ${remoteplatforms[@]##*-}; do
   for f in run verify; do
     file=run/$p/$f.sh
     [ -f $file -a ! -x $file ] && chmod a+x $file
   done
   [ -x run/$p/run.sh ] &&
      cat <<-EOF
	if onlyExclude $p "\$OnlyPlatforms" "\$ExcludePlatforms"; then
	  (cd ./run/$p && ./run.sh \$*)
	fi
	EOF
 done
)  > run/runtests.sh
for p in ${remoteplatforms[@]##*-}; do
  for f in run verify; do
    file=run/$p/$f.sh
    [ -f $file -a ! -x $file ] && chmod a+x $file
  done
  phost=${p}_host puser=${p}_user ppasswd=${p}_passwd pdir=${p}_dir
  [ -x run/$p/run.sh ] && {
     cat <<-EOF > run/$p/runremote.sh
	\$OCPI_CDK_DIR/scripts/testrunremote.sh ${!phost} ${!puser} ${!ppasswd} \\
	"cd ${!pdir}/$1/run/$p && \$*"
	EOF
     chmod a+x run/$p/runremote.sh
  }
done
chmod a+x run/*.sh
