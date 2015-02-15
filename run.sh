for size in 1024 2048 4096 8192; do 
    for f in $(ls bin); do
       echo $f $size
       ./bin/$f $size >> results.txt
    done
done
