$paths = @(
    "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    "C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
)
$outputFile = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\search_results.txt"
$writer = New-Object System.IO.StreamWriter($outputFile, $false, [System.Text.Encoding]::UTF8)

$keywords = @("speed", "CAN", "VESC", "familiar", "Slave", "voltage", "telemetry", "erpm")

foreach ($path in $paths) {
    if (-not (Test-Path $path)) { continue }
    $writer.WriteLine("================================================================================")
    $writer.WriteLine("SEARCHING FILE: $path")
    $writer.WriteLine("================================================================================")
    
    $lines = Get-Content $path
    foreach ($line in $lines) {
        try {
            $obj = ConvertFrom-Json $line
            $txt = ""
            if ($obj.content) { $txt += "CONTENT: " + $obj.content + "`n" }
            if ($obj.prompt) { $txt += "PROMPT: " + $obj.prompt + "`n" }
            if ($obj.thinking_process) { $txt += "THINKING_PROCESS: " + $obj.thinking_process + "`n" }
            
            # Check if any keyword matches
            $matchFound = $false
            foreach ($kw in $keywords) {
                if ($txt -match [regex]::Escape($kw)) {
                    $matchFound = $true
                    break
                }
            }
            
            if ($matchFound) {
                # Let's perform a deeper check. We want lines that discuss speed not displaying, CAN issue, etc.
                if ($txt -match "speed" -or $txt -match "familiar" -or $txt -match "Slave" -or $txt -match "telemetry") {
                    $writer.WriteLine("--------------------------------------------------------------------------------")
                    $writer.WriteLine("Step Index: $($obj.step_index) | Type: $($obj.type) | Source: $($obj.source)")
                    $writer.WriteLine("--------------------------------------------------------------------------------")
                    
                    # Output the text but split it into lines and only keep lines that match or nearby lines for context
                    $txtLines = $txt -split "`r?`n"
                    for ($i = 0; $i -lt $txtLines.Length; $i++) {
                        $tl = $txtLines[$i]
                        # Check if matches
                        $isMatch = $false
                        foreach ($kw in $keywords) {
                            if ($tl -match [regex]::Escape($kw)) {
                                $isMatch = $true
                                break
                            }
                        }
                        if ($isMatch) {
                            # Print a bit of context around it
                            $start = [Math]::Max(0, $i - 1)
                            $end = [Math]::Min($txtLines.Length - 1, $i + 1)
                            for ($j = $start; $j -le $end; $j++) {
                                $prefix = if ($j -eq $i) { ">>> " } else { "    " }
                                $writer.WriteLine("$prefix$($txtLines[$j])")
                            }
                            $writer.WriteLine("...")
                            # Skip ahead so we don't print duplicate context
                            $i = $end
                        }
                    }
                }
            }
        } catch {
            # Not JSON or parsing error
        }
    }
}

$writer.Close()
Write-Host "Done! Search results written to $outputFile"
