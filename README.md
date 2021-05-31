# chia-plotter (pipelined multi-threaded)

This is a new implementation of a chia plotter which is desinged as a processing pipeline,
similar to how GPUs work, only in this case the "cores" are normal software CPU threads.

As a result this plotter is able to fully max out any storage device's bandwidth,
simply by increasing the number of "cores", ie. threads.

## Phase 1

Phase 1 is able to finish in less than 60 min on my i7-9750H (45W 6-core) with a regular SATA SSD.

I expect a full plot to finish in less than 3 hours on this particular machine,
which is about 3x faster than with the official plotter.

Phase 1 requires the most CPU load, first and foremost the BLAKE3 hashing of the y values,
after that comes the matching algorithm and only then sorting. RAM usage is about ~2 GB, using `jemalloc`.

By default in phase 1, two read threads are used, 4 sort threads, 4 matching threads,
4 f(x)-evaluation threads and some in-between threads.

You can test yourself by running `test_phase_1` in a directory that's on your SSD.
Parameters are [number of threads, default = 4] [log number of sort buckets = 8], k is hardcoded to 32.

## Phase 2

Phase 2 is able to finish in about 25 min on my i7-9750H (45W 6-core) with a regular SATA SSD.

Using 4 threads by default to mark used entries in the bitfield, as well as 4 threads to remap positions.

When marking the bitfield the threads will show high CPU usage (`top -H`), however they are just waiting for
the L3 cache to complete the atomic operations. So they are not actually burning electricity but they keep
the cores busy, as such you shouldn't use more threads than you have physical cores, to let the other
hyper-threads do other work.

## Phase 3

Is in development at the moment.

## Phase 4

TODO

## Further Development

I'll keep working to finish the plotter, should take about 2 more weeks at the most (I do have a day job).

Donations are welcome, I joined the party late, aint got no chia yet with my little micro farm (35/50 TiB).

I hope this plotter will save you on buying a bunch of plotting hardware in the future.

XCH: xch1w5c2vv5ak08pczeph7tp5xmkl5762pdf3pyjkg9z4ks4ed55j3psgay0zh

(indeed, my address contains the word 'gay', what can I do about it (no offense),
at least I can easily see if it's mine)

## Future Plans

I do have some history with GPU mining, back in 2014 I was the first to open source a XPM GPU miner,
which was about 40x more efficient than the CPU miner. See my other repos.

As such, it's only a matter of time until i'll add OpenCL support to speed up the plotter even more,
keeping most of the load off the CPUs.
