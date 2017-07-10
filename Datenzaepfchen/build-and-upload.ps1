## Powershell-script to build and upload Datenzaepfchen to the board. Includes
## building and writing of the SPIFFS filesystem and the sketch code.

## SPIFFS-stuff is lifted out of the Arduino-IDE-plugin "upload sketch data",
## which at the core only calls external tools.

# Tool-Executable locations:
$mkspiffs = "C:\Users\Marian Aldenhövel\AppData\Local\Arduino15\packages\esp8266\tools\mkspiffs\0.1.2\mkspiffs.exe"
$esptool = "C:\Users\Marian Aldenhövel\AppData\Local\Arduino15\packages\esp8266\tools\esptool\0.4.9\esptool.exe"
$arduino = "C:\Program Files (x86)\Arduino\arduino_debug.exe"

# From Arduino-IDE preferences. These ARE board- or host-specific!

# For mkspiffs.exe:
$spiPage = 256 
$spiBlock = 8192
$spiSize = 3052 * 1024

# For esptool.exe
$resetMethod = "nodemcu"
$uploadSpeed = 256000
$serialPort = "COM4"
$uploadAddress = "0x100000"

function Compress-GZip {
    [cmdletbinding(SupportsShouldProcess=$True,ConfirmImpact="Low")]
    param (
        [Alias("PSPath")][parameter(mandatory=$true,ValueFromPipeline=$true,ValueFromPipelineByPropertyName=$true)][string]$FullName
    )
    Process {
        $_BufferSize = 1024 * 8
        if (Test-Path -Path $FullName -PathType Leaf) {
            # Write-Host "Reading from: $FullName"
            $tmpPath = ls -Path $FullName
            $GZipPath = Join-Path -Path ($tmpPath.DirectoryName) -ChildPath ($tmpPath.Name + '.gz')
            
            if (Test-Path -Path $GZipPath -PathType Leaf -IsValid) {
                # Write-Host "Compressing to: $GZipPath"
            } else {
                # Write-Host -Message "$FullName is not a valid path/file"
                return
            }
        } else {
            Write-Error -Message "$FullName does not exist"
            return
        }

        Write-Host "Compressing $FullName..."
        
        $input = New-Object System.IO.FileStream (ls -path $FullName).FullName, ([IO.FileMode]::Open), ([IO.FileAccess]::Read), ([IO.FileShare]::Read);
        
        New-Item $GZipPath -Type file -Force | Out-Null
        $output = New-Object System.IO.FileStream (ls -path $GZipPath).FullName, ([IO.FileMode]::Create), ([IO.FileAccess]::Write), ([IO.FileShare]::None)
        $gzipStream = New-Object System.IO.Compression.GzipStream $output, ([IO.Compression.CompressionMode]::Compress)

        try {
            $buffer = New-Object byte[]($_BufferSize);
            while ($true) {
                $read = $input.Read($buffer, 0, ($_BufferSize))
                if ($read -le 0) {
                    break;
                }
                $gzipStream.Write($buffer, 0, $read)
            }
        }
        finally {
            $gzipStream.Close();
            $output.Close();
            $input.Close();
        }

        # Remove source file.
        Remove-Item $FullName -Force
        
    }
}

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Prepare build folder..."
if (!(Test-Path -Path "build" )){
    New-Item -ItemType directory -Path "build" | Out-Null
} else {
	Remove-Item -Recurse -Force "build/*"
}

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Copy filesystem data..."
Copy-Item -Path "data" -Destination "build" -Force -Recurse -PassThru | ?{$_ -is [system.io.fileinfo]} | Select -Property FullName | Out-String

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Compress filesystem data..."
Get-ChildItem -Path "build/data" -Recurse -Exclude *.gz | 
    Where {!$_.PSIsContainer} |
    Where {$_.Length -gt 0} |
    Select-Object FullName |
    Compress-GZip

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Create SPIFFS image..."
Write-Host -ForegroundColor Yellow "mkspiffs.exe --create .\build\data --page $spiPage --block $spiBlock --size $spiSize --debug 5 build\spiffs.bin"
Start-Process $mkspiffs -ArgumentList "--create", ".\build\data", "--page", $spiPage, "--block", $spiBlock, "--size", $spiSize, "--debug", 5, "build\spiffs.bin" -Wait -NoNewWindow

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Compile and upload sketch..."
Write-Host -ForegroundColor Yellow "arduino[-debug].exe --upload --verbose-build --verbose-upload JarOfPrimes.ino" 
Start-Process $arduino -ArgumentList "--upload",  "--verbose-build", "--verbose-upload", "JarOfPrimes.ino" -Wait -NoNewWindow

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Upload SPIFFS image..."
Write-Host -ForegroundColor Yellow "esptool.exe -cd $resetMethod -cb $uploadSpeed -cp $serialPort -ca $uploadAddress -cf build\spiffs.bin"
Start-Process $esptool -ArgumentList "-cd", $resetMethod, "-cb", $uploadSpeed, "-cp", $serialPort, "-ca", $uploadAddress, "-cf", "build\spiffs.bin" -Wait -NoNewWindow

Write-Host -ForegroundColor Yellow ""
Write-Host -ForegroundColor Yellow "--------------------------------------------------------------------------------------"
Write-Host -ForegroundColor Yellow "Done."
#>