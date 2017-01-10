#!/bin/bash

echo "test common!"
nasm=""
nasm_list=(nasm /usr/bin/nasm /opt/local/bin/nasm)
for cmd in ${nasm_list[@]}
do
    ver=`$cmd -v 2>/dev/null | awk '{print $3}'`
	[[ $ver =~ ^2\.1[0-9] ]] && nasm=$cmd && break
done

echo "ver is $ver"
echo "nasm is $nasm"
[ "$nasm" = "" ] && echo "[Error] pls install nasm (2.10+)" 1>&2 && exit 1

