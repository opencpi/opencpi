
$OCPI_BASE_DIR/runtime/local/util/$OCPI_OUT_DIR/timeCvt --format=CSV --in=timeData.raw --out=timeData.csv
cat timeData.csv | grep ":unit_test:" | grep ",1,0" | grep "Worker Run" > runtime.csv
if [ ! -f "./expectedTime.csv" ] 
 then
    cp ./runtime.csv ./expectedTime.csv;
fi
$OCPI_BASE_DIR/runtime/local/util/$OCPI_OUT_DIR/timeCompare --deviation=$1 --tf=runtime.csv --ef=expectedTime.csv

