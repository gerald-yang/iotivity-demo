sudo stty -F /dev/ttyACM0 speed 1200 cs8 -cstopb -parenb
sudo /home/gerald/canonical/iot/arduino/arduino-1.5.7/hardware/tools/bossac -i -d --port=ttyACM0 -U false -e -w -v -b /home/gerald/canonical/iot/iotivity-1.0.0/out/arduino/arm/release/resource/csdk/stack/samples/arduino/SimpleClientServer/demoserver/demoserver.hex -R
