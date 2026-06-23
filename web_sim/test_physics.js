// ponytail: Minimal unit tests for the ESC physics model using Node.js assert.
const assert = require('assert');
const ESCModel = require('./esc_model');

function runTest(name, fn) {
    console.log(`Running ${name}...`);
    try {
        fn();
        console.log(`${name} PASSED`);
        return true;
    } catch (err) {
        console.error(`${name} FAILED:`, err);
        return false;
    }
}

function test_throttle_mapping() {
    const model = new ESCModel();
    const center = 2048.0;
    const minVal = 1000.0;
    const maxVal = 3000.0;

    // 1. Center is deadband, should map to 0
    let t_center = model.mapPotToThrottle(center, center, minVal, maxVal);
    assert.ok(Math.abs(t_center - 0.0) < 0.01);

    // 2. Deadband bounds (+- 100 around center)
    let t_db_high = model.mapPotToThrottle(center + 100.0, center, minVal, maxVal);
    assert.ok(Math.abs(t_db_high - 0.0) < 0.01);

    let t_db_low = model.mapPotToThrottle(center - 100.0, center, minVal, maxVal);
    assert.ok(Math.abs(t_db_low - 0.0) < 0.01);

    // 3. Just outside deadband
    let t_outside_high = model.mapPotToThrottle(center + 101.0, center, minVal, maxVal);
    assert.ok(t_outside_high > 0.0);

    let t_outside_low = model.mapPotToThrottle(center - 101.0, center, minVal, maxVal);
    assert.ok(t_outside_low < 0.0);

    // 4. Maximum values
    let t_max = model.mapPotToThrottle(maxVal, center, minVal, maxVal);
    assert.ok(Math.abs(t_max - 100.0) < 0.01);

    let t_min = model.mapPotToThrottle(minVal, center, minVal, maxVal);
    assert.ok(Math.abs(t_min - -100.0) < 0.01);

    // 5. Midpoint mapping
    let db_high = center + 100.0;
    let mid_high = db_high + (maxVal - db_high) / 2.0;
    let t_mid_high = model.mapPotToThrottle(mid_high, center, minVal, maxVal);
    assert.ok(Math.abs(t_mid_high - 50.0) < 0.01);
}

function test_ramping_failsafe() {
    const model = new ESCModel();
    // 1. Safe start logic: input throttle must be within deadband to enable safe_start
    model.calculateRampedThrottle(50.0, 0.1, false, false);
    assert.strictEqual(model.safe_start, false);
    assert.ok(Math.abs(model.ramped_throttle - 0.0) < 0.01); // Should not ramp up before safe start

    model.calculateRampedThrottle(0.0, 0.1, false, false);
    assert.strictEqual(model.safe_start, true);

    // 2. Standard acceleration ramp (75% / sec)
    // Starting at 0%, target is 100%. After 1 second, it should be 75.0%
    model.calculateRampedThrottle(100.0, 1.0, false, false);
    assert.ok(Math.abs(model.ramped_throttle - 75.0) < 0.01);

    // 3. Fast decay on brake/deceleration (500% / sec)
    // Target is 0%, starting at 75%. After 0.1 seconds, it should decay by 50% to 25%
    model.calculateRampedThrottle(0.0, 0.1, false, false);
    assert.ok(Math.abs(model.ramped_throttle - 25.0) < 0.01);

    // 4. Coast decay during signal loss (200% / sec)
    // Target is 0%, starting at 25%, signal_lost = true. After 0.1 seconds, decay by 20% to 5%
    model.ramped_throttle = 25.0;
    model.calculateRampedThrottle(100.0, 0.1, true, false);
    assert.ok(Math.abs(model.ramped_throttle - 5.0) < 0.01);
    assert.strictEqual(model.safe_start, false); // safe_start reset to false on signal loss
}

