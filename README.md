# MLCache: A Multi-Armed Bandit Policy for an Operating System Page Cache
by Renato Costa (renatomc@cs.ubc.ca) and Jose Pazos (jpazos@cs.ubc.ca)

MLCache is a Linux page cache replacement policy that uses the UCB1 algorithm
to calculate a score for each page in the page cache.

### Project Structure

* `linux/src`: Snapshot of the Linux kernel source tree used for this project (4.14.0-rc7).
MLCache adds a new driver (`drivers/mlcache`), a new tracepoint event (`include/trace/events/mlcache.h`),
as well as other in place changes.

* `linux/qemu`: Scripts are available in order to create a Debian disk image (using `debootstrap`) with
a previously built kernel image; and to run a QEMU virtual machine.

* `proposal`, `report`: the proposal and report for this project, respectively.

### Testing MLCache

When building the kernel, two configuration options are required:

```
CONFIG_MLCACHE=y
CONFIG_MLCACHE_ACTIVE=y
```

The first the enables MLCache driver; the second allows MLCache to actively
collect and update page data (see usage instructions below). If MLCache is
enabled but not active, it will just collect page hit/miss ratios, without
affecting the standard LRU eviction policy in Linux.

### Usage

In a kernel with MLCache enabled, there should be a `/proc/mlcache/filter` file in the system.
Writing to that file controls MLCache's operation:

```
# Checking the status of the MLCache driver
root@mlcache:~# cat /proc/mlcache/filter
[Active] MLCache is currently not enabled.

# Requesting MLCache to monitor the process tree under the running shell
root@mlcache:~# echo tree:$$ >/proc/mlcache/filter

# Check the status of the driver after the command above
root@mlcache:~# cat /proc/mlcache/filter
[Active] MLCache is watching the tree under process 1675

# Triggering some file system activity
root@mlcache:~# ls
fs.sh  linux  pg.sh  vanilla.fs.sh  vanilla.pg.sh

# Reading hit ratio statistics for the monitored processes
root@mlcache:~# cat /proc/mlcache/1675
Hits: 23 | Misses: 1
```
