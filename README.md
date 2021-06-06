# chia-plotter (pipelined multi-threaded)

This is a new implementation of a chia plotter which is desinged as a processing pipeline,
similar to how GPUs work, only the "cores" are normal software CPU threads.

As a result this plotter is able to fully max out any storage device's bandwidth,
simply by increasing the number of "cores", ie. threads.

## Usage

```
chia_plot <pool_key> <farmer_key> [tmp_dir] [tmp_dir2] [num_threads] [log_num_buckets]

For <pool_key> and <farmer_key> see output of `chia keys show`.
<tmp_dir> needs about 200G space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')
<tmp_dir2> needs about 110G space and ideally is a RAM drive, it will handle about 75% of all writes.
If <tmp_dir> is not specified it defaults to current directory.
If <tmp_dir2> is not specified it defaults to <tmp_dir>.
```

Make sure to crank up `<num_threads>` if you have plenty of cores, the default is 4.
Depending on the phase more threads will be launched, the setting is just a multiplier.

RAM usage depends on `<num_threads>` and `<log_num_buckets>`.
With default `<log_num_buckets>` and 4 threads it's ~2GB, with 16 threads it's ~6GB.

## How to Support

XCH: xch1w5c2vv5ak08pczeph7tp5xmkl5762pdf3pyjkg9z4ks4ed55j3psgay0zh

I developed this on my own time, even though I already filled all my HDDs (~50 TiB) with the official (slow) plotter.

## Results

On a dual Xeon(R) E5-2650v2@2.60GHz R720 with 256GB RAM and a 3x800GB SATA SSD RAID0, using a 110G tmpfs for `<tmp_dir2>`:

```
Number of threads: 16
Number of sort buckets: 2^7 (128)
[P1] Table 1 took 19.7565 sec
[P1] Table 2 took 158.023 sec, found 4295006334 matches
[P1] Lost 38078 matches due to 32-bit overflow.
[P1] Table 3 took 192.344 sec, found 4295069227 matches
[P1] Lost 101461 matches due to 32-bit overflow.
[P1] Table 4 took 232.667 sec, found 4295005288 matches
[P1] Lost 38169 matches due to 32-bit overflow.
[P1] Table 5 took 209.224 sec, found 4294989698 matches
[P1] Lost 21846 matches due to 32-bit overflow.
[P1] Table 6 took 210.724 sec, found 4295038169 matches
[P1] Lost 70663 matches due to 32-bit overflow.
[P1] Table 7 took 167.994 sec, found 4295033899 matches
Phase 1 took 1190.78 sec
[P2] max_table_size = 4295069227
[P2] Table 7 scan took 16.5743 sec
[P2] Table 7 rewrite took 42.6904 sec, dropped 0 entries (0 %)
[P2] Table 6 scan took 47.0322 sec
[P2] Table 6 rewrite took 79.0409 sec, dropped 581347959 entries (13.5353 %)
[P2] Table 5 scan took 45.14 sec
[P2] Table 5 rewrite took 78.6137 sec, dropped 761971771 entries (17.7409 %)
[P2] Table 4 scan took 45.4273 sec
[P2] Table 4 rewrite took 77.9435 sec, dropped 828850768 entries (19.298 %)
[P2] Table 3 scan took 47.002 sec
[P2] Table 3 rewrite took 74.3886 sec, dropped 855131659 entries (19.9096 %)
[P2] Table 2 scan took 48.3898 sec
[P2] Table 2 rewrite took 72.6112 sec, dropped 865617481 entries (20.154 %)
Phase 2 took 701.277 sec
Wrote plot header with 140 bytes
[P3-1] Table 2 took 73.1778 sec, wrote 3429388853 right entries
[P3-2] Table 2 took 71.2839 sec, wrote 3429388853 left entries, 3429388853 final
[P3-1] Table 3 took 77.0444 sec, wrote 3439937568 right entries
[P3-2] Table 3 took 71.2768 sec, wrote 3439937568 left entries, 3439937568 final
[P3-1] Table 4 took 127.274 sec, wrote 3466154520 right entries
[P3-2] Table 4 took 70.514 sec, wrote 3466154520 left entries, 3466154520 final
[P3-1] Table 5 took 123.923 sec, wrote 3533017927 right entries
[P3-2] Table 5 took 71.7726 sec, wrote 3533017927 left entries, 3533017927 final
[P3-1] Table 6 took 130.999 sec, wrote 3713690210 right entries
[P3-2] Table 6 took 73.8551 sec, wrote 3713690210 left entries, 3713690210 final
[P3-1] Table 7 took 66.7254 sec, wrote 4295033899 right entries
[P3-2] Lost 66603 entries due to 32-bit overflow.
[P3-2] Table 7 took 89.372 sec, wrote 4294967296 left entries, 4294967296 final
Phase 3 took 1054.73 sec, wrote 21877156374 entries to final plot
[P4] Starting to write C1 and C3 tables
[P4] Finished writing C1 and C3 tables
[P4] Writing C2 table
[P4] Finished writing C2 table
Phase 4 took 193.905 sec, final plot size is 108835549588 bytes
Total plot creation time was 3140.75 sec
```

## Future Plans

I do have some history with GPU mining, back in 2014 I was the first to open source a XPM GPU miner,
which was about 40x more efficient than the CPU miner. See my other repos.

As such, it's only a matter of time until I'll add OpenCL support to speed up the plotter even more,
keeping most of the load off the CPUs.

## Dependencies

- cmake (>=3.14)
- libsodium-dev

## Install

```
git submodule update --init
./make_devel.sh
```

The binaries will end up in `build/`, you can copy them elsewhere freely (on the same machine, or similar OS).

## Known Issues

- Doesn't compile with gcc-11, use a lower version.
- Needs at least cmake 3.14 (because of bls-signatures)

## How to install latest cmake on 18.04

https://askubuntu.com/questions/1203635/installing-latest-cmake-on-ubuntu-18-04-3-lts-run-via-wsl-openssl-error