function test_failsafe_timeout_coasting() {
    const model = new ESCModel();
    model.safe_start = true;
    model.ramped_throttle = 50.0; // Start with some throttle

    // We simulate comm loss.
    let now = 0;
    let last_rx = 0; // Comm received at t=0
    
    // After 240ms (now = 240)
    now = 240;
    let signalLost = (now - last_rx) > 250;
    assert.strictEqual(signalLost, false);
    model.calculateRampedThrottle(100.0, 0.24, signalLost, false);
    // At 75%/s acceleration, target is 100%. Ramped throttle = 50.0 + 75 * 0.24 = 68.0%
    assert.ok(Math.abs(model.ramped_throttle - 68.0) < 0.01);

    // Now we step to 260ms (now = 260ms, dt = 0.02s since last update).
    // Time since last rx is 260ms > 250ms, so signal is lost.
    now = 260;
    signalLost = (now - last_rx) > 250;
    assert.strictEqual(signalLost, true);
    model.calculateRampedThrottle(100.0, 0.02, signalLost, false);
    // When signal is lost, target becomes 0.0, safe_start becomes false, and coast rate is 200%/s.
    // Over dt = 0.02s, ramped throttle should decay by 200 * 0.02 = 4.0%
    // 68.0 - 4.0 = 64.0%
    assert.ok(Math.abs(model.ramped_throttle - 64.0) < 0.01);
    assert.strictEqual(model.safe_start, false);
}

function test_battery_model() {
    const model = new ESCModel();
    model.ramped_throttle = 100.0; // 100% throttle
    model.capacity_ah = 10.0; // Full capacity
    model.lvc_active = false;
    model.erpm = 0.0;

    // 1. Initial update: check current calculation and voltage sag
    // 100% throttle -> duty cycle = 1.0 -> motor_current = 50A -> battery_current = 50A
    // Voltage sag = 50 * 0.1 = 5.0V
    // OCV at 10.0Ah = 42.0V -> loaded_voltage = 37.0V
    model.updateEscPhysics(0.0, false);
    assert.ok(Math.abs(model.motor_current - 50.0) < 0.01);
    assert.ok(Math.abs(model.battery_current - 50.0) < 0.01);
    assert.ok(Math.abs(model.voltage_sag - 5.0) < 0.01);
    assert.ok(Math.abs(model.battery_voltage - 37.0) < 0.01);
    assert.ok(Math.abs(model.power_w - (37.0 * 50.0)) < 0.01);

    // 2. Capacity drain
    // Update over 72 seconds (72s = 0.02 hours)
    // Battery current = 50A -> drain = 50A * 0.02h = 1.0 Ah
    // Capacity should become 9.0 Ah
    model.updateEscPhysics(72.0, false);
    assert.ok(Math.abs(model.capacity_ah - 9.0) < 0.01);

    // 3. Rapid drain multiplier
    // With rapid drain = true, drain current = base_drain (50A) + 15A = 65A
    // multiplied by 30x = 1950 A equivalent drain
    // Update over 1.8 seconds (1.8s = 0.0005h)
    // drain = 1950A * 0.0005h = 0.975 Ah
    // Capacity should decrease from 9.0Ah to ~8.025 Ah
    model.updateEscPhysics(1.8, true);
    assert.ok(Math.abs(model.capacity_ah - 8.025) < 0.01);

    // 4. Low-Voltage Cutoff (LVC)
    // Force capacity down close to 0 to trigger LVC (LVC at 32V)
    model.capacity_ah = 0.5; // OCV = 30 + 12 * 0.05 = 30.6V
    // Update physics: even without load, OCV is 30.6V < 32.0V, so it should trigger LVC immediately
    model.updateEscPhysics(0.1, false);
    assert.strictEqual(model.lvc_active, true);
    assert.ok(Math.abs(model.motor_current - 0.0) < 0.01);
    assert.ok(Math.abs(model.battery_current - 0.0) < 0.01);
    assert.ok(Math.abs(model.battery_voltage - 30.6) < 0.01); // Recovered to OCV since current is 0

    // 5. Recharge / Recovery
    model.capacity_ah = 10.0; // recharge
    model.updateEscPhysics(0.1, false);
    assert.strictEqual(model.lvc_active, false); // should recover
}

