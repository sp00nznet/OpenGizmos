# Decode labyrinth tilemap from Operation Neptune
# Format: 32-byte header + map info + RLE compressed tiles

$inputDir = "C:\ggng\on_labrnth_raw"
$outputDir = "C:\ggng\on_labyrinth_decoded"
$paletteFile = "C:\ggng\on_palettes\on_labrnth_bmp.pal"

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$palette = [System.IO.File]::ReadAllBytes($paletteFile)
Write-Host "Loaded palette: $($palette.Length) bytes"

function Write-BMP8 {
    param([string]$path, [int]$width, [int]$height, [byte[]]$pixels, [byte[]]$palette)

    $rowPadding = (4 - ($width % 4)) % 4
    $rowSize = $width + $rowPadding
    $pixelDataSize = $rowSize * $height
    $fileSize = 54 + 1024 + $pixelDataSize

    $bmp = New-Object byte[] $fileSize
    $bmp[0] = 0x42; $bmp[1] = 0x4D
    [BitConverter]::GetBytes([int]$fileSize).CopyTo($bmp, 2)
    [BitConverter]::GetBytes([int](54 + 1024)).CopyTo($bmp, 10)
    [BitConverter]::GetBytes([int]40).CopyTo($bmp, 14)
    [BitConverter]::GetBytes([int]$width).CopyTo($bmp, 18)
    [BitConverter]::GetBytes([int]$height).CopyTo($bmp, 22)
    $bmp[26] = 1; $bmp[28] = 8
    [BitConverter]::GetBytes([int]$pixelDataSize).CopyTo($bmp, 34)
    [BitConverter]::GetBytes([int]256).CopyTo($bmp, 46)
    [Array]::Copy($palette, 0, $bmp, 54, 1024)

    $dataOffset = 54 + 1024
    for ($y = $height - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $width; $x++) {
            $idx = $y * $width + $x
            if ($idx -lt $pixels.Length) {
                $bmp[$dataOffset] = $pixels[$idx]
            }
            $dataOffset++
        }
        $dataOffset += $rowPadding
    }

    [System.IO.File]::WriteAllBytes($path, $bmp)
}

# Process each labyrinth file
$files = Get-ChildItem "$inputDir\UNKNOWN_*.bin" | Sort-Object Name

foreach ($file in $files) {
    $data = [System.IO.File]::ReadAllBytes($file.FullName)
    $name = $file.BaseName

    Write-Host "=== Processing $name ($($data.Length) bytes) ==="

    # Read header info
    $version = [BitConverter]::ToUInt16($data, 0)
    $type = [BitConverter]::ToUInt16($data, 2)
    $flags = [BitConverter]::ToUInt16($data, 4)

    # Check for map dimensions at offset 38-41
    if ($data.Length -gt 42) {
        $width = [BitConverter]::ToUInt16($data, 38)
        $height = [BitConverter]::ToUInt16($data, 40)
        Write-Host "Dimensions: ${width}x${height}"

        if ($width -gt 0 -and $width -le 1024 -and $height -gt 0 -and $height -le 1024) {
            # Allocate pixel buffer
            $pixels = New-Object byte[] ($width * $height)
            for ($i = 0; $i -lt $pixels.Length; $i++) { $pixels[$i] = 0 }

            # Decompress RLE starting at offset 42
            $pos = 42
            $pixelIdx = 0

            while ($pos -lt $data.Length -and $pixelIdx -lt $pixels.Length) {
                $byte = $data[$pos]
                $pos++

                if ($byte -eq 0xFF -and ($pos + 1) -lt $data.Length) {
                    # RLE: FF <value> <count>
                    $value = $data[$pos]
                    $pos++
                    $count = $data[$pos] + 1
                    $pos++

                    for ($i = 0; $i -lt $count -and $pixelIdx -lt $pixels.Length; $i++) {
                        $pixels[$pixelIdx] = $value
                        $pixelIdx++
                    }
                }
                else {
                    # Literal pixel
                    $pixels[$pixelIdx] = $byte
                    $pixelIdx++
                }
            }

            Write-Host "Decompressed $pixelIdx pixels (expected: $($width * $height))"

            # Save as BMP
            $outPath = "$outputDir\${name}_${width}x${height}.bmp"
            Write-BMP8 -path $outPath -width $width -height $height -pixels $pixels -palette $palette
            Write-Host "Saved: $outPath"
        }
        else {
            Write-Host "Invalid dimensions: ${width}x${height}"
        }
    }
    Write-Host ""
}

Write-Host "Done!"
