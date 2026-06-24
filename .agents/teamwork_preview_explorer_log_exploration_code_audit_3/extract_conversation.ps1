$path = "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\conversation_text_5cf.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

$lines = Get-Content $path
foreach ($line in $lines) {
    try {
        $obj = ConvertFrom-Json $line
        # Only interested in text-based fields from inputs or outputs
        if ($obj.type -eq "USER_INPUT" -or $obj.type -eq "MODEL_RESPONSE" -or $obj.type -eq "PLANNER_RESPONSE" -or $obj.type -eq "EPHEMERAL_MESSAGE") {
            $txt = ""
            if ($obj.prompt) { $txt += "[PROMPT]`n" + $obj.prompt + "`n" }
            if ($obj.thinking_process) { $txt += "[THINKING]`n" + $obj.thinking_process + "`n" }
            if ($obj.content) { $txt += "[CONTENT]`n" + $obj.content + "`n" }
            
            if ($txt.Trim() -ne "") {
                $writer.WriteLine("==========================================================")
                $writer.WriteLine("Step $($obj.step_index) | Type: $($obj.type) | Source: $($obj.source)")
                $writer.WriteLine("==========================================================")
                $writer.WriteLine($txt)
                $writer.WriteLine()
            }
        }
    } catch {}
}
$writer.Close()
Write-Host "Done!"
