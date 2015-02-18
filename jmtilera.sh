trap "trap - ERR; break" ERR; for i in 1; do
source env/start.sh

source platforms/tilera/tilera-env.sh

export OCPI_HAVE_TILERA=1

export TILERA_ROOT=/home/work/kulp/tile3.3/install/TileraMDE-4.3.3.184581/tilegx

export WORKQUEUE_INCLUDE=/home/tools/gcd/tilera/libpwq-master/include/
export WORKQUEUE_LIB=/home/tools/gcd/tilera/libpwq-master/
export OCPI_HAVE_PTWORKQUEUE=1


source env/finish.sh
done; trap - ERR

