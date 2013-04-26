#!/bin/bash

rm log*
mysql -u 'libmatcher' -p'l1bm@tcher' 'libmatcher' < CleanSQL
./AddSubject.py ~/libmatcher/mnt/VULN/libsndfile_64.so.1.0.20 12 3 200 64
./GoLibMatcher.py approx 64712 replace complete 3 64
