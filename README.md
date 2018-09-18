# coremedia-icinga2-checks

This repository provides checks for Icinga2 that have been created specifically for the **CoreMedia Monitoring Toolbox**.

A Redis Service serves as a data basis in which the measuring points are stored temporarily.

The following checks are currently available:

 - [`check_app_cache`](./README.md#check_app_cache)
 - [`check_capconnection`](./README.md#check_capconnection)
 - [`check_feeder_status`](./README.md#check_feeder_status)
 - [`check_license`](./README.md#check_license)
 - [`check_nodeexporter_disk`](./README.md#check_nodeexporter_disk)
 - [`check_nodeexporter_load`](./README.md#check_nodeexporter_load)
 - [`check_nodeexporter_memory`](./README.md#check_nodeexporter_memory)
 - [`check_runlevel`](./README.md#check_runlevel)
 - [`check_sequencenumbers`](./README.md#check_sequencenumbers)
 - [`check_tomcat_memory`](./README.md#check_tomcat_memory)

## external dependencies

- `libhiredis-dev`
- `libev-dev`
- `cmake`

## build

Simply use the *Makefile*

- build only redox: `make redox`
- build full: `make`
- clean the project: `make clean`


## usage

### <a name="check_app_cache"></a>check_app_cache
```
check_app_cache -R moebius-monitoring.coremedia.vm  -H moebius-delivery.coremedia.vm  -A "cae-live-1" -C uapi-cache
OK - 5.3% UAPI Cache used<br>Max: 100.00 MB<br>Used: 5.30 MB | max=104857600 used=5552358 percent_used=5.3

check_app_cache -R moebius-monitoring.coremedia.vm  -H moebius-delivery.coremedia.vm  -A "cae-live-1" -C blob-cache
OK - 0% BLOB Cache used<br>Max: 10.00 GB<br>Used: 0.00 B | max=10737418240 used=0 percent_used=0
```

### <a name="check_capconnection"></a>check_capconnection
```
check_capconnection -R moebius-monitoring.coremedia.vm  -H moebius-delivery.coremedia.vm  -A cae-live-1
OK - Cap Connection <b>open</b>
```

### <a name="check_feeder_status"></a>check_feeder_status
```
check_feeder_status -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm -F caefeeder-live
OK - all 964 Elements feeded<br>Heartbeat 4892 ms | max=964 current=964 diff=0 heartbeat=4892

check_feeder_status -R moebius-monitoring.coremedia.vm  -H moebius-backend.coremedia.vm -F caefeeder-preview
OK - all 964 Elements feeded<br>Heartbeat 2911 ms | max=964 current=964 diff=0 heartbeat=2911

check_feeder_status -R moebius-monitoring.coremedia.vm  -H moebius-backend.coremedia.vm -F content-feeder
OK - Pending Events: 0 | pending_events=0
```

### <a name="check_license"></a>check_license
```
check_license -R moebius-monitoring.coremedia.vm  -H moebius-backend.coremedia.vm -C content-management-server --soft --hard
WARNING - soft: CoreMedia license is valid until 22.11.2018 - <b>64 days left</b> <br>
OK - hard: CoreMedia license is valid until 22.11.2018 - <b>64 days left</b>  | valid_soft=64;80;20 valid_hard=64;40;10
```

### <a name="check_nodeexporter_disk"></a>check_nodeexporter_disk
```
check_nodeexporter_disk -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm -P '/'
OK - partition '/' - size: 19.99 GB, used: 8.10 GB, used percent: 40% | size=21463302144 used=8702148608 used_percent=40
```

### <a name="check_nodeexporter_load"></a>check_nodeexporter_load
```
check_nodeexporter_load -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm
OK - load average: 0, 0.05, 0.07, (3 cpus)
```

### <a name="check_nodeexporter_memory"></a>check_nodeexporter_memory
```
check_nodeexporter_memory -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm
memory  - size: 2.78 GB, used: 1.80 GB, used percent: 64%
```

### <a name="check_runlevel"></a>check_runlevel
```
check_runlevel -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm --contentserver master-live-server
OK - Runlevel in <b>running</b> Mode
```

### <a name="check_sequencenumbers"></a>check_sequencenumbers
```
check_sequencenumbers -R moebius-monitoring.coremedia.vm  -r moebius-frontend.coremedia.vm
OK - RLS and MLS in sync<br>MLS Sequence Number: 1441<br>RLS Sequence Number: 1441 | diff=0 mls_seq_nr=1441 rls_seq_nr=1441
```

### <a name="check_tomcat_memory"></a>check_tomcat_memory
```
check_tomcat_memory -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm -A caefeeder-live -M perm-mem
CRITICAL - 96.07% Perm Memory used<br>Commited: 126.69 MB<br>Used: 121.70 MB | committed=132841472 used=127616816

check_tomcat_memory -R moebius-monitoring.coremedia.vm  -H moebius-frontend.coremedia.vm -A caefeeder-live -M heap-mem
OK - 43.61% Heap Memory used<br>Max: 999.06 MB<br>Commited: 178.98 MB<br>Used: 78.05 MB | max=1047592960 committed=187678720 used=81843824
```

