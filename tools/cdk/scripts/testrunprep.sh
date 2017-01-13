#!/bin/bash --noprofile

# Script to determine what platforms are available, both local and remote,
# and prepare per-platform scripts to run test cases appropriate that platform
# One argument is the relative pathname from the current project to here (cwd)

source $OCPI_CDK_DIR/scripts/util.sh
function getRemote {
  set -o pipefail
  $OCPI_CDK_DIR/scripts/testrunremote.sh \
      $1 $2 $3 "echo ==START== && ocpirun -C --only-platforms" | sed '1,/==START==/d'
  R=$?
  set +o pipefail
  return $R
}

ToolsDir=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR
echo Probing for available local platforms...
localplatforms=(`$ToolsDir/ocpirun -C --only-platforms`)
echo Local platforms are: ${localplatforms[@]}
if [ -n "${OCPI_REMOTE_TEST_SYSTEMS}" ]; then
  remotesystems=(${OCPI_REMOTE_TEST_SYSTEMS//:/ })
  echo Remote systems specified: ${remotesystems[@]}
  echo Probing for available remote platforms...
  for s in $remotesystems; do
    x=(${s//=/ })
    plats=($(getRemote ${x[0]} ${x[1]} ${x[2]})) || exit 1
    remoteplatforms+=(${plats[@]})
    for p in ${plats[@]##*-}; do
      eval ${p}_host=${x[0]} ${p}_user=${x[1]} ${p}_passwd=${x[2]} ${p}_dir=${x[3]}
     done
  done
  echo Remote platforms are: ${remoteplatforms[@]}
fi
export OCPI_LIBRARY_PATH=../lib/rcc:gen/assemblies:$OCPI_LIBRARY_PATH
$ToolsDir/ocpigen -v -C ${localplatforms[@]} ${remoteplatforms[@]}
(echo 'source $OCPI_CDK_DIR/scripts/util.sh'
 for p in ${localplatforms[@]##*-}; do
   for f in run verify; do
     file=run/$p/$f.sh
     [ -f $file -a ! -x $file ] && chmod a+x $file
   done
   [ -x run/$p/run.sh ] &&
      cat <<-EOF
	if ! isPresent $p \$ExcludePlatforms &&
	    ( [ -z "\$OnlyPlatforms" ] || isPresent $p \$OnlyPlatforms ) then
	  echo ================== Executing test cases for local platform $p
	  (cd ./run/$p && ./run.sh)
	fi
	EOF
 done
 for p in ${remoteplatforms[@]##*-}; do
   for f in run verify; do
     file=run/$p/$f.sh
     [ -f $file -a ! -x $file ] && chmod a+x $file
   done
   phost=${p}_host puser=${p}_user ppasswd=${p}_passwd pdir=${p}_dir
   [ -x run/$p/run.sh ] &&
      cat <<-EOF
	if ! isPresent $p \$ExcludePlatforms &&
	    ( [ -z "\$OnlyPlatforms" ] || isPresent $p \$OnlyPlatforms ) then
	  echo ================== Executing test cases for remote platform $p
	  \$OCPI_CDK_DIR/scripts/testrunremote.sh ${!phost} ${!puser} ${!ppasswd} \\
	  "cd ${!pdir}/$1/run/$p && ./run.sh"
	fi
	EOF
 done
) > run/runtests.sh
(echo 'source $OCPI_CDK_DIR/scripts/util.sh'
 for p in ${localplatforms[@]##*-} ${remoteplatforms[@]##*-}; do
   [ -x run/$p/verify.sh ] &&
      cat <<-EOF
	if ! isPresent $p \$ExcludePlatforms &&
	    ( [ -z "\$OnlyPlatforms" ] || isPresent $p \$OnlyPlatforms ) then
	  echo ================== Verifying for platform $p
	  (cd ./run/$p && ./verify.sh)
	fi
	EOF
 done) > run/verifytests.sh     
chmod a+x run/*.sh
chmod a+x run/runtests.sh
