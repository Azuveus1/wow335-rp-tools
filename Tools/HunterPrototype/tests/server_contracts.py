"""Compatibility entry point: use disposable fixtures for escrow/mail tests."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('server_bounties.py')),run_name='__main__')
