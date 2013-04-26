#!/bin/bash
~/idaadv/idal -B -Cg -S../bin/ida/idc/gendot.idc $1$2
sed s/vertex/label/g $1$2".1.dot" | sed 's/"\]\[start="/\\n/g' | sed 's/"\]\[end="/\\n/g' > $1$2".1.tmp"
dot $1$2".1.tmp" -Tpng -o $1$2".png"
