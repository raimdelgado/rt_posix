# RT-POSIX

RT-POSIX is a library to easily create real-time tasks using the POSIX-API, inspired by popular RTOSes such as FreeRTOS and Xenomai.

## Installation

RT-POSIX includes unit testing using Google Test (GTest). Thus, the goolgetest package should be installed first.

```bash
# install cmake and googletest packages
sudo apt-get update
sudo apt-get install cmake googletest

# organize directories to avoid conflicts (Ubuntu 18.04)
sudo -s   # this requires password
cd /usr/src
mkdir -p GTest
mv googletest googlemock CMakeLists.txt Gtest/
cd GTest
mkdir -p build
cd build
cmake -DBUILD_SHARED_LIBS=ON ..
make

cp -rfp googletest/include/gtest /usr/include
cp -rfp googlemock/gtest/libgtest_main.so googlemock/gtest/libgtest.so /usr/lib
ldconfig

# check if GNU detects the libraries
ldconfig -v | grep gtest

# the output should be like this below...
    libgtest_main.so -> libgtest_main.so
	libgtest.so -> libgtest.so

```

## Contributing
Pull requests are always welcome. For major issues, please open an issue first and we will discuss that.


## Quality Control (DEVOPS)
Monitoring of Static Analysis, Unit Tests, and Code Coverage results will be added later using Jenkins.

## License
[GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
