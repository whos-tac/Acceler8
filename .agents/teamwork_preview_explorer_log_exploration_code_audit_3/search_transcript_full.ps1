$path = "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\matches_5cf_full.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

$lines = Get-Content $path
foreach ($line in $lines) {
    try {
        $obj = ConvertFrom-Json $line
        $txt = ""
        if ($obj.content) { $txt += $obj.content + " " }
        if ($obj.prompt) { $txt += $obj.prompt + " " }
        if ($obj.thinking_process) { $txt += $obj.thinking_process + " " }
        
        if ($txt -match "speed" -or $txt -match "telemetry" -or $txt -match "CAN" -or $txt -match "VESC" -or $txt -match "Slave") {
            # Let's extract sentences with context
            $sentences = $txt -split "(?<=[.!?])\s+"
            $matchedSentences = @()
            foreach ($s in $sentences) {
                if ($s -match "speed" -or $s -match "telemetry" -or $s -match "CAN" -or $s -match "VESC" -or $s -match "Slave") {
                    $matchedSentences += $s.Trim()
                }
            }
            if ($matchedSentences.Count -gt 0) {
                $writer.WriteLine("--- Step $($obj.step_index) ($($obj.type)) ---")
                foreach ($ms in $matchedSentences) {
                    $writer.WriteLine("  * " + $ms)
                }
                $writer.WriteLine()
            }
        }
    } catch {}
}
$writer.Close()
Write-Host "Done!"
