# Extract sprites - handle both little-endian and big-endian header formats
# Operation Neptune SORTER.RSC uses two header formats

$inputDir = "C:\ggng\on_sorter_raw"
$outputDir = "C:\ggng\on_sprites_all"
$paletteFile = "C:\ggng\on_palettes\sorter_bmp.pal"

# Known dimensions
$knownDimensions = @{
    "35283" = @{w=94; h=109}
    "35368" = @{w=64; h=58}
    "35384" = @{w=55; h=47}
    "35299" = @{w=31; h=9}
    "35557" = @{w=43; h=43}
}

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

function Estimate-Dimensions {
    param([byte[]]$data, [int]$startOffset, [int]$spriteSize)

    $rowCount = 0
    $pos = $startOffset
    $endPos = [Math]::Min($startOffset + $spriteSize, $data.Length)

    while ($pos -lt $endPos) {
        if ($data[$pos] -eq 0x00) { $rowCount++ }
        $pos++
    }

    if ($rowCount -eq 0) { return @{w=32; h=32} }

    $estimatedPixels = $spriteSize * 2
    $estimatedWidth = [Math]::Max(16, [Math]::Min(256, [int]($estimatedPixels / $rowCount)))

    $commonWidths = @(16, 24, 32, 40, 48, 55, 64, 80, 94, 96, 128, 160, 192, 256)
    $closestW = $commonWidths | Sort-Object { [Math]::Abs($_ - $estimatedWidth) } | Select-Object -First 1

    return @{w=$closestW; h=$rowCount}
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

$spriteFiles = Get-ChildItem "$inputDir\CUSTOM_32513_*.bin" | Sort-Object { [int]($_.BaseName -replace 'CUSTOM_32513_', '') }

$totalSprites = 0
$totalResources = 0

foreach ($file in $spriteFiles) {
    $resourceId = $file.BaseName -replace 'CUSTOM_32513_', ''
    $data = [System.IO.File]::ReadAllBytes($file.FullName)

    if ($data.Length -lt 16) {
        Write-Host "Skipping $resourceId - too small"
        continue
    }

    # Detect header format
    $isBigEndian = ($data[0] -eq 0x00 -and $data[1] -eq 0x01)

    if ($isBigEndian) {
        # Big-endian format: 00 01 00 NN ...
        $spriteCount = $data[2] * 256 + $data[3]
        $headerSize = 16 + $spriteCount * 4
        $offsetTableStart = 16

        if ($spriteCount -eq 0 -or $spriteCount -gt 500 -or $data.Length -lt $headerSize) {
            Write-Host "Skipping $resourceId (BE) - invalid count $spriteCount"
            continue
        }

        # Read offsets - they appear to be little-endian 32-bit despite BE header
        $offsets = @()
        for ($i = 0; $i -lt $spriteCount; $i++) {
            $offset = [BitConverter]::ToUInt32($data, $offsetTableStart + $i * 4)
            $offsets += $offset
        }
    }
    else {
        # Little-endian format: 01 00 NN NN ...
        $version = [BitConverter]::ToUInt16($data, 0)
        $spriteCount = [BitConverter]::ToUInt16($data, 2)
        $headerSize = 14 + $spriteCount * 4
        $offsetTableStart = 14

        if ($spriteCount -eq 0 -or $spriteCount -gt 500 -or $data.Length -lt $headerSize) {
            Write-Host "Skipping $resourceId (LE) - invalid count $spriteCount"
            continue
        }

        $offsets = @()
        for ($i = 0; $i -lt $spriteCount; $i++) {
            $offset = [BitConverter]::ToUInt32($data, $offsetTableStart + $i * 4)
            $offsets += $offset
        }
    }

    # Get dimensions
    if ($knownDimensions.ContainsKey($resourceId)) {
        $dims = $knownDimensions[$resourceId]
        $width = $dims.w
        $height = $dims.h
        $format = if ($isBigEndian) { "BE-known" } else { "LE-known" }
    }
    else {
        $firstOffset = $offsets[0]
        if ($spriteCount -gt 1) {
            $spriteSize = $offsets[1] - $offsets[0]
        }
        else {
            $spriteSize = $data.Length - $firstOffset
        }
        $dims = Estimate-Dimensions -data $data -startOffset $firstOffset -spriteSize ([Math]::Abs($spriteSize))
        $width = $dims.w
        $height = $dims.h
        $format = if ($isBigEndian) { "BE-est" } else { "LE-est" }
    }

    Write-Host "Processing $resourceId ($format) - $spriteCount sprites, ${width}x${height}"
    $totalResources++

    # Extract each sprite
    for ($i = 0; $i -lt $spriteCount; $i++) {
        $offset = $offsets[$i]

        if ($offset -ge $data.Length) { continue }

        $pixels = Decompress-RLE -data $data -startOffset $offset -width $width -height $height

        $outPath = "$outputDir\idx${resourceId}_spr$($i.ToString('D3'))_${width}x${height}.bmp"
        Write-BMP8 -path $outPath -width $width -height $height -pixels $pixels -palette $palette
        $totalSprites++
    }
}

Write-Host ""
Write-Host "Done! Extracted $totalSprites sprites from $totalResources resources"
