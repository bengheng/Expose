#!/bin/bash

echo "Dumping schema and seed from $1"
mysqldump -u libmatcher -h virtual2.eecs.umich.edu -pl1bm@tcher $1 --no-data > schema.sql
mysqldump -u libmatcher -h virtual2.eecs.umich.edu -pl1bm@tcher $1 vuln expt_approx client > seed.sql

