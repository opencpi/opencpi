./mixerTest
rc=$?
if [[ $rc != 0 ]]; then
    exit $rc;
fi
./utTime.sh 20
rc=$?
exit $rc
