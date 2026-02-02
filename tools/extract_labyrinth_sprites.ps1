# Extract sprites from labyrinth resource 63969 and 63970
# These appear to be tile/sprite data for the labyrinth

$inputDir = "C:\ggng\on_labrnth_raw"
$outputDir = "C:\ggng\on_labyrinth_sprites"
$paletteFile = "C:\ggng\on_palettes\on_labrnth_bmp.pal"

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$palette = [System.IO.File]::ReadAllBytes($paletteFile)

function Decompress-RLE {
    param([byte[]]$data, [int]$startOffset, [int]$width, [int]$height)

    $pixels = New-Object byte[] ($width * $height)
    for ($i = 0; $i -lt $pixels.Length; $i++) { $pixels[$i] = 0 }

    $pos = $startOffset
    $y = 0
    $x = 0

    while ($y -lt $height -and $pos -lt $data.Length) {
        $byte = $data[$pos]
        $pos++

        if ($byte -eq 0xFF -and ($pos + 1) -lt $data.Length) {
            $value = $data[$pos]
            $pos++
            $count = $data[$pos] + 1
            $pos++
            for ($i = 0; $i -lt $count -and $x -lt $width; $i++) {
                $pixels[$y * $width + $x] = $value
                $x++
            }
        }
        elseif ($byte -eq 0x00) {
            $y++
            $x = 0
        }
        else {
            if ($x -lt $width) {
                $pixels[$y * $width + $x] = $byte
                $x++
            }
        }
    }
    return $pixels
}

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
            $bmp[$dataOffset] = $pixels[$y * $width + $x]
            $dataOffset++
        }
        $dataOffset += $rowPadding
    }

    [System.IO.File]::WriteAllBytes($path, $bmp)
}

# Process sprite files
$files = @(
    @{name="UNKNOWN_63969"; count=39},
    @{name="UNKNOWN_63970"; count=43}
)

# Try different tile sizes
$tileSize = 64  # Try 64x64 first

foreach ($f in $files) {
    $path = "$inputDir\$($f.name).bin"
    $data = [System.IO.File]::ReadAllBytes($path)
    $spriteCount = $f.count

    Write-Host "Processing $($f.name) - $spriteCount sprites"

    # Header is 14 bytes + offset table
    $headerSize = 14 + $spriteCount * 4

    # Read offsets
    $offsets = @()
    for ($i = 0; $i -lt $spriteCount; $i++) {
        $offset = [BitConverter]::ToUInt32($data, 14 + $i * 4)
        $offsets += $offset
    }

    # Estimate dimensions from first sprite size
    if ($spriteCount -gt 1) {
        $firstSpriteSize = $offsets[1] - $offsets[0]
        Write-Host "First sprite size: $firstSpriteSize bytes"
    }

    # Extract each sprite
    for ($i = 0; $i -lt $spriteCount; $i++) {
        $offset = $offsets[$i]

        if ($offset -ge $data.Length) { continue }

        # Try to auto-detect size by counting row terminators
        $rowCount = 0
        $pos = $offset
        $endPos = if ($i -lt $spriteCount - 1) { $offsets[$i+1] } else { $data.Length }

        while ($pos -lt $endPos) {
            if ($data[$pos] -eq 0x00) { $rowCount++ }
            $pos++
        }

        if ($rowCount -eq 0) { $rowCount = 64 }
        $width = 64
        $height = $rowCount

        $pixels = Decompress-RLE -data $data -startOffset $offset -width $width -height $height

        $outPath = "$outputDir\$($f.name)_spr$($i.ToString('D3'))_${width}x${height}.bmp"
        Write-BMP8 -path $outPath -width $width -height $height -pixels $pixels -palette $palette
    }

    Write-Host "Extracted $spriteCount sprites"
    Write-Host ""
}

Write-Host "Done!"
