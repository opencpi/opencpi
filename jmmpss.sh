trap "trap - ERR; break" ERR; for i in 1; do
source env/start.sh

source platforms/mpss/mpss-env.sh

export OCPI_HAVE_MPSS=1

export MPSS_ROOT=/opt/mpss/3.5.2/sysroots/x86_64-mpsssdk-linux/usr/

export OCPI_HAVE_SCIF=1
export OCPI_SCIF_NODE=1
export OCPI_SCIF_PORT=4
export OCPI_SMB_SIZE=131072


source env/finish.sh
done; trap - ERR

