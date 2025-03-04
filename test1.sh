#!/bin/bash

TMC="./tmc"
fail=0
pass=0

echo "TEST1"

for file in `ls examples/*.tm` ; do
    $TMC $file "${file%.*}.sr" >"${file%.*}.output"
    if [ "$?" == "0" ] && cmp -s "${file%.*}.output" "${file%.*}.expect" ; then
        echo "$file [PASS]"
        let pass=pass+1
    else
        echo "$file [FAIL]"
        let fail=fail+1
    fi
done

echo "Pass: $pass, Fail: $fail"
