if [ -z "$1" ]; then
	echo "Need to specify TARGET_ARCH"
	exit 1
else
	cd iotivity-1.0.0 && scons TARGET_OS=android TARGET_ARCH=$1 ANDROID_HOME=/home/gerald/Android/Sdk ANDROID_NDK=/home/gerald/Android/android-ndk-r10e ANDROID_GRADLE=/home/gerald/android-studio/gradle/gradle-2.8/bin/gradle
fi
