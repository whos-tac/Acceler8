$path = "c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\search_results_5cf.txt"
$lines = Get-Content $path
$keywords = @("speed", "voltage", "CAN", "VESC", "mcu_id", "Slave", "master", "erpm", "0x0B", "0x0C", "0x0D")

# Let's search for lines containing both "speed" and "voltage" or "erpm" or "mcu_id" or "Slave"
for ($i = 0; $i -lt $lines.Length; $i++) {
    $line = $lines[$i]
    if ($line -match "Step Index:") {
        # Check the next few lines for discussion about why speed is not displaying
        $block = ""
        $j = $i
        while ($j -lt $lines.Length -and -not ($lines[$j] -match "--------------------------------------------------------------------------------")) {
            $block += $lines[$j] + "`n"
            $j++
        }
        
        # Check if the block discusses speed issue
        if ($block -match "speed" -and ($block -match "zero" -or $block -match "display" -or $block -match "bug" -or $block -match "issue" -or $block -match "slave" -or $block -match "mcu_id")) {
            if ($block -match "can_driver" -or $block -match "calculate_speed" -or $block -match "telemetry") {
                Write-Host "=== Matching Block (Step $($line)) ==="
                # Print first 10 lines of block
                $blines = $block -split "`n"
                $maxLines = [Math]::Min(15, $blines.Length)
                for ($k = 0; $k -lt $maxLines; $k++) {
                    Write-Host $blines[$k]
                }
                Write-Host "..."
                Write-Host "======================================"
            }
        }
        $i = $j - 1
    }
}
