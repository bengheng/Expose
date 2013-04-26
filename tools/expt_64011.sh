#!/bin/bash

rm log*

echo "create user libmatcher@localhost identified by 'l1bm@tcher'" | mysql -u root
echo "create database libmatcher" | mysql -u root
echo "grant all on libmatcher.* to libmatcher@localhost" | mysql -u root

mysql -u 'libmatcher' -p'l1bm@tcher' 'libmatcher' < schema.sql
mysql -u 'libmatcher' -p'l1bm@tcher' 'libmatcher' < seed.sql
mysql -u 'libmatcher' -p'l1bm@tcher' 'libmatcher' < CleanSQL

python ./AddSubject.py ~/libmatcher/mnt/VULN/libz_64.so.1.2.2 11 3 200 64
python ./GoLibMatcher.py approx 64011 replace complete 3 64
mysqldump -u 'libmatcher' -p'l1bm@tcher' 'libmatcher' > expt_64011.sql
