if [ -n "$1" ]; then
	cd iotivity-1.0.0 && scons -j $1
else
	cd iotivity-1.0.0 && scons
fi
