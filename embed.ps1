# embed.ps1 - Standard PNG Embedding (Base64)
$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$PiecesDir = "$Root\pieces"
$OutFile = "$Root\src\Assets.h"

Write-Host "Packing PNGs into Standalone Header..." -ForegroundColor Cyan

if (!(Test-Path $PiecesDir)) {
    Write-Warning "Pieces directory not found."
    exit
}

# 1. Create a single binary blob of all PNGs
$ms = [System.IO.MemoryStream]::new()
$bw = [System.IO.BinaryWriter]::new($ms)

$Files = Get-ChildItem "$PiecesDir\*.png"
$bw.Write([int]$Files.Count)

foreach ($f in $Files) {
    Write-Host "  Embedding $($f.Name)..." -ForegroundColor Gray
    $data = [System.IO.File]::ReadAllBytes($f.FullName)
    $nameBytes = [System.Text.Encoding]::ASCII.GetBytes($f.Name)
    
    $bw.Write([byte]$nameBytes.Length)
    $bw.Write($nameBytes)
    $bw.Write([int]$data.Length)
    $bw.Write($data)
}

# 2. Convert entire blob to Base64
$blob = [Convert]::ToBase64String($ms.ToArray())
$ms.Close()

# 3. Write Assets.h
$sb = [System.Text.StringBuilder]::new()
$sb.AppendLine("#pragma once`n#include <map>`n#include <vector>`n#include <string>`n#include <cstring>`n") | Out-Null
$sb.AppendLine("namespace Assets {") | Out-Null
$sb.AppendLine("    // Texture Cache: Filename -> PNG Binary Data") | Out-Null
$sb.AppendLine("    inline std::map<std::string, std::vector<unsigned char>>& GetCache() { static std::map<std::string, std::vector<unsigned char>> textures; return textures; }") | Out-Null
$sb.AppendLine("    static const std::string blob = ") | Out-Null

for ($i=0; $i -lt $blob.Length; $i += 120) {
    $len = [Math]::Min(120, $blob.Length - $i)
    $sb.AppendLine("        `"$($blob.Substring($i, $len))`"") | Out-Null
}
$sb.AppendLine("    ;") | Out-Null

$sb.AppendLine(@"
    inline std::vector<unsigned char> Decode(const std::string &in) {
        std::vector<unsigned char> out; std::vector<int> T(256, -1);
        const char* codes = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; i++) T[codes[i]] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
            if (T[c] == -1 || c == '=') continue;
            val = (val << 6) + T[c]; valb += 6;
            if (valb >= 0) { out.push_back((val >> valb) & 0xFF); valb -= 8; }
        }
        return out;
    }

    inline void Load() {
        auto& cache = GetCache();
        if (!cache.empty()) return;
        std::vector<unsigned char> data = Decode(blob);
        if (data.size() < 4) return;
        const unsigned char* p = data.data();
        int count = 0; std::memcpy(&count, p, 4); p += 4;
        for(int i=0; i<count; ++i) {
            int nLen = *p++; std::string name((const char*)p, nLen); p += nLen;
            int dLen = 0; std::memcpy(&dLen, p, 4); p += 4;
            cache[name] = std::vector<unsigned char>(p, p + dLen);
            p += dLen;
        }
    }
}
"@) | Out-Null

[System.IO.File]::WriteAllText($OutFile, $sb.ToString())
Write-Host "Success! Header Size: $([math]::Round($sb.Length / 1024, 1)) KB" -ForegroundColor Green