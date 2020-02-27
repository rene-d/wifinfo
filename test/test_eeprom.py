#!/usr/bin/env python3

import os
from pathlib import Path
import tempfile
import unittest
import subprocess
import sys


tools_dir = Path(__file__).parent.parent / "tools"

sys.path.append(tools_dir.as_posix())

from eeprom import read_eeprom, write_eeprom


class EEPRomTestCase(unittest.TestCase):
    def test_deserialize(self):
        build_dir = os.getenv("BUILD_DIR", (Path(__file__).parent.parent / "build").as_posix())

        config_bin = tempfile.NamedTemporaryFile("wb+")
        subprocess.check_output(["./gen_eeprom", "-o", config_bin.name], cwd=build_dir)
        eeprom = config_bin.read()
        config_bin.close()
        self.assertEqual(len(eeprom), 1024)

        config = read_eeprom(eeprom)

        self.assertEqual(config["ssid"], "reseauwifi")
        self.assertEqual(config["psk"], "motdepasse")
        self.assertEqual(config["psk"], "motdepasse")
        self.assertEqual(config["httpreq_freq"], 300)
        self.assertEqual(config["httpreq_port"], 80)

        eeprom2 = write_eeprom(config)
        self.assertEqual(eeprom2, eeprom)


if __name__ == "__main__":
    unittest.main(module="test_eeprom")
