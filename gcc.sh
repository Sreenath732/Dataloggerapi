cd https://github.com/Sreenath732/Testing/blob/main
sudo apt-get install libpqxx-4.0v5
sudo apt-get install libpqxx-dev
sudo apt-get install libjsoncpp-dev
sudo apt-get install libgcrypt-dev
sree=$(g++ dataloggerapi.cpp.cpp -lpqxx -lpq -std=c++14 -o dataloggerapi -static-libstdc++ -ljsoncpp -lcrypt)
echo $sree