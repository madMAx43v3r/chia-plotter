# chia-plotter (pipelined multi-threaded)

This is a new implementation of a chia plotter which is designed as a processing pipeline,
similar to how GPUs work, only the "cores" are normal software CPU threads.

As a result this plotter is able to fully max out any storage device's bandwidth,
simply by increasing the number of "cores", ie. threads.

Sponsored by [Flexpool.io](https://www.flexpool.io/) - Check them out if you're looking for a secure and scalable Chia pool.

## Usage

Join the Discord for support: https://discord.gg/maFmsyzQ

```
For <poolkey> and <farmerkey> see output of `chia keys show`.
To plot for pools, specify <contract> address via -c instead of <poolkey>, see `chia plotnft show`.
<tmpdir> needs about 220 GiB space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')
<tmpdir2> needs about 110 GiB space and ideally is a RAM drive, it will handle about 75% of all writes.
Combined (tmpdir + tmpdir2) peak disk usage is less than 256 GiB.
In case of <count> != 1, you may press Ctrl-C for graceful termination after current plot is finished,
or double press Ctrl-C to terminate immediately.

Usage:
  chia_plot [OPTION...]

  -n, --count arg      Number of plots to create (default = 1, -1 = infinite)
  -r, --threads arg    Number of threads (default = 4)
  -u, --buckets arg    Number of buckets (default = 256)
  -v, --buckets3 arg   Number of buckets for phase 3+4 (default = buckets)
  -t, --tmpdir arg     Temporary directory, needs ~220 GiB (default = $PWD)
  -2, --tmpdir2 arg    Temporary directory 2, needs ~110 GiB [RAM] (default = <tmpdir>)
  -d, --finaldir arg   Final directory (default = <tmpdir>)
  -w, --waitforcopy    Wait for copy to start next plot
  -p, --poolkey arg    Pool Public Key (48 bytes)
  -c, --contract arg   Pool Contract Address (62 chars)
  -f, --farmerkey arg  Farmer Public Key (48 bytes)
  -G, --tmptoggle      Alternate tmpdir/tmpdir2 (default = false)
      --help           Print help
```

Make sure to crank up `<threads>` if you have plenty of cores, the default is 4.
Depending on the phase more threads will be launched, the setting is just a multiplier.

RAM usage depends on `<threads>` and `<buckets>`.
With the new default of 256 buckets it's about 0.5 GB per thread at most.

`-G` option will alternate the temp dirs used while plotting to give each one, tmpdir and tmpdir2, equal usage. The first plot creation will use tmpdir and tmpdir2 as expected. The next run, if -n equals 2 or more, will swap the order to tmpdir2 and tmpdir. The next run swaps again to tmpdir and tmpdir2. This will occur until the number of plots created is reached or until stopped.

### RAM disk setup on Linux
`sudo mount -t tmpfs -o size=110G tmpfs /mnt/ram/`

Note: 128 GiB System RAM minimum required for RAM disk.

## How to Support

XCH: xch1w5c2vv5ak08pczeph7tp5xmkl5762pdf3pyjkg9z4ks4ed55j3psgay0zh

ETH-ERC20: 0x97057cdf529867838d2a1f7f23ba62456764e0cd

LTC: MNUnszsX2srv5EJpu9YYHAXb19MqUpuBjD

BTC: 15GSE5ymStxXMvJ58hyosEVm4FXFxUyJZg

## Results

On a dual Xeon® E5-2650v2<span>@</span>2.60GHz R720 with 256GB RAM and a 3x800GB SATA SSD RAID0, using a 110G tmpfs for `<tmpdir2>`:

<details>
  <summary>Click to expand</summary>
  
  ```
  Number of Threads: 16
  Number of Buckets: 2^8 (256)
  Working Directory:   /mnt/tmp3/chia/tmp/ 
  Working Directory 2: /mnt/tmp3/chia/tmp/ram/
  [P1] Table 1 took 17.2488 sec
  [P1] Table 2 took 145.011 sec, found 4294911201 matches
  [P1] Table 3 took 170.86 sec, found 4294940789 matches
  [P1] Table 4 took 203.713 sec, found 4294874801 matches
  [P1] Table 5 took 201.346 sec, found 4294830453 matches
  [P1] Table 6 took 195.928 sec, found 4294681297 matches
  [P1] Table 7 took 158.053 sec, found 4294486972 matches
  Phase 1 took 1092.2 sec
  [P2] max_table_size = 4294967296
  [P2] Table 7 scan took 15.5542 sec
  [P2] Table 7 rewrite took 37.7806 sec, dropped 0 entries (0 %)
  [P2] Table 6 scan took 46.7014 sec
  [P2] Table 6 rewrite took 65.7315 sec, dropped 581295425 entries (13.5352 %)
  [P2] Table 5 scan took 45.4663 sec
  [P2] Table 5 rewrite took 61.9683 sec, dropped 761999997 entries (17.7423 %)
  [P2] Table 4 scan took 44.8217 sec
  [P2] Table 4 rewrite took 61.36 sec, dropped 828847725 entries (19.2985 %)
  [P2] Table 3 scan took 44.9121 sec
  [P2] Table 3 rewrite took 61.5872 sec, dropped 855110820 entries (19.9097 %)
  [P2] Table 2 scan took 43.641 sec
  [P2] Table 2 rewrite took 59.6939 sec, dropped 865543167 entries (20.1528 %)
  Phase 2 took 620.488 sec
  Wrote plot header with 268 bytes
  [P3-1] Table 2 took 73.1018 sec, wrote 3429368034 right entries
  [P3-2] Table 2 took 42.3999 sec, wrote 3429368034 left entries, 3429368034 final
  [P3-1] Table 3 took 68.9318 sec, wrote 3439829969 right entries
  [P3-2] Table 3 took 43.8179 sec, wrote 3439829969 left entries, 3439829969 final
  [P3-1] Table 4 took 71.3236 sec, wrote 3466027076 right entries
  [P3-2] Table 4 took 46.2887 sec, wrote 3466027076 left entries, 3466027076 final
  [P3-1] Table 5 took 70.6369 sec, wrote 3532830456 right entries
  [P3-2] Table 5 took 45.5857 sec, wrote 3532830456 left entries, 3532830456 final
  [P3-1] Table 6 took 75.8534 sec, wrote 3713385872 right entries
  [P3-2] Table 6 took 48.8266 sec, wrote 3713385872 left entries, 3713385872 final
  [P3-1] Table 7 took 83.2586 sec, wrote 4294486972 right entries
  [P3-2] Table 7 took 56.3803 sec, wrote 4294486972 left entries, 4294486972 final
  Phase 3 took 733.323 sec, wrote 21875928379 entries to final plot
  [P4] Starting to write C1 and C3 tables  
  [P4] Finished writing C1 and C3 tables   
  [P4] Writing C2 table
  [P4] Finished writing C2 table
  Phase 4 took 84.6697 sec, final plot size is 108828428322 bytes
  Total plot creation time was 2530.76 sec 
  ```
</details>

## How to Verify

To make sure the plots are valid you can use the `ProofOfSpace` tool from [chiapos](https://github.com/Chia-Network/chiapos):

```bash
git clone https://github.com/Chia-Network/chiapos.git
cd chiapos && mkdir build && cd build && cmake .. && make -j8
./ProofOfSpace check -f plot-k32-???.plot [num_iterations]
```

## How to update to latest version

```bash
cd chia-plotter
git checkout master
git pull
git submodule update --init
```

## Future Plans

I do have some history with GPU mining, back in 2014 I was the first to open source [a XPM GPU miner,](https://github.com/madMAx43v3r/xpmclient)
which was about 40x more efficient than the CPU miner. [See my other repos.](https://github.com/madMAx43v3r?tab=repositories)

As such, it's only a matter of time until I add OpenCL support to speed up the plotter even more,
keeping most of the load off the CPUs.

## Dependencies

- cmake (>=3.14)
- libsodium-dev

## Install

<details>
  <summary>Windows</summary>
  
  Binaries built by [stotiks](https://github.com/stotiks) can be found here:
https://github.com/stotiks/chia-plotter/releases

</details>

<details>
  <summary>Arch Linux</summary>

  First, install dependencies from pacman:
  ```bash
  sudo pacman -S cmake libsodium gmp gcc11
  ```
  Then, clone and compile the project:
  ```bash
  # Checkout the source and install
  git clone https://github.com/madMAx43v3r/chia-plotter.git
  cd chia-plotter

  git submodule update --init
  ./make_devel.sh
  ./build/chia_plot --help
  ```
</details>

<details>
  <summary>CentOS 7</summary>
  
  ```bash
  git clone https://github.com/madMAx43v3r/chia-plotter.git
  cd chia-plotter

  git submodule update --init
  sudo yum install epel-release -y
  sudo yum install cmake3 libsodium libsodium-static -y
  ln /usr/bin/cmake3 /usr/bin/cmake
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
</details>

<details>
  <summary>Clear Linux</summary>
  
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
</details>

<details>
  <summary>Ubuntu 20.04</summary>
  
  ```bash
  sudo apt install -y libsodium-dev cmake g++ git build-essential
  # Checkout the source and install
  git clone https://github.com/madMAx43v3r/chia-plotter.git 
  cd chia-plotter

  git submodule update --init
  ./make_devel.sh
  ./build/chia_plot --help
  ```

  The binaries will end up in `build/`, you can copy them elsewhere freely (on the same machine, or similar OS).
</details>

<details>
  <summary>Debian 10 ("buster")</summary>

  Make sure to add buster-backports to your sources.list otherwise the installation will fail because an older cmake version. See the [debian backport documentation](https://backports.debian.org/Instructions/) for reference.

  ```bash
  # Install cmake 3.16 from buster-backports
  sudo apt install -t buster-backports cmake
  sudo apt install -y libsodium-dev g++ git
  # Checkout the source and install
  git clone https://github.com/madMAx43v3r/chia-plotter.git 
  cd chia-plotter

  git submodule update --init
  ./make_devel.sh
  ./build/chia_plot --help
  ```
  The binaries will end up in `build/`, you can copy them elsewhere freely (on the same machine, or similar OS).
</details>

<details>
  <summary>macOS</summary>
  
  First you need to install the [Brew](https://brew.sh/) package manager and [Xcode](https://apps.apple.com/app/xcode/id497799835) OR [Xcode Command Line Tools](https://developer.apple.com/download/).
  ```bash
  # Download Xcode Command Line Tools (skip if you already have Xcode)
  xcode-select --install

  # Now download chia-plotter's dependencies
  brew install libsodium cmake git autoconf automake libtool wget
  brew link cmake

  # If you downloaded Xcode run these:
  sudo ln -s /usr/local/include/sodium.h /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/
  sudo ln -s /usr/local/include/sodium /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/

  # If you downloaded CommandLineTools run these:
  sudo ln -s /usr/local/include/sodium.h /Library/Developer/CommandLineTools/usr/include
  sudo ln -s /usr/local/include/sodium /Library/Developer/CommandLineTools/usr/include

  ```

  Confirm which directory you have on YOUR Mac before applying following commands
  ```
  # For x86_64 Macs
  wget https://raw.githubusercontent.com/facebookincubator/fizz/master/build/fbcode_builder/CMake/FindSodium.cmake -O /usr/local/opt/cmake/share/cmake/Modules/FindSodium.cmake
  ```
   or
  ``` 
  # For ARM64 (M1) Macs
  wget https://raw.githubusercontent.com/facebookincubator/fizz/master/build/fbcode_builder/CMake/FindSodium.cmake -O /opt/homebrew/Cellar/cmake/*/share/cmake/Modules/FindSodium.cmake
  ```

  ```
  git clone https://github.com/madMAx43v3r/chia-plotter.git 
  cd chia-plotter
  git submodule update --init
  ./make_devel.sh
  ./build/chia_plot --help
  ```
  If a maximum open file limit error occurs (as default OS setting is 256, which is too low for default bucket size of `256`), run this before starting the plotter
  ```
  ulimit -n 3000
  ```
  This file limit change will only affect the current session.
</details>

<details>
  <summary>Running in a Docker container</summary>

  In some setups and scenarios, it could be useful to run your plotter inside a Docker container. This could be potentially useful while running `chia-plotter` in Windows.

  To do so, [install Docker](https://docs.docker.com/get-docker/) on your computer and them run the following command:

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

  In a Linux benchmark, we were able to find that running in Docker is only 5% slower than running in native OS.

  For Windows users, you should check if your Docker configuration has any RAM or CPU limits. Since Docker runs inside HyperV, that could potentially constrain your hardware usage. In any case, you can set the RAM limits with the `-m` flag (after the `docker run` command).

  ### Regarding multithread in Docker

  While running in Windows, you may need to proper configure your Docker to allow multi CPUs. You can do so by following [this article](https://www.thorsten-hans.com/docker-container-cpu-limits-explained/)

  In a nutshell, you could also pass the `--cpus` flag to your `docker run` command in order to achieve the same result.

  So, for example, the following command...
  ```sh
  docker run \
    -v <path-to-your-tmp-dir>:/mnt/harvester \
    -v <path-to-your-final-dir>:/mnt/farm \
    -m 8G \
    --cpus 8 \
    odelucca/chia-plotter \
      -t /mnt/harvester/ \
      -d /mnt/farm/ \
      -p <pool-key> \
      -f <farm-key> \
      -r 8
  ```

  ...would run your plotter with 8 CPU cores and 8GB of RAM.

  ### Building a Docker container
  Make sure your submodules are up-to-date by running `git submodule update --init`, then simply build with `docker build .`
</details>

---

## Known Issues

- Needs at least cmake 3.14 (because of bls-signatures)

[How to install latest cmake on Ubuntu 18.04](https://askubuntu.com/questions/1203635/installing-latest-cmake-on-ubuntu-18-04-3-lts-run-via-wsl-openssl-error)
