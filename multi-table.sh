#! / bin/sh

n=$1
m=$2
for i in $(seq 1 $n)
do
	for j in $(seq 1 $m)
	do
		mul=`expr $i \* $j`
		echo -n $i*$j=$mul" "
	done
	echo -n "\n"
done
exit 0

