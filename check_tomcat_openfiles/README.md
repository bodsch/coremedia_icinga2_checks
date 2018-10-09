# `check_tomcat_openfiles`

This check returns the opend files per application.



**The Redis service can also be set using the environment variable `REDIS_HOST`.**


## Usage

```bash
check_tomcat_openfiles [-R <redis_server>] -H <hostname> -A <application> -w <warning> -c <critical>
```

## Example
```bash
check_tomcat_openfiles -R moebius-monitoring.coremedia.vm -H moebius-backend.coremedia.vm -A content-management-server
OK - content-management-server keeps 103 files open | open_files=103;128;256
```
