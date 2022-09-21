sudo apt-get update
sudo apt-get install cppcheck
cppcheck --enable=all dataloggerapi.cpp
sudo apt --fix-broken install
cppcheck --enable=all dataloggerapi.cpp
