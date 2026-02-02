# Decode READER resources - mix of tilemaps and sprites
$inputDirs = @("C:\ggng\on_reader1_raw", "C:\ggng\on_reader2_raw")
$outputDir = "C:\ggng\on_reader_decoded"
$paletteFile = "C:\ggng\on_palettes\on_labrnth_bmp.pal"

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$palette = [System.IO.File]::ReadAllBytes($paletteFile)

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

foreach ($dir in $inputDirs) {
    $files = Get-ChildItem "$dir\*.bin" | Sort-Object Name

    foreach ($file in $files) {
        $data = [System.IO.File]::ReadAllBytes($file.FullName)
        $name = $file.BaseName

        if ($data.Length -lt 42) {
            Write-Host "Skipping $name - too small"
            continue
        }

        $spriteCount = [BitConverter]::ToUInt16($data, 2)
        $flags = [BitConverter]::ToUInt16($data, 4)

        Write-Host "Processing $name - count=$spriteCount, flags=$flags, size=$($data.Length)"

        if ($spriteCount -eq 1 -and $flags -eq 8) {
            # This is a tilemap resource
            $width = [BitConverter]::ToUInt16($data, 38)
            $height = [BitConverter]::ToUInt16($data, 40)

            if ($width -eq 640 -and $height -eq 480) {
                # Decode as partial/overlay image
                $pixels = New-Object byte[] ($width * $height)
                for ($i = 0; $i -lt $pixels.Length; $i++) { $pixels[$i] = 0 }

                $pos = 42
                $pixelIdx = 0

                while ($pos -lt $data.Length -and $pixelIdx -lt $pixels.Length) {
                    $byte = $data[$pos]
                    $pos++

                    if ($byte -eq 0xFF -and ($pos + 1) -lt $data.Length) {
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
                        $pixels[$pixelIdx] = $byte
                        $pixelIdx++
                    }
                }

                $outPath = "$outputDir\${name}_tilemap.bmp"
                Write-BMP8 -path $outPath -width $width -height $height -pixels $pixels -palette $palette
                Write-Host "  Saved tilemap: $pixelIdx pixels"
            }
        }
        else {
            # This might be a sprite resource
            Write-Host "  Sprite resource with $spriteCount entries"

            # Try to extract as sprites with estimated dimensions
            $headerSize = 14 + $spriteCount * 4
            if ($data.Length -gt $headerSize) {
                for ($i = 0; $i -lt $spriteCount; $i++) {
                    $offset = [BitConverter]::ToUInt32($data, 14 + $i * 4)

                    if ($offset -lt $data.Length) {
                        # Estimate dimensions from row terminators
                        $rowCount = 0
                        $endPos = if ($i -lt $spriteCount - 1) {
                            [BitConverter]::ToUInt32($data, 14 + ($i + 1) * 4)
                        } else {
                            $data.Length
                        }

                        for ($j = $offset; $j -lt $endPos; $j++) {
                            if ($data[$j] -eq 0x00) { $rowCount++ }
                        }

                        if ($rowCount -eq 0) { $rowCount = 32 }
                        $w = 64
                        $h = $rowCount

                        $pixels = New-Object byte[] ($w * $h)
                        for ($k = 0; $k -lt $pixels.Length; $k++) { $pixels[$k] = 0 }

                        $pos = $offset
                        $y = 0
                        $x = 0
                        while ($y -lt $h -and $pos -lt $endPos) {
                            $byte = $data[$pos]
                            $pos++
                            if ($byte -eq 0xFF -and ($pos + 1) -lt $endPos) {
                                $value = $data[$pos]
                                $pos++
                                $count = $data[$pos] + 1
                                $pos++
                                for ($k = 0; $k -lt $count -and $x -lt $w; $k++) {
                                    $pixels[$y * $w + $x] = $value
                                    $x++
                                }
                            }
                            elseif ($byte -eq 0x00) {
                                $y++
                                $x = 0
                            }
                            else {
                                if ($x -lt $w) {
                                    $pixels[$y * $w + $x] = $byte
                                    $x++
                                }
                            }
                        }

                        $outPath = "$outputDir\${name}_spr$($i.ToString('D2'))_${w}x${h}.bmp"
                        Write-BMP8 -path $outPath -width $w -height $h -pixels $pixels -palette $palette
                    }
                }
            }
        }
    }
}

Write-Host ""
Write-Host "Done!"
