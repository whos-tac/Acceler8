$paths = @(
    "C:\Users\thatw\.gemini\antigravity\brain\b4a5a63e-a7a5-47a9-893b-ac10e6d404a1\.system_generated\logs\transcript.jsonl",
    "C:\Users\thatw\.gemini\antigravity\brain\f38bcb63-cb25-4cea-aeba-ac95f4ee1c0a\.system_generated\logs\transcript.jsonl"
)
foreach ($path in $paths) {
    if (-not (Test-Path $path)) { continue }
    Write-Host "=== Searching $path ==="
    $lines = Get-Content $path
    foreach ($line in $lines) {
        if ($line -match "familiar") {
            try {
                $obj = ConvertFrom-Json $line
                Write-Host "Step $($obj.step_index) ($($obj.type)):"
                if ($obj.content) { Write-Host "CONTENT: $($obj.content)" }
                if ($obj.thinking_process) { Write-Host "THINKING: $($obj.thinking_process)" }
            } catch {
                Write-Host "Regex match but JSON parse failed: $_"
            }
        }
    }
}
