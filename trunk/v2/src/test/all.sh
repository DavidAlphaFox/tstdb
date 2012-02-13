#!/usr/bin/env sh
. ~/.bash_profile

echo $(which python)
for f in ./*.py
do
    python $f
done
