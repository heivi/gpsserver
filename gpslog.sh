#!/bin/bash

TIMESTAMP=$(date +"%s")
cd /home/change/to/your/folder/gpsseuranta/
read -r FIRSTLINE < gps.log

if [ "$FIRSTLINE" != "" ]
then
	cp gps.log /home/change/to/your/folder/gpslog/${TIMESTAMP}.txt
	cat /dev/null > ./gps.log
fi
