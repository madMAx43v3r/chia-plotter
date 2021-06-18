### Installing on Clear Linux Server

```bash
sudo swupd update

sudo swupd bundle-add c-extras-gcc10
sudo swupd bundle-add devpkg-libsodium

sudo swupd bundle-add git
sudo swupd bundle-add wget
sudo swupd bundle-add zip

# Use gcc10 during build
export CC=gcc-10
export CXX=g++-10
git submodule update --init


# Install gmp
cd /tmp
wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.lz
tar -xvf gmp-6.2.1.tar.xz
cd gmp-6.2.1
./configure
make && make check
sudo make install

echo PATH=$PATH:/usr/local/bin/ # for statically compiled cmake if not already in your PATH
sudo ln -sf /usr/local/lib/libgmp.so.10.4.1 /usr/lib64/libgmp.so

# Install libsodium
cd /tmp
wget https://download.libsodium.org/libsodium/releases/LATEST.tar.gz
tar -xvf LATEST.tar.gz
cd libsodium-stable/
./configure
make && make check
sudo make install

# Checkout the source and install
cd ~/
git clone https://github.com/madMAx43v3r/chia-plotter.git 
cd ~/chia-plotter/

./make_devel.sh
./build/chia_plot  --help
```
