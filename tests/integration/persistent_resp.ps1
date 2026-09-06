param([int]$Port = 6380)
$ErrorActionPreference = 'Stop'

function Read-Line([System.IO.Stream]$Stream) {
    $bytes = [System.Collections.Generic.List[byte]]::new()
    while ($true) {
        $value = $Stream.ReadByte()
        if ($value -lt 0) { throw 'Connection closed while reading RESP line' }
        if ($value -eq 13) {
            if ($Stream.ReadByte() -ne 10) { throw 'Invalid RESP line ending' }
            return [Text.Encoding]::UTF8.GetString($bytes.ToArray())
        }
        $bytes.Add([byte]$value)
    }
}

function Read-Response([System.IO.Stream]$Stream) {
    $prefix = [char]$Stream.ReadByte()
    if ($prefix -eq '+' -or $prefix -eq '-' -or $prefix -eq ':') {
        return "$prefix$(Read-Line $Stream)"
    }
    if ($prefix -eq '$') {
        $length = [int](Read-Line $Stream)
        if ($length -eq -1) { return '$-1' }
        $payload = New-Object byte[] $length
        $offset = 0
        while ($offset -lt $length) {
            $count = $Stream.Read($payload, $offset, $length - $offset)
            if ($count -eq 0) { throw 'Connection closed while reading bulk string' }
            $offset += $count
        }
        if ($Stream.ReadByte() -ne 13 -or $Stream.ReadByte() -ne 10) { throw 'Invalid bulk terminator' }
        return '$' + $length + ':' + [Text.Encoding]::UTF8.GetString($payload)
    }
    throw "Unexpected RESP prefix: $prefix"
}

$client = [Net.Sockets.TcpClient]::new('127.0.0.1', $Port)
try {
    $stream = $client.GetStream()
    $cases = @(
        @{ Request = "*1`r`n`$4`r`nPING`r`n"; Expected = '+PONG' },
        @{ Request = "*3`r`n`$3`r`nSET`r`n`$6`r`ncourse`r`n`$20`r`nsoftware-engineering`r`n"; Expected = '+OK' },
        @{ Request = "*2`r`n`$3`r`nGET`r`n`$6`r`ncourse`r`n"; Expected = '$20:software-engineering' },
        @{ Request = "*2`r`n`$6`r`nEXISTS`r`n`$6`r`ncourse`r`n"; Expected = ':1' },
        @{ Request = "*2`r`n`$3`r`nDEL`r`n`$6`r`ncourse`r`n"; Expected = ':1' }
    )
    foreach ($case in $cases) {
        $bytes = [Text.Encoding]::UTF8.GetBytes($case.Request)
        $stream.Write($bytes, 0, $bytes.Length)
        $actual = Read-Response $stream
        if ($actual -ne $case.Expected) { throw "Expected $($case.Expected), got $actual" }
    }
    Write-Host 'persistent RESP acceptance passed'
} finally {
    $client.Dispose()
}