function test_lvc_chatter_fix() {
    const model = new ESCModel();
    model.ramped_throttle = 100.0;
    model.capacity_ah = 0.5; // low capacity, triggers LVC

    // Run one update to trigger LVC
    model.updateEscPhysics(0.1, false);
    assert.strictEqual(model.lvc_active, true);
    assert.ok(Math.abs(model.motor_current - 0.0) < 0.01);
    assert.ok(Math.abs(model.battery_voltage - 30.6) < 0.01); // Recovered voltage = 30.6V

    // Now, even if capacity is slightly increased to make voltage recover above 33.0V (e.g. capacity = 3.0Ah, voltage = 30 + 12 * 0.3 = 33.6V)
    // LVC should still be active because capacity is not >= 9.9 Ah
    model.capacity_ah = 3.0;
    model.updateEscPhysics(0.1, false);
    assert.strictEqual(model.lvc_active, true);

    // Now, recharge capacity to 9.9 Ah
    model.capacity_ah = 9.9;
    model.updateEscPhysics(0.1, false);
    assert.strictEqual(model.lvc_active, false); // Should recover now!
}

function test_integration_stability() {
    const model = new ESCModel();
    model.ramped_throttle = 100.0;
    model.erpm = 0.0;

    // Call updateEscPhysics with a very large dt = 10.0 seconds.
    // Analytical integration guarantees it stays stable.
    model.updateEscPhysics(10.0, false);
    assert.ok(model.erpm > 0.0);
    assert.ok(model.erpm <= 80000.0 + 0.1);
    assert.ok(Math.abs(model.erpm - 80000.0) < 1.0); // Should have converged to target erpm

    // 2. Test large timestep dt = 72.0s
    model.erpm = 0.0;
    model.capacity_ah = 10000.0; // Large capacity to prevent LVC
    model.updateEscPhysics(72.0, false);
    assert.ok(model.erpm > 0.0);
    assert.ok(model.erpm <= 80000.0 + 0.1);
    assert.ok(Math.abs(model.erpm - 80000.0) < 0.001);

    // 3. Test extremely large timestep dt = 100000.0s
    model.erpm = 0.0;
    model.capacity_ah = 10000000.0; // Large capacity to prevent LVC
    model.updateEscPhysics(100000.0, false);
    assert.ok(model.erpm > 0.0);
    assert.ok(model.erpm <= 80000.0 + 0.1);
    assert.ok(Math.abs(model.erpm - 80000.0) < 0.001);

    // 4. Test dt = 0.0s
    model.erpm = 50000.0;
    model.capacity_ah = 10.0;
    model.updateEscPhysics(0.0, false);
    assert.ok(Math.abs(model.erpm - 50000.0) < 0.001);
}

function test_brake_throttle_scaling() {
    const model = new ESCModel();
    model.safe_start = true;

    // Input negative throttle: -50%
    // Target throttle = -((50 - 10) / 90) * 100% = -44.44%
    // Scaled by (50.0/50.0) = -44.44%
    model.calculateRampedThrottle(-50.0, 1.0, false, false);
    assert.ok(Math.abs(model.ramped_throttle - -44.44) < 0.1);
}

function test_nan_sanitization() {
    const model = new ESCModel();
    model.safe_start = true;

    // input NaN throttle, should not crash, should be treated as 0.0
    model.calculateRampedThrottle(NaN, 0.1, false, false);
    assert.ok(Math.abs(model.ramped_throttle - 0.0) < 0.01);

    // input NaN to mapPotToThrottle, should map to 0 (since it uses center)
    let t_nan = model.mapPotToThrottle(NaN, 2048.0, 1000.0, 3000.0);
    assert.ok(Math.abs(t_nan - 0.0) < 0.01);
}

function test_recharge() {
    const model = new ESCModel();
    model.capacity_ah = 0.5;
    model.lvc_active = true;

    model.recharge();
    assert.strictEqual(model.capacity_ah, 10.0);
    assert.strictEqual(model.lvc_active, false);
}

let success = true;
success &= runTest('test_throttle_mapping', test_throttle_mapping);
success &= runTest('test_ramping_failsafe', test_ramping_failsafe);
success &= runTest('test_failsafe_timeout_coasting', test_failsafe_timeout_coasting);
success &= runTest('test_battery_model', test_battery_model);
success &= runTest('test_lvc_chatter_fix', test_lvc_chatter_fix);
success &= runTest('test_integration_stability', test_integration_stability);
success &= runTest('test_brake_throttle_scaling', test_brake_throttle_scaling);
success &= runTest('test_nan_sanitization', test_nan_sanitization);
success &= runTest('test_recharge', test_recharge);

if (success) {
    console.log("ALL TESTS PASSED SUCCESSFULLY!");
    process.exit(0);
} else {
    console.error("SOME TESTS FAILED!");
    process.exit(1);
}
