# `check_nodeexporter_openfiles`

This check reads open file descriptors provided by a `node-exporter`.



**The Redis service can also be set using the environment variable `REDIS_HOST`.**


## Usage

```bash
check_nodeexporter_openfiles [-R <redis_server>] -H <hostname>
```

## Example
```bash
check_nodeexporter_openfiles -R moebius-monitoring.coremedia.vm -H moebius-backend.coremedia.vm -c 5000
OK - 2304 of 586196 file descriptors open | open_files=2304;3072;5000
```
