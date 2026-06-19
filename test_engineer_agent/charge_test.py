import time
from typing import Dict, Any

try:
    from .hardware_loader import (
        load_battery_simulator_class,
        load_power_supply_class,
    )
except ImportError:
    from hardware_loader import (
        load_battery_simulator_class,
        load_power_supply_class,
    )

BatterySimulator = load_battery_simulator_class()
PowerSupply = load_power_supply_class()


class ChargeTimeTestResult:
    def __init__(self, requirement_id: str, passed: bool, duration_seconds: float, details: Dict[str, Any]):
        self.requirement_id = requirement_id
        self.passed = passed
        self.duration_seconds = duration_seconds
        self.details = details


class ChargeTimeVerifier:
    def __init__(
        self,
        timeout_minutes: int = 120,
        power_supply_address: str = 'GPIB0::5::INSTR',
        battery_simulator_address: str = 'GPIB0::1::INSTR',
        battery_simulator_channel: int = 1,
        input_voltage: float = 15.5,
        battery_voltage: float = 3.7,
        current_threshold_ma: float = 5.0,
    ):
        self.timeout_minutes = timeout_minutes
        self.power_supply_address = power_supply_address
        self.battery_simulator_address = battery_simulator_address
        self.battery_simulator_channel = battery_simulator_channel
        self.input_voltage = input_voltage
        self.battery_voltage = battery_voltage
        self.current_threshold_ma = current_threshold_ma

        self.power_supply = None
        self.battery_simulator = None

    def _open_power_supply(self) -> bool:
        if not self.power_supply_address:
            return False

        self.power_supply = PowerSupply(self.power_supply_address)
        try:
            self.power_supply.setVoltage(self.input_voltage)
            self.power_supply.Output(True)
            return True
        except Exception:
            self._disconnect_power_supply()
            return False

    def _disconnect_power_supply(self) -> None:
        if self.power_supply is not None:
            try:
                self.power_supply.Output(False)
            except Exception:
                pass
            try:
                self.power_supply.closeConnection()
            except Exception:
                pass
            self.power_supply = None

    def _open_battery_simulator(self) -> bool:
        if not self.battery_simulator_address:
            return False

        self.battery_simulator = BatterySimulator(self.battery_simulator_address, self.battery_simulator_channel)
        try:
            self.battery_simulator.setVoltage(self.battery_voltage)
            self.battery_simulator.setCurrent(5)
            self.battery_simulator.setCurrentRange(1)
            self.battery_simulator.Output(True)
            return True
        except Exception:
            self._disconnect_battery_simulator()
            return False

    def _disconnect_battery_simulator(self) -> None:
        if self.battery_simulator is not None:
            try:
                self.battery_simulator.Output(False)
            except Exception:
                pass
            try:
                self.battery_simulator.closeConnection()
            except Exception:
                pass
            self.battery_simulator = None

    def verify_charge_time(
        self,
        requirement_id: str,
        target_minutes: int = 60,
        poll_interval_seconds: int = 30,
    ) -> ChargeTimeTestResult:
        if not self._open_power_supply():
            raise RuntimeError('Unable to initialize power supply for charge verification')

        if not self._open_battery_simulator():
            self._disconnect_power_supply()
            raise RuntimeError('Unable to initialize battery simulator for charge verification')

        start_time = time.time()

        result_details: Dict[str, Any] = {
            'target_minutes': target_minutes,
            'poll_interval_seconds': poll_interval_seconds,
            'input_voltage': self.input_voltage,
            'battery_voltage': self.battery_voltage,
            'current_threshold_ma': self.current_threshold_ma,
            'states': [],
        }

        try:
            while True:
                elapsed_seconds = time.time() - start_time
                battery_current = self.battery_simulator.getCurrent()

                result_details['states'].append({
                    'elapsed_seconds': round(elapsed_seconds, 1),
                    'battery_current_amps': battery_current,
                })

                if battery_current <= self.current_threshold_ma / 1000.0:
                    details = {
                        'reason': 'Charge current reached threshold',
                        'elapsed_seconds': elapsed_seconds,
                        'battery_current_amps': battery_current,
                        'current_threshold_ma': self.current_threshold_ma,
                    }
                    passed = elapsed_seconds <= target_minutes * 60
                    return ChargeTimeTestResult(requirement_id, passed, elapsed_seconds, details)

                if elapsed_seconds >= target_minutes * 60:
                    details = {
                        'reason': 'Target duration reached before current fell below threshold',
                        'elapsed_seconds': elapsed_seconds,
                        'battery_current_amps': battery_current,
                    }
                    return ChargeTimeTestResult(requirement_id, False, elapsed_seconds, details)

                if elapsed_seconds >= self.timeout_minutes * 60:
                    details = {
                        'reason': 'Verification timed out before reaching current threshold',
                        'elapsed_seconds': elapsed_seconds,
                        'battery_current_amps': battery_current,
                    }
                    return ChargeTimeTestResult(requirement_id, False, elapsed_seconds, details)

                time.sleep(poll_interval_seconds)

        finally:
            self._disconnect_battery_simulator()
            self._disconnect_power_supply()
