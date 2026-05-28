# Logic Snapshot: Multi-ESC Telemetry Aggregation

To provide a unified view of the vehicle's state on a 4WD or 2WD board, the dashboard aggregates data from multiple CAN-connected motor controllers.

## Aggregation Strategy

### 1. Battery Power (Current & Watts)
- **Problem**: Each ESC reports its own battery draw.
- **Solution**: The `CANDriver` sums the `battery_current_a` from both the Master (163) and Slave (224).
- **Metric**: `g_vehicle_state.battery_current_a = Master_I + Slave_I`.
- **Result**: The "Watt" display on the UI reflects the true total power output of the machine.

### 2. Voltage (The Average)
- **Problem**: Minor voltage drops or measurement differences between ESCs.
- **Solution**: Average the voltage readings when both are available.
- **Metric**: `g_vehicle_state.battery_voltage_v = (Master_V + Slave_V) / 2`.

### 3. Temperature (The Safety Max)
- **Problem**: One motor or ESC might be overheating while the other is cool.
- **Solution**: Display the maximum temperature across all units.
- **Metric**: `g_vehicle_state.mosfet_temp_c = max(Master_FET, Slave_FET)`.
- **Reasoning**: This ensures the rider is warned if ANY component is reaching thermal limits.

### 4. Speed & ERPM
- **Problem**: Which motor's speed should be used for the odometer/speedometer?
- **Solution**: Use the Master (163) by default. If the Master is inactive, fallback to the Slave (224).
- **Logic**: 
  ```cpp
  if (master_alive) {
      g_vehicle_state.erpm = master_esc.erpm;
  } else if (slave_alive) {
      g_vehicle_state.erpm = slave_esc.erpm;
  }
  ```

---

## State Management
Individual ESC state is tracked in the `CANDriver::EscData` struct.
Aggregation is performed every `poll()` cycle using a 500ms liveness timeout for each ESC ID.
