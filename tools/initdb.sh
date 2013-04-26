#!/bin/bash

# set -o errexit
# set -v

if [ "$1" == "" ]; then
	echo "Usage: $0 <sqlrootpwd>"
	exit 1
fi

WORKERS=("choplifter.gpcc.itd.umich.edu" "vanguard.gpcc.itd.umich.edu")

# Create users
for w in "${WORKERS[@]}"; do
	echo "Creating user 'libmatcher'@'$w'"
	mysql -u root -p"$1" -e "CREATE USER 'libmatcher'@'$w' IDENTIFIED BY 'l1bm@tcher'"
done

# Create databases for 32-bits experiments
for c in 0 1 2; do
	for v in {1..7}; do
		echo "Creating database expt_32"$c"0"$v
		mysql -u root -p"$1" -e "CREATE DATABASE expt_32"$c"0"$v
		mysql -u root -p"$1" "expt_32"$c"0"$v < schema.sql
		mysql -u root -p"$1" "expt_32"$c"0"$v < seed.sql
		
		# Grant necessary privileges for all clients
		for w in "${WORKERS[@]}"; do
			mysql -u root -p"$1" -e "GRANT ALL PRIVILEGES ON expt_32"$c"0"$v".* TO libmatcher@$w"
		done
	done
done

# Create databases for 64-bits experiments
for c in 0 1 2; do
	for v in {11..17}; do
		echo "Creating database expt_64$c$v"
		mysql -u root -p"$1" -e "CREATE DATABASE expt_64$c$v"
		mysql -u root -p"$1" "expt_64$c$v" < schema.sql
		mysql -u root -p"$1" "expt_64$c$v" < seed.sql

		# Grant necessary privileges for all clients
		for w in "${WORKERS[@]}"; do
			mysql -u root -p"$1" -e "GRANT ALL PRIVILEGES ON expt_64$c$v.* TO libmatcher@$w"
		done
	done
done

