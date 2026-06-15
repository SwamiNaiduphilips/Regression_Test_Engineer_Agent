import time
from typing import Dict, Any

try:
    from .hardware_loader import load_hid_interface_class
except ImportError:
    from hardware_loader import load_hid_interface_class

HID_Interface = load_hid_interface_class()


class ChargeTimeTestResult:
    def __init__(self, requirement_id: str, passed: bool, duration_seconds: float, details: Dict[str, Any]):
        self.requirement_id = requirement_id
        self.passed = passed
        self.duration_seconds = duration_seconds
        self.details = details


class ChargeTimeVerifier:
    def __init__(self, hid_device_name: str = 'ShaverAnalyser', timeout_minutes: int = 120):
        self.hid_device_name = hid_device_name
        self.timeout_minutes = timeout_minutes
        self.hid = None

    def _connect(self) -> bool:
        self.hid = HID_Interface(self.hid_device_name, isteststand=True)
        return self.hid.open_Philips_dongle()

    def _disconnect(self) -> None:
        if self.hid is not None:
            try:
                self.hid.reset_to_default()
            except Exception:
                pass
            try:
                self.hid.close_Philips_dongle()
            except Exception:
                pass
            self.hid = None

    def verify_charge_time(self, requirement_id: str, target_minutes: int = 60, poll_interval_seconds: int = 30) -> ChargeTimeTestResult:
        if not self._connect():
            raise RuntimeError('Unable to connect to HID dongle for charge verification')

        start_time = time.time()
        self.hid.charge(True)

        result_details: Dict[str, Any] = {
            'target_minutes': target_minutes,
            'poll_interval_seconds': poll_interval_seconds,
            'states': [],
        }

        try:
            while True:
                elapsed_seconds = time.time() - start_time
                charger_connected = self.hid.GET_chargerconnected()
                inlet_voltage = self.hid.GET_inlet_voltage()
                result_details['states'].append({
                    'elapsed_seconds': round(elapsed_seconds, 1),
                    'charger_connected': charger_connected,
                    'inlet_voltage': inlet_voltage,
                })

                if not charger_connected:
                    details = {
                        'reason': 'Charger disconnected during verification',
                        'elapsed_seconds': elapsed_seconds,
                        'inlet_voltage': inlet_voltage,
                    }
                    return ChargeTimeTestResult(requirement_id, False, elapsed_seconds, details)

                if elapsed_seconds >= target_minutes * 60:
                    passed = True
                    details = {
                        'reason': 'Charge time threshold reached',
                        'elapsed_seconds': elapsed_seconds,
                        'inlet_voltage': inlet_voltage,
                    }
                    return ChargeTimeTestResult(requirement_id, passed, elapsed_seconds, details)

                if elapsed_seconds >= self.timeout_minutes * 60:
                    details = {
                        'reason': 'Verification timed out before reaching target charge time',
                        'elapsed_seconds': elapsed_seconds,
                        'inlet_voltage': inlet_voltage,
                    }
                    return ChargeTimeTestResult(requirement_id, False, elapsed_seconds, details)

                time.sleep(poll_interval_seconds)

        finally:
            self.hid.charge(False)
            self._disconnect()
