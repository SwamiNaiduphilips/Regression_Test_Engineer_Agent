# Regression Test Engineer Agent

This repository contains a Python-based test engineer agent prototype for verifying device charge-time requirements.

## Structure

- `test_engineer_agent/`
  - `charge_test.py` - charge-time verification logic using the HID interface
  - `hardware_loader.py` - dynamic loader for existing hardware wrapper modules
  - `__main__.py` - runnable entrypoint for the agent
- `Hardware controll scripts/` - existing hardware integration modules used by the agent

## Usage

1. Install dependencies (see `requirements.txt`).
2. Run from the repository root:

```powershell
python -m test_engineer_agent
```

3. The agent will:
   - connect to the HID dongle
   - enable charging
   - poll charge state and inlet voltage until the target charge-time threshold is reached
   - stop charging and disconnect cleanly

## Notes

- The current implementation verifies whether the device remains charging for a configured target time, such as `60` minutes.
- The agent uses the existing `Hardware controll scripts/InletCommunication/HIDInterface.py` module.
- This repository is now initialized with Git and pushed to `https://github.com/SwamiNaiduphilips/Regression_Test_Engineer_Agent.git`.

## Requirements

The hardware scripts may require additional dependencies depending on your setup.

- Python 3.14+
- `pywinusb`
- `pyserial`
- `pyvisa`
- `nidaqmx`
