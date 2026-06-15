import os
import sys

if __package__ is None and hasattr(sys, 'argv'):
    pkg_dir = os.path.dirname(os.path.abspath(__file__))
    if pkg_dir not in sys.path:
        sys.path.insert(0, pkg_dir)
    from charge_test import ChargeTimeVerifier
else:
    from .charge_test import ChargeTimeVerifier


def main():
    verifier = ChargeTimeVerifier(hid_device_name='ShaverAnalyser', timeout_minutes=90)
    result = verifier.verify_charge_time(requirement_id='REQ_CHARGE_60M', target_minutes=60, poll_interval_seconds=30)

    print('Requirement ID:', result.requirement_id)
    print('Passed:', result.passed)
    print('Duration (s):', result.duration_seconds)
    print('Details:', result.details)


if __name__ == '__main__':
    main()
