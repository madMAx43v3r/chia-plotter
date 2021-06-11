# chia-plotter (pipelined multi-threaded)

This is a new implementation of a chia plotter which is designed as a processing pipeline,
similar to how GPUs work, only the "cores" are normal software CPU threads.

As a result this plotter is able to fully max out any storage device's bandwidth,
simply by increasing the number of "cores", ie. threads.

## Usage

```
For <poolkey> and <farmerkey> see output of `chia keys show`.
<tmpdir> needs about 220 GiB space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')
<tmpdir2> needs about 110 GiB space and ideally is a RAM drive, it will handle about 75% of all writes.
Combined (tmpdir + tmpdir2) peak disk usage is less than 256 GiB.

Usage:
  chia_plot [OPTION...]

  -n, --count arg      Number of plots to create (default = 1, -1 = infinite)
  -r, --threads arg    Number of threads (default = 4)
  -u, --buckets arg    Number of buckets (default = 128)
  -t, --tmpdir arg     Temporary directory, needs ~220 GiB (default = $PWD)
  -2, --tmpdir2 arg    Temporary directory 2, needs ~110 GiB [RAM] (default = <tmpdir>)
  -d, --finaldir arg   Final directory (default = <tmpdir>)
  -p, --poolkey arg    Pool Public Key (48 bytes)
  -f, --farmerkey arg  Farmer Public Key (48 bytes)
      --help           Print help
```

Make sure to crank up `<threads>` if you have plenty of cores, the default is 4.
Depending on the phase more threads will be launched, the setting is just a multiplier.

RAM usage depends on `<threads>` and `<buckets>`. With default `<buckets>` it's about 1 GB per thread at most.

## How to Support

XCH: xch1w5c2vv5ak08pczeph7tp5xmkl5762pdf3pyjkg9z4ks4ed55j3psgay0zh

I developed this on my own time, even though I already filled all my HDDs (~50 TiB) with the official (slow) plotter.

## Results

On a dual Xeon<sup>(R)</sup> E5-2650v2<span>@</span>2.60GHz R720 with 256GB RAM and a 3x800GB SATA SSD RAID0, using a 110G tmpfs for `<tmpdir2>`:

```
Number of Threads: 16
Number of Sort Buckets: 2^7 (128)
Working Directory:   /mnt/tmp3/chia/tmp/
Working Directory 2: /mnt/tmp3/chia/tmp/ram/
[P1] Table 1 took 18.8022 sec
[P1] Table 2 took 147.205 sec, found 4295074754 matches
[P1] Table 3 took 176.34 sec, found 4295037895 matches
[P1] Table 4 took 209.898 sec, found 4294984890 matches
[P1] Table 5 took 214.792 sec, found 4295100396 matches
[P1] Table 6 took 209.668 sec, found 4294984295 matches
[P1] Table 7 took 169.413 sec, found 4294972949 matches
Phase 1 took 1146.15 sec
[P2] max_table_size = 4295100396
[P2] Table 7 scan took 15.7779 sec
[P2] Table 7 rewrite took 35.3033 sec, dropped 0 entries (0 %)
[P2] Table 6 scan took 45.5338 sec
[P2] Table 6 rewrite took 63.9984 sec, dropped 581281517 entries (13.534 %)
[P2] Table 5 scan took 44.5098 sec
[P2] Table 5 rewrite took 61.4229 sec, dropped 762079936 entries (17.743 %)
[P2] Table 4 scan took 43.6594 sec
[P2] Table 4 rewrite took 60.2593 sec, dropped 828850570 entries (19.2981 %)
[P2] Table 3 scan took 56.0747 sec
[P2] Table 3 rewrite took 68.8049 sec, dropped 855127091 entries (19.9097 %)
[P2] Table 2 scan took 55.4185 sec
[P2] Table 2 rewrite took 71.0217 sec, dropped 865651624 entries (20.1545 %)
Phase 2 took 649.758 sec
Wrote plot header with 268 bytes
[P3-1] Table 2 took 81.9547 sec, wrote 3429423130 right entries
[P3-2] Table 2 took 40.1714 sec, wrote 3429423130 left entries, 3429423130 final
[P3-1] Table 3 took 70.6664 sec, wrote 3439910804 right entries
[P3-2] Table 3 took 42.7598 sec, wrote 3439910804 left entries, 3439910804 final
[P3-1] Table 4 took 72.7 sec, wrote 3466134320 right entries
[P3-2] Table 4 took 43.5351 sec, wrote 3466134320 left entries, 3466134320 final
[P3-1] Table 5 took 78.1118 sec, wrote 3533020460 right entries
[P3-2] Table 5 took 42.9138 sec, wrote 3533020460 left entries, 3533020460 final
[P3-1] Table 6 took 84.2833 sec, wrote 3713702778 right entries
[P3-2] Table 6 took 45.0141 sec, wrote 3713702778 left entries, 3713702778 final
[P3-1] Table 7 took 83.0243 sec, wrote 4294972949 right entries
[P3-2] Table 7 took 50.5329 sec, wrote 4294967296 left entries, 4294967296 final
Phase 3 took 743.208 sec, wrote 21877158788 entries to final plot
[P4] Starting to write C1 and C3 tables  
[P4] Finished writing C1 and C3 tables   
[P4] Writing C2 table
[P4] Finished writing C2 table
Phase 4 took 81.4248 sec, final plot size is 108835558845 bytes
Total plot creation time was 2620.62 sec 
```

