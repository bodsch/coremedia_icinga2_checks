# `check_app_cache`

This allows you to monitor both UAPI and BLOB caches.

For this purpose, the corresponding **JMX** parameters are read from the Redis Service.

The measurement points `HeapCacheSize` and `HeapCacheLevel` are used for the UAPI cache under `CapConnection`.

For the BLOB caches, the measuring points `BlobCacheSize` and `BlobCacheLevel`.

The Redis service can also be set using the environment variable `REDIS_HOST`.


## Usage

```bash
check_app_cache --help

check_app_cache v1.0.0
  Copyright (c) 2018 Bodo Schulz <bodo@boone-schulz.de>

This plugin will return the used tomcat memory corresponding to the content server
valid cache types are:
  - uapi-cache and
  - blob-cache

Usage:
 check_app_cache [-R <redis_server>] -H <hostname> -C <cache type>

Options:
 -h, --help
    Print detailed help screen
 -V, --version
    Print version information
 -R, --redis
    (optional) the redis service who stored the measurements data.
 -H, --hostname
    the host to be checked.
 -A, --application
    the name of the application
 -C, --cache
    the application cache type.
```

## Example
