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
