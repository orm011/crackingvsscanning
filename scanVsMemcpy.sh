make scanning
make memcpybench

i=0; while [[ $i < 5 ]]; do
OMP_NUM_THREADS=1 ./bin/scanning --sizemb 4096 --pivot 100
./bin/memcpybench -s 4096 -a memcpy
i=$((i + 1))
done