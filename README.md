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
Number of Threads: 16
Number of Sort Buckets: 2^7 (128)
Working Directory:   ./
Working Directory 2: ./ram/
[P1] Table 1 took 21.0467 sec
[P1] Table 2 took 152.6 sec, found 4295044959 matches
[P1] Lost 77279 matches due to 32-bit overflow.
[P1] Table 3 took 181.169 sec, found 4295030463 matches
[P1] Lost 62514 matches due to 32-bit overflow.
[P1] Table 4 took 223.303 sec, found 4295044715 matches
[P1] Lost 76928 matches due to 32-bit overflow.
[P1] Table 5 took 232.129 sec, found 4294967739 matches
[P1] Lost 235 matches due to 32-bit overflow.
[P1] Table 6 took 221.468 sec, found 4294932892 matches
[P1] Table 7 took 182.597 sec, found 4294838936 matches
Phase 1 took 1214.37 sec
[P2] max_table_size = 4295044959
[P2] Table 7 scan took 16.9198 sec
[P2] Table 7 rewrite took 44.796 sec, dropped 0 entries (0 %)
[P2] Table 6 scan took 47.5287 sec
[P2] Table 6 rewrite took 81.2195 sec, dropped 581301544 entries (13.5346 %)
[P2] Table 5 scan took 46.6094 sec
[P2] Table 5 rewrite took 77.9914 sec, dropped 761979000 entries (17.7412 %)
[P2] Table 4 scan took 52.427 sec
[P2] Table 4 rewrite took 75.7487 sec, dropped 828872625 entries (19.2983 %)
[P2] Table 3 scan took 54.0839 sec
[P2] Table 3 rewrite took 74.9016 sec, dropped 855088153 entries (19.9088 %)
[P2] Table 2 scan took 49.692 sec
[P2] Table 2 rewrite took 73.0273 sec, dropped 865610902 entries (20.1537 %)
Phase 2 took 721.638 sec
Wrote plot header with 268 bytes
[P3-1] Table 2 took 76.0894 sec, wrote 3429434057 right entries
[P3-2] Table 2 took 75.1076 sec, wrote 3429434057 left entries, 3429434057 final
[P3-1] Table 3 took 78.0162 sec, wrote 3439942310 right entries
[P3-2] Table 3 took 73.0284 sec, wrote 3439942310 left entries, 3439942310 final
[P3-1] Table 4 took 133.769 sec, wrote 3466172090 right entries
[P3-2] Table 4 took 76.1504 sec, wrote 3466172090 left entries, 3466172090 final
[P3-1] Table 5 took 127.125 sec, wrote 3532988739 right entries
[P3-2] Table 5 took 77.7182 sec, wrote 3532988739 left entries, 3532988739 final
[P3-1] Table 6 took 134.779 sec, wrote 3713631348 right entries
[P3-2] Table 6 took 81.9068 sec, wrote 3713631348 left entries, 3713631348 final
[P3-1] Table 7 took 69.066 sec, wrote 4294838936 right entries
[P3-2] Table 7 took 94.0157 sec, wrote 4294838936 left entries, 4294838936 final
Phase 3 took 1104.11 sec, wrote 21877007480 entries to final plot
[P4] Starting to write C1 and C3 tables
[P4] Finished writing C1 and C3 tables
[P4] Writing C2 table
[P4] Finished writing C2 table
Phase 4 took 89.0748 sec, final plot size is 108834390977 bytes
Total plot creation time was 3129.28 sec
```

## How to Verify

To make sure the plots are valid you can use the `ProofOfSpace` tool from `chiapos`:

```
ProofOfSpace check -f plot-k32-???.plot [num_iterations]
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