## How to Verify

To make sure the plots are valid you can use the `ProofOfSpace` tool from [chiapos](https://github.com/Chia-Network/chiapos):

```bash
git clone https://github.com/Chia-Network/chiapos.git
cd chiapos && mkdir build && cd build && cmake .. && make -j8
./ProofOfSpace check -f plot-k32-???.plot [num_iterations]
```

## Future Plans

I do have some history with GPU mining, back in 2014 I was the first to open source a XPM GPU miner,
which was about 40x more efficient than the CPU miner. See my other repos.

As such, it's only a matter of time until I'll add OpenCL support to speed up the plotter even more,
keeping most of the load off the CPUs.

## Dependencies

- cmake (>=3.14)
- libgmp3-dev
- libsodium-dev

## Install
---
### Windows
Binaries built by [stotiks](https://github.com/stotiks) can be found here:
https://github.com/stotiks/chia-plotter/releases

---
### Arch Linux
```bash
sudo pamac install cmake gmp libgmp-static libsodium libsodium-static gcc10
# Checkout the source and install
git clone https://github.com/madMAx43v3r/chia-plotter.git 
cd chia-plotter

# Use gcc10 during build
export CC=gcc-10
export CXX=g++-10
git submodule update --init
./make_devel.sh
./build/chia_plot --help
```
---
### CentOS 7
```bash
git clone https://github.com/dendil/chia-plotter.git
cd chia-plotter

git submodule update --init
sudo yum install cmake3 gmp-devel libsodium gmp-static libsodium-static -y
# Install a package with repository for your system:
# On CentOS, install package centos-release-scl available in CentOS repository:
sudo yum install centos-release-scl -y
# Install the collection:
sudo yum install devtoolset-7 -y
# Start using software collections:
scl enable devtoolset-7 bash
./make_devel.sh
./build/chia_plot --help
```
---
### Ubuntu 20.04
```bash
sudo apt install -y libsodium-dev libgmp3-dev cmake g++ git
# Checkout the source and install
git clone https://github.com/madMAx43v3r/chia-plotter.git 
cd chia-plotter

git submodule update --init
./make_devel.sh
./build/chia_plot --help
```

The binaries will end up in `build/`, you can copy them elsewhere freely (on the same machine, or similar OS).

---
### macOS Catalina
First you need to install a package manager called [Brew](https://brew.sh/) and [Xcode](https://apps.apple.com/app/xcode/id497799835) from the Apple App Store.
```bash
brew install libsodium gmp cmake git autoconf automake libtool
sudo ln -s /usr/local/include/gmp.h /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
sudo ln -s /usr/local/include/sodium.h /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
sudo ln -s /usr/local/include/sodium /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
git clone https://github.com/madMAx43v3r/chia-plotter.git 
cd chia-plotter
git submodule update --init
./make_devel.sh
./build/chia_plot --help
```

## Running in a Docker container

In some setups and scenarios, it could be useful to run your plotter inside a Docker container. This could be potentially useful while running `chia-plotter` in Windows.

To do so, install Docker in your computer and them run the following command:

```sh
docker run \
  -v <path-to-your-tmp-dir>:/mnt/harvester \
  -v <path-to-your-final-dir>:/mnt/farm \
  odelucca/chia-plotter \
    -t /mnt/harvester/ \
    -d /mnt/farm/ \
    -p <pool-key> \
    -f <farm-key> \
    -r <number-of-CPU-cores>
```
> 💡 You can provide any of the plotter arguments after the image name (`odelucca/chia-plotter`)

In a Linux benchmark, we were able to find that running in Docker has only 5% performance impact than running in native OS.

For Windows users, you should check if your Docker configuration has any RAM or CPU limits. Since Docke runs inside HyperV, that could potentially constrain your hardware usage. In any case, you can set the RAM limits with the `-m` flag (after the `docker run` command).

### Regarding multithread in Docker

While running in Windows, you may need to proper configure your Docker to allow multi CPUs. You can do so by following [this article](https://www.thorsten-hans.com/docker-container-cpu-limits-explained/)

In a nutshell, you could also pass the `--cpus` flag to your `docker run` command in order to achieve the same result.

So, for example, the following command:
```sh
docker run \
  -v <path-to-your-tmp-dir>:/mnt/harvester \
  -v <path-to-your-final-dir>:/mnt/farm \
  -m 8000 \
  --cpus 8 \
  odelucca/chia-plotter \
    -t /mnt/harvester/ \
    -d /mnt/farm/ \
    -p <pool-key> \
    -f <farm-key> \
    -r 8
```

Would run your plotter with 8 CPUs and 8GB of RAM.

---

## Known Issues

- Doesn't compile with gcc-11, use a lower version.
- Needs at least cmake 3.14 (because of bls-signatures)

## How to install latest cmake on 18.04

https://askubuntu.com/questions/1203635/installing-latest-cmake-on-ubuntu-18-04-3-lts-run-via-wsl-openssl-error

