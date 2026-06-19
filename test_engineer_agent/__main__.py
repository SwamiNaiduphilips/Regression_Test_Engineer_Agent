import os
import sys

if __package__ is None and hasattr(sys, 'argv'):
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if repo_root not in sys.path:
        sys.path.insert(0, repo_root)
    from test_engineer_agent.charge_test import ChargeTimeVerifier
else:
    from .charge_test import ChargeTimeVerifier


def main():
    verifier = ChargeTimeVerifier(
        timeout_minutes=90,
        power_supply_address='GPIB0::5::INSTR',
        battery_simulator_address='GPIB0::1::INSTR',
        battery_simulator_channel=1,
        input_voltage=15.5,
        battery_voltage=3.7,
        current_threshold_ma=5.0,
    )
    result = verifier.verify_charge_time(
        requirement_id='REQ_CHARGE_60M',
        target_minutes=60,
        poll_interval_seconds=30,
    )

    print('Requirement ID:', result.requirement_id)
    print('Passed:', result.passed)
    print('Duration (s):', result.duration_seconds)
    print('Details:', result.details)


if __name__ == '__main__':
    main()
