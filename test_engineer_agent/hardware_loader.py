import importlib.util
from pathlib import Path


def _load_class_from_file(file_path: Path, class_name: str):
    if not file_path.exists():
        raise FileNotFoundError(f'Hardware file not found: {file_path}')

    spec = importlib.util.spec_from_file_location(file_path.stem, str(file_path))
    if spec is None or spec.loader is None:
        raise ImportError(f'Unable to load spec for {file_path}')

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    if not hasattr(module, class_name):
        raise ImportError(f'{class_name} class not found in {file_path}')

    return getattr(module, class_name)


def load_hid_interface_class():
    base_dir = Path(__file__).resolve().parent.parent
    hid_interface_path = base_dir / 'Hardware controll scripts' / 'InletCommunication' / 'HIDInterface.py'
    return _load_class_from_file(hid_interface_path, 'HID_Interface')


def load_battery_simulator_class():
    base_dir = Path(__file__).resolve().parent.parent
    simulator_path = base_dir / 'Hardware controll scripts' / 'BatterySimulator' / 'BatterySimulator.py'
    return _load_class_from_file(simulator_path, 'BatterySimulator')


def load_power_supply_class():
    base_dir = Path(__file__).resolve().parent.parent
    power_supply_path = base_dir / 'Hardware controll scripts' / 'PowerSupply' / 'PowerSupply.py'
    return _load_class_from_file(power_supply_path, 'PowerSupply')
