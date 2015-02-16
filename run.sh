size=200
out="results.$(hostname).$(date +"%m_%d_%H_%M_%S")"

for f in $(ls bin); do
    for pivot in 1 10 20 30 40 50 60 70 80 90 99; do
	set -x
	./bin/$f --sizemb $size --pivot $pivot >> $out
	set +x
    done
done


