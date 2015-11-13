if [ "$1" = "build" ]; then
	cd iotivity-1.0.0 && scons TARGET_OS=android TARGET_ARCH=x86 ANDROID_HOME=/home/gerald/Android/Sdk ANDROID_NDK=/home/gerald/Android/android-ndk-r10e GRADLE_HOME=/home/gerald/android-studio/gradle/gradle-2.4/bin
elif [ "$1" = "program" ]; then
	adb devices
	adb install iotivity-1.0.0/android/examples/simpleclient/build/outputs/apk/simpleclient-debug.apk
else
	echo "Unknown option: $1"
fi
