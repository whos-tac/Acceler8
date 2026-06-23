// ponytail: Self-contained ESC model matching esc_model.cpp behavior.
// Built minimal version, no extra dependencies or abstractions.

class ESCModel {
    constructor() {
        this.ramped_throttle = 0.0;
        this.erpm = 0.0;
        this.motor_current = 0.0;
        this.battery_current = 0.0;
        this.battery_voltage = 42.0;
        this.capacity_ah = 10.0; // Max 10.0 Ah
        this.voltage_sag = 0.0;
        this.power_w = 0.0;
        this.lvc_active = false;
        this.safe_start = false;
    }

    // ponytail: Map analog potentiometer reading to throttle percentage
    mapPotToThrottle(potVal, potCenter = 2048.0, potMin = 1000.0, potMax = 3000.0) {
        if (potVal === undefined || potVal === null || Number.isNaN(potVal)) {
            potVal = potCenter;
        }
        let potF = potVal;
        if (potF > potMax) potF = potMax;
        if (potF < potMin) potF = potMin;

        const deadbandHigh = potCenter + 100.0;
        const deadbandLow = potCenter - 100.0;
        let throttle = 0.0;

        if (potF > deadbandHigh) {
            const denom = potMax - deadbandHigh;
            if (denom >= 10.0) {
                throttle = ((potF - deadbandHigh) / denom) * 100.0;
            }
        } else if (potF < deadbandLow) {
            const denom = deadbandLow - potMin;
            if (denom >= 10.0) {
                throttle = ((potF - deadbandLow) / denom) * 100.0;
            }
        }

        if (throttle > 100.0) throttle = 100.0;
        if (throttle < -100.0) throttle = -100.0;
        return throttle;
    }

    // ponytail: Ramp and failsafe calculation
    calculateRampedThrottle(inputThrottle, dt, signalLost, settingsActive) {
        if (inputThrottle === undefined || inputThrottle === null || Number.isNaN(inputThrottle)) {
            inputThrottle = 0.0;
        }
        if (signalLost) {
            this.safe_start = false;
        }
        if (!signalLost && Math.abs(inputThrottle) <= 10.0 /* THROTTLE_DEADZONE */) {
            this.safe_start = true;
        }

        let target = 0.0;
        if (signalLost || settingsActive || !this.safe_start) {
            target = 0.0;
        } else {
            if (Math.abs(inputThrottle) <= 10.0) {
                target = 0.0;
            } else {
                const sign = (inputThrottle > 0.0) ? 1.0 : -1.0;
                target = sign * (Math.abs(inputThrottle) - 10.0) * (100.0 / (100.0 - 10.0));
            }
            if (target < 0.0) {
                // ponytail: scale target for brake throttle using MAX_BRAKE_CURRENT_A / MAX_DRIVE_CURRENT_A
                target *= (50.0 / 50.0);
            }
        }

        let timeLeft = dt;
        let iterations = 0;
        while (timeLeft > 0.0 && this.ramped_throttle !== target && iterations < 10) {
            iterations++;
            let currentRate;
            if (signalLost || settingsActive || !this.safe_start) {
                currentRate = 200.0; // FAILSAFE_COAST_RATE
            } else {
                let accelerating = false;
                if (target > 0.0 && target > this.ramped_throttle && this.ramped_throttle >= 0.0) {
                    accelerating = true;
                } else if (target < 0.0 && target < this.ramped_throttle && this.ramped_throttle <= 0.0) {
                    accelerating = true;
                }
                currentRate = accelerating ? 75.0 /* RAMP_RATE_PER_SEC */ : 500.0 /* RAMP_DOWN_RATE_PER_SEC */;
            }

            const maxDelta = currentRate * timeLeft;

            if (this.ramped_throttle < target) {
                if (this.ramped_throttle < 0.0 && target >= 0.0) {
                    const dist = 0.0 - this.ramped_throttle;
                    if (maxDelta > dist) {
                        this.ramped_throttle = 0.0;
                        timeLeft -= dist / currentRate;
                        continue;
                    }
                }
                this.ramped_throttle += maxDelta;
                if (this.ramped_throttle > target) this.ramped_throttle = target;
                break;
            } else {
                if (this.ramped_throttle > 0.0 && target <= 0.0) {
                    const dist = this.ramped_throttle - 0.0;
                    if (maxDelta > dist) {
                        this.ramped_throttle = 0.0;
                        timeLeft -= dist / currentRate;
                        continue;
                    }
                }
                this.ramped_throttle -= maxDelta;
                if (this.ramped_throttle < target) this.ramped_throttle = target;
                break;
            }
        }
    }

    // ponytail: Battery and ESC physics integration
    updateEscPhysics(dt, rapidDrain) {
        // 1. Calculate active capacity drain (max 10 Ah)
        let baseDrain = this.battery_current;
        let drainMultiplier = 1.0;
        if (rapidDrain) {
            baseDrain += 15.0; // Drains 15A at idle
            drainMultiplier = 30.0; // Scale up for visual feedback/test speed
        }
        const totalDrain = baseDrain * drainMultiplier;
        this.capacity_ah -= (totalDrain * dt) / 3600.0;
        if (this.capacity_ah < 0.0) this.capacity_ah = 0.0;

        // 2. Open circuit voltage based on capacity (linear between 30V at 0Ah and 42V at 10Ah)
        const maxCapacity = 10.0;
        let ocv = 30.0 + 12.0 * (this.capacity_ah / maxCapacity);
        if (ocv < 30.0) ocv = 30.0;
        if (ocv > 42.0) ocv = 42.0;

        // 3. Compute motor current and battery current from ramped throttle
        const dutyCycle = Math.abs(this.ramped_throttle) / 100.0;

        if (this.lvc_active) {
            this.motor_current = 0.0;
            this.battery_current = 0.0;
        } else {
            this.motor_current = dutyCycle * 50.0; // Max 50A
            this.battery_current = this.motor_current * dutyCycle;
        }

        // 4. Calculate voltage sag based on battery current and internal resistance (R_int = 0.1 Ohm)
        this.voltage_sag = this.battery_current * 0.1;
        this.battery_voltage = ocv - this.voltage_sag;

        // 5. Low Voltage Cutoff (LVC) check at 32.0V
        if (this.battery_voltage < 32.0) {
            this.lvc_active = true;
            this.motor_current = 0.0;
            this.battery_current = 0.0;
            this.voltage_sag = 0.0;
            this.battery_voltage = ocv; // recovers to OCV when load is removed
        }

        // ponytail: LVC should only be cleared when a recharge/reset capacity event occurs (capacity >= 9.9 Ah)
        if (this.lvc_active && this.capacity_ah >= 9.9) {
            this.lvc_active = false;
        }

        // 6. Calculate electrical power
        this.power_w = this.battery_voltage * this.battery_current;

        // 7. Update ERPM based on throttle, with lag (analytical solution to guarantee stability under large dt inputs)
        let targetErpm = 0.0;
        if (!this.lvc_active) {
            targetErpm = (this.ramped_throttle / 100.0) * 80000.0;
        }
        this.erpm = targetErpm - (targetErpm - this.erpm) * Math.exp(-2.0 * dt);
    }

    // ponytail: Recharge clears LVC and restores capacity
    recharge() {
        this.capacity_ah = 10.0;
        this.lvc_active = false;
    }
}

module.exports = ESCModel;
