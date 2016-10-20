for v in $(env | egrep ^OCPI | sort | cut -f1 -d=); do
  # echo Clearing $v
  unset $v
done
