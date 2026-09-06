$ErrorActionPreference = 'Stop'

$pong = redis-cli -p 6380 PING
if ($pong -ne 'PONG') { throw "PING failed: $pong" }

$set = redis-cli -p 6380 SET course software-engineering
if ($set -ne 'OK') { throw "SET failed: $set" }

$value = redis-cli -p 6380 GET course
if ($value -ne 'software-engineering') { throw "GET failed: $value" }

$exists = redis-cli -p 6380 EXISTS course
if ($exists -ne '1') { throw "EXISTS failed: $exists" }

$deleted = redis-cli -p 6380 DEL course
if ($deleted -ne '1') { throw "DEL failed: $deleted" }

Write-Host 'redis-cli acceptance passed'
