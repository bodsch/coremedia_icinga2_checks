# `check_sequencenumers`

This check returns the difference of sequencenumbers between an `replication-live-server` ans his `master-live-server`.

Since version 170x can the `replication-live-server` provide his `master-live-server`.

For the RLS we use the data from the JMX Endpoint `LatestCompletedSequenceNumber` under the `Replicator` node
The MLS provide his data under `RepositorySequenceNumber` under the `Server` node.



**The Redis service can also be set using the environment variable `REDIS_HOST`.**


## Usage

```bash

```

## Example
```bash

```
