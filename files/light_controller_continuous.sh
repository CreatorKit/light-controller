#!/bin/sh /etc/rc.common

while true
do
	line=$(ps | grep 'light_controller_appd' | grep -v grep)
	if [ -z "$line" ]
	then
		/etc/init.d/light_controller_appd start
	else
		sleep 5
	fi
done

