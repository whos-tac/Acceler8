$path = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\extracted_context.txt"
if (Test-Path $path) {
    $lines = Get-Content $path
    for ($i = 0; $i -lt $lines.Length; $i++) {
        $l = $lines[$i]
        if ($l -match "speed" -and $l -match "display" -and ($l -match "zero" -or $l -match "fail" -or $l -match "work" -or $l -match "issue")) {
            Write-Host "--- Match on line $i ---"
            $start = [Math]::Max(0, $i - 3)
            $end = [Math]::Min($lines.Length - 1, $i + 3)
            for ($j = $start; $j -le $end; $j++) {
                $prefix = if ($j -eq $i) { ">>> " } else { "    " }
                Write-Host "$prefix$($lines[$j])"
            }
            Write-Host "------------------------"
        }
    }
} else {
    Write-Host "File not found."
}
