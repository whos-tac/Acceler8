# ESP-NOW Telemetry & Remote Architecture

The ACCELER8 dashboard incorporates a robust wireless protocol via **ESP-NOW**, operating on 2.4GHz Wi-Fi independently of standard routers. This allows for low-latency bidirectional communication between the dashboard, an external receiver, and a custom remote control.

## 1. Network Topology
The system defines specific MAC addresses for known peers:
*   **Remote MAC**: `D0:CF:13:32:42:3C`
*   **Receiver MAC**: `EC:64:C9:CC:D8:54`
*   **DashBoard MAC**: Implicitly dynamic, though it communicates actively with the Remote and Receiver.

## 2. Telemetry Broadcasting (Dash -> Remote)
The dashboard continuously sends a `TelemetryPacket` at a fixed rate of **10 Hz**.
*   **Data transmitted**: Speed (`speed_kmh`), Battery Voltage, Battery Current, Power (W), Motor Temp, MOSFET Temp, Range (km), and CAN connection status (`can_alive`).
*   **Use Case**: The remote control can display live telemetry data directly on its own screen.

## 3. Command & Status Reception (Receiver/Remote -> Dash)
The dashboard listens for various packets to adjust its state or trigger UI actions.
*   **`ReceiverStatusPacket`**: Indicates the status of the connection between the Receiver and the Remote. Modifies `g_vehicle_state.remote_disconnected`. The UI triggers an alert overlay if the remote disconnects.
*   **`ControlPacket`**: Contains button presses from the remote (`button_state`). These inputs are utilized by the `SettingsScreen` and the main `ui_controller`. Note: Bit 4 (CONFIRM) is handled as the horn by the receiver but operates as "Enter" inside Dash menus.
*   **`TelemetryPacket` (Mock/Debug)**: The dashboard can receive mock telemetry data from a dummy Receiver. This puts the system into `mock_mode_active = true`, overriding CAN data with wireless telemetry for UI testing and debugging without the physical vehicle.

## 4. Broadcasting Configuration (Dash -> Receiver)
The dashboard continuously sends an `EscConfigPacket` back to the receiver at **10 Hz**.
*   **Data transmitted**: `settings_active` (boolean), `gear` (0-3), `direction` (0-1), and `headlight_active` (boolean).
*   **Use Case**: Tells the receiver to change acceleration curves (gear), invert motor direction, actuate external headlights, and ignore throttle inputs while the rider is actively configuring the dashboard (`settings_active = true`).

## 5. Integration with Global State
When packets are received via `dash_onDataRecv`, they are injected into `g_vehicle_state` using the `DASH_LOCK()` mutex to ensure thread safety with the UI loop.

## 6. Packet Structures
*(Defined in `src/espnow_packets.h`)*
Ensure that any new metrics added to `TelemetryPacket` are kept structurally identical across all devices (Dash, Receiver, Remote) to prevent ESP-NOW parsing errors.

```cpp
struct TelemetryPacket {
    float speed_kmh;
    float battery_voltage_v;
    float battery_current_a;
    float power_w;
    float motor_temp_c;
    float mosfet_temp_c;
    float range_km;
    bool  can_alive;
    bool  remote_disconnected;
};

struct ControlPacket {
    float throttle_percent; // -100.0 to 100.0
    uint8_t button_state;   // bit 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT, 4=CONFIRM(Horn)
};

struct EscConfigPacket {
    bool settings_active;
    uint8_t gear;           // 0=No gear, 1=Low, 2=Med, 3=High
    uint8_t direction;      // 0=Forward, 1=Reverse
    bool headlight_active;
};
```
