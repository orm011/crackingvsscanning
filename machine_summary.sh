CPU=$(cat /proc/cpuinfo | grep name | head -n 1)
CPUCNT=$(cat /proc/cpuinfo | grep processor | wc -l)
MEM=$(free -g | head -n 2 | tail -n 1 | awk '{print $2}')
GCC=$(gcc --version | grep gcc)
KER=$(uname -r)
LIBCVER=$(/lib64/libc.so.6 | grep 'release version')
LIBCCMP=$(/lib64/libc.so.6 | grep 'Compiled by')
LIBCKER=$(/lib64/libc.so.6 | grep 'Compiled on')
echo cpu $CPU 
echo cpus: $CPUCNT
echo ram GB: $MEM 
echo kernel: $KER
echo gcc version: $GCC 
echo libc version: $LIBCVER
echo libc built: $LIBCCMP
echo libc kernel: $LIBCKER
