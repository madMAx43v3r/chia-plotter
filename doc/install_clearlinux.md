### Installing on Clear Linux Server

```bash
sudo swupd update
sudo swupd bundle-add c-basic devpkg-libsodium git wget

echo PATH=$PATH:/usr/local/bin/ # for statically compiled cmake if not already in your PATH

# Install libsodium
cd /tmp
wget https://download.libsodium.org/libsodium/releases/LATEST.tar.gz
tar -xvf LATEST.tar.gz
cd libsodium-stable
./configure
make && make check
sudo make install

# Checkout the source and install
cd ~/
git clone https://github.com/madMAx43v3r/chia-plotter.git 
cd ~/chia-plotter
git submodule update --init

./make_devel.sh
./build/chia_plot --help
```
