#!/bin/bash

tmp_file=$(mktemp -q)
diff_file=$(mktemp -q)

for fl in $(find -type f -name "*.[c,h]")
do
	indent -st -linux -l120 -i4 -nut $fl > $tmp_file
	diff $fl $tmp_file > $diff_file
	res=$(cat $diff_file | wc -l)
	if [ $res -ne 0 ]; then
		echo "The file $fl does not follow the coding style."
		echo "--> Please run ./scripts/fix.sh to fix all files automatically."
		exit 1
	fi
done

rm $tmp_file
rm $diff_file
