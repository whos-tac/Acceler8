$sessions = @("e6f644f6-811b-48e2-8d5b-d4fdbd907539", "ec3151b9-1f2d-4cbe-8ca7-ab9c263350dc", "f38bcb63-cb25-4cea-aeba-ac95f4ee1c0a")
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\familiar_matches.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

foreach ($sess in $sessions) {
    $path = "C:\Users\thatw\.gemini\antigravity\brain\$sess\.system_generated\logs\transcript.jsonl"
    if (-not (Test-Path $path)) { continue }
    $writer.WriteLine("==========================================================")
    $writer.WriteLine("SESSION: $sess")
    $writer.WriteLine("==========================================================")
    
    $lines = Get-Content $path
    foreach ($line in $lines) {
        try {
            $obj = ConvertFrom-Json $line
            $txt = ""
            if ($obj.content) { $txt += $obj.content + " " }
            if ($obj.prompt) { $txt += $obj.prompt + " " }
            if ($obj.thinking_process) { $txt += $obj.thinking_process + " " }
            
            if ($txt -match "familiar") {
                $writer.WriteLine("Step $($obj.step_index) | Type: $($obj.type)")
                $sentences = $txt -split "(?<=[.!?])\s+"
                foreach ($s in $sentences) {
                    if ($s -match "familiar") {
                        $writer.WriteLine("  * " + $s.Trim())
                    }
                }
                $writer.WriteLine()
            }
        } catch {}
    }
}
$writer.Close()
Write-Host "Done!"
