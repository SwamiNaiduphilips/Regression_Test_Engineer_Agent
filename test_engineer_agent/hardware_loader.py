import importlib.util
from pathlib import Path


def load_hid_interface_class():
    base_dir = Path(__file__).resolve().parent.parent
    hid_interface_path = base_dir / 'Hardware controll scripts' / 'InletCommunication' / 'HIDInterface.py'

    if not hid_interface_path.exists():
        raise FileNotFoundError(f'HID interface file not found: {hid_interface_path}')

    spec = importlib.util.spec_from_file_location('hid_interface', str(hid_interface_path))
    if spec is None or spec.loader is None:
        raise ImportError(f'Unable to load spec for {hid_interface_path}')

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    if not hasattr(module, 'HID_Interface'):
        raise ImportError('HID_Interface class not found in HIDInterface.py')

    return module.HID_Interface
