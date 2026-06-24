$path = "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\matching_steps.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

$lines = Get-Content $path
foreach ($line in $lines) {
    try {
        $obj = ConvertFrom-Json $line
        $txt = ""
        if ($obj.content) { $txt += $obj.content + " " }
        if ($obj.prompt) { $txt += $obj.prompt + " " }
        if ($obj.thinking_process) { $txt += $obj.thinking_process + " " }
        
        $match = $false
        if ($txt -match "speed" -and $txt -match "CAN" -and $txt -match "telemetry") { $match = $true }
        
        if ($match) {
            $writer.WriteLine("Step $($obj.step_index) | Type: $($obj.type)")
            $writer.WriteLine("---------------------------------------------")
            # Print any sentences containing both speed and CAN/telemetry
            $sentences = $txt -split "(?<=[.!?])\s+"
            foreach ($s in $sentences) {
                if ($s -match "speed" -and ($s -match "CAN" -or $s -match "telemetry" -or $s -match "VESC" -or $s -match "zero")) {
                    $writer.WriteLine("  * " + $s.Trim())
                }
            }
            $writer.WriteLine()
        }
    } catch {}
}
$writer.Close()
Write-Host "Done!"
