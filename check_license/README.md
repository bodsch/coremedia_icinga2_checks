# check_license

This check implements a check for the contentserver license.

Since version 170x exists an JMX measurement points for the valid license dates `LicenseValidUntilSoft` and `LicenseValidUntilHard` under the `Server` node.

These check can be run agains `soft`, `hard` or `both` license dates (see the examples below).


**The Redis service can also be set using the environment variable `REDIS_HOST`.**


## Usage

```bash
check_license v1.1.0
  Copyright (c) 2018 Bodo Schulz <bodo@boone-schulz.de>

This plugin will return the runlevel state corresponding to the content server
valid content server are:
  - content-management-server,
  - master-live-server and
  - replication-live-server

Usage:
 check_license [-R <redis_server>] -H <hostname> -C <content_server> --hard [--soft] [-w <WSOFT,WHARD>] [-c <CSOFT,CHARD>]

Options:
 -h, --help
    Print detailed help screen
 -V, --version
    Print version information
 -R, --redis
    (optional) the redis service who stored the measurements data.
 -H, --hostname
    the host to be checked.
 -C, --contentserver
    the content server.
 -t, --soft
    license valid until soft.
 -d, --hard
    license valid until hard.
 -w, --warning=WSOFT,WHARD
    exit with WARNING status if license average exceeds WSOFT or WHARD.
 -c, --critical=WSOFT,CHARD
    exit with CRITICAL status if license average exceeds CSOFT or CHARD.
```

## Example
```bash
check_license --hostname osmc.local  -C content-management-server --soft
soft: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (OK) | valid_soft=88

check_license --hostname osmc.local  -C content-management-server --hard --soft
soft: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (OK)<br>hard: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (OK) | valid_soft=88 valid_hard=88

check_license --hostname osmc.local  -C content-management-server -w 90 -c 10 --soft
soft: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (WARNING) | valid_soft=88

check_license --hostname osmc.local  -C content-management-server -w 80,50 -c 10,5 --hard --soft
soft: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (OK)<br>hard: CoreMedia license is valid until 06.06.2018 - <b>88 days left</b> (OK) | valid_soft=88 valid_hard=88

```
