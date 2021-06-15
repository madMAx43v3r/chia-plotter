# K33 chia-plotter (pipelined multi-threaded)

This is a new implementation of a chia plotter which is designed as a processing pipeline,
similar to how GPUs work, only the "cores" are normal software CPU threads.

As a result this plotter is able to fully max out any storage device's bandwidth,
simply by increasing the number of "cores", ie. threads.

## Usage

Check discord for support: https://discord.gg/rj46Dc5c

```
For <poolkey> and <farmerkey> see output of `chia keys show`.
<tmpdir> needs about 220 GiB space, it will handle about 25% of all writes. (Examples: './', '/mnt/tmp/')
<tmpdir2> needs about 110 GiB space and ideally is a RAM drive, it will handle about 75% of all writes.
Combined (tmpdir + tmpdir2) peak disk usage is less than 256 GiB.

Usage:
  chia_plot [OPTION...]

  -n, --count arg      Number of plots to create (default = 1, -1 = infinite)
  -r, --threads arg    Number of threads (default = 4)
  -u, --buckets arg    Number of buckets (default = 256)
  -t, --tmpdir arg     Temporary directory, needs ~220 GiB (default = $PWD)
  -2, --tmpdir2 arg    Temporary directory 2, needs ~110 GiB [RAM] (default = <tmpdir>)
  -d, --finaldir arg   Final directory (default = <tmpdir>)
  -p, --poolkey arg    Pool Public Key (48 bytes)
  -f, --farmerkey arg  Farmer Public Key (48 bytes)
      --help           Print help
```

Make sure to crank up `<threads>` if you have plenty of cores, the default is 4.
Depending on the phase more threads will be launched, the setting is just a multiplier.

RAM usage depends on `<threads>` and `<buckets>`.
With the new default of 256 buckets it's about 0.5 GB per thread at most.

### RAM disk setup on Linux
`sudo mount -t tmpfs -o size=110G tmpfs /mnt/ram/`

## How to Support

XCH: xch1w5c2vv5ak08pczeph7tp5xmkl5762pdf3pyjkg9z4ks4ed55j3psgay0zh

I developed this on my own time, even though I already filled all my HDDs (~50 TiB) with the official (slow) plotter.

## Results

On a dual Xeon<sup>(R)</sup> E5-2650v2<span>@</span>2.60GHz R720 with 256GB RAM and a 3x800GB SATA SSD RAID0, using a 110G tmpfs for `<tmpdir2>`:

```
Multi-threaded pipelined Chia k33 plotter - af085c7
Final Directory: /mnt/h/
Number of Plots: 6
Crafting plot 1 out of 6
Process ID: 3721
Number of Threads: 6
Number of Buckets: 2^8 (256)
Pool Public Key:   93dadb561d206dced3c1a6469533b467c93c78f01ab63136bc9b61d6ad168977793fe683e4c7b00e196b7675e2b87903
Farmer Public Key: b98e43bcbe25e855aa76ea10dda16ceb13ee7a5f8fd97a331984881eb0c15b3a70468bde5ea431dd283d58ddab19a533
Working Directory:   /mnt/d/1/
Working Directory 2: /mnt/c/1/
Plot Name: plot-k33-2021-06-16-02-13-94b0a83d6a76c7e00f1627c3105d6087a884b92e6b8da76d4ad3c1ea558a6259
[P1] Table 1 took 30 sec
[P1] Table 2 took 188.531 sec, found 4294869816 matches
[P1] Table 3 took 213.945 sec, found 4294795226 matches
[P1] Table 4 took 254.325 sec, found 4294609411 matches
[P1] Table 5 took 247.082 sec, found 4294297032 matches
[P1] Table 6 took 230.853 sec, found 4293595719 matches
[P1] Table 7 took 181.128 sec, found 4292300478 matches
Phase 1 took 1346.4 sec
[P2] max_table_size = 4294967296
[P2] Table 7 scan took 16.7054 sec
[P2] Table 7 rewrite took 44.7353 sec, dropped 0 entries (0 %)
[P2] Table 6 scan took 57.4712 sec
[P2] Table 6 rewrite took 118.555 sec, dropped 581442915 entries (13.5421 %)
[P2] Table 5 scan took 60.034 sec

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

