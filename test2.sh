#!/bin/bash

TMC="./tmm treemeta.tmm"
fail=0
pass=0

echo "TEST2"

if [ ! -f treemeta.tmm ] ; then
    ./tmc treemeta.tm treemeta.tm >treemeta.tmm
fi

for file in `ls examples/*.tm` ; do
    $TMC $file >comp.tmm
    ./tmm comp.tmm "${file%.*}.sr" >"${file%.*}.output"
    if [ "$?" == "0" ] && cmp -s "${file%.*}.output" "${file%.*}.expect" ; then
        echo "$file [PASS]"
        let pass=pass+1
    else
        echo "$file [FAIL]"
        let fail=fail+1
    fi
done

rm -f comp.tmm

echo "Pass: $pass, Fail: $fail"
