CPU=$(cat /proc/cpuinfo | grep name | head -n 1)
CPUCNT=$(cat /proc/cpuinfo | grep processor | wc -l)
MEM=$(free -g | head -n 2 | tail -n 1 | awk '{print $2}')
GCC=$(gcc --version | grep gcc)
KER=$(uname -r)

LIBC=$(ldd /usr/bin/ld | grep libc | sed 's/.* => \(\/[^ ]*\) .*/\1/')
LIBCVER=$($LIBC | grep 'release version')
PAGESIZE=$(getconf PAGESIZE)

echo cpu $CPU 
echo threads: $CPUCNT
echo ram GB: $MEM 
echo kernel: $KER
echo gcc version: $GCC 
echo libc version: $LIBCVER
echo page size: $PAGESIZE
