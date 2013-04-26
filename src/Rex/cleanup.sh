#!/bin/bash

rm $2
cat $1 | sed s/"ASSERT( "// | sed s/" );"// | sed s/"f0"// | sed s/"f1"// | sort | tee  $2
