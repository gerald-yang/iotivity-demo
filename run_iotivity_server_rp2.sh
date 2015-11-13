if [ -z "$DEMOROOT" ]; then
	echo "Please 'source env-setup' first"
	exit 1
fi

sudo PYTHONPATH=${DEMOROOT}/iotivity-1.0.0/extlibs/GrovePi/Software/Python/ ${DEMOROOT}/iotivity-1.0.0/out/linux/armeabi-v7a-hard/release/resource/examples/demoserver
