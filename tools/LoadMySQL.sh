#!/bin/bash

echo "Loading schema and seed to $1"

mysql -u root -p$2 -h virtual2.eecs.umich.edu $1 < schema.sql
mysql -u root -p$2 -h virtual2.eecs.umich.edu $1 < seed.sql

