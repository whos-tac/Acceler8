$path = "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\search_results_5cf.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

$lines = Get-Content $path
foreach ($line in $lines) {
    if ($line -match "familiar" -or $line -match "speed" -or $line -match "CAN" -or $line -match "VESC" -or $line -match "Slave" -or $line -match "voltage") {
        try {
            $obj = ConvertFrom-Json $line
            $txt = ""
            if ($obj.content) { $txt += "CONTENT: " + $obj.content + "`n" }
            if ($obj.prompt) { $txt += "PROMPT: " + $obj.prompt + "`n" }
            if ($obj.thinking_process) { $txt += "THINKING: " + $obj.thinking_process + "`n" }
            
            # Print if it has any match
            if ($txt -match "familiar" -or $txt -match "Slave" -or $txt -match "speed" -or $txt -match "CAN" -or $txt -match "VESC") {
                $writer.WriteLine("=== Step $($obj.step_index) ===")
                $writer.WriteLine($txt)
                $writer.WriteLine()
            }
        } catch {}
    }
}
$writer.Close()
Write-Host "Done!"
