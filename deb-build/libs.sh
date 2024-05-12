#!/bin/bash

ldd $(find build/ -executable -type f) | awk '/=>/{print $(NF-1)}' | rev | cut -f1 -d/ | rev | sort | uniq | egrep -v "^not$" >libs-current.txt

cmp -s libs-current.txt ../libs.txt
if [ $? -ne 0 ]; then
	echo "Libs changed ... recalculating" >&2
	mv -f libs-current.txt ../libs.txt
	cat ../libs.txt | while read n; do dpkg-query -S $n | grep "$n$"; done | sed 's/^\([^:]\+\):.*$/\1/' | sort | uniq | tr "\n" "," | sed "s/,$//" >../deps.txt
fi

cat ../deps.txt
echo ,libqt6sql6-psql