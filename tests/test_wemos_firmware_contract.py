from pathlib import Path
import unittest

from src.motor_control import map_x_to_angle


FIRMWARE = (
    Path(__file__).parents[1]
    / "firmware"
    / "wemos_stepper_tachometer"
    / "wemos_stepper_tachometer.ino"
)


class WemosFirmwareContractTests(unittest.TestCase):
    def test_face_mapping_matches_tachometer_zero_to_180_profile(self):
        self.assertEqual(map_x_to_angle(0, 640, 0, 180, 90), 0)
        self.assertEqual(map_x_to_angle(320, 640, 0, 180, 90), 90)
        self.assertEqual(map_x_to_angle(640, 640, 0, 180, 90), 180)

    def test_firmware_uses_wemos_stepper_contract_without_credentials(self):
        source = FIRMWARE.read_text(encoding="utf-8")

        self.assertIn("#include <PubSubClient.h>", source)
        self.assertIn("#include <AccelStepper.h>", source)
        self.assertIn('TOPIC_SUB_ANGLE = "tachometer/angle"', source)
        self.assertIn("#define IN1 5", source)
        self.assertIn("#define IN2 4", source)
        self.assertIn("#define IN3 14", source)
        self.assertIn("#define IN4 12", source)
        self.assertIn("mqttClient.loop()", source)
        self.assertIn("stepper.run()", source)
        self.assertNotIn("Meeting@2024", source)
        self.assertNotIn("Main Hall", source)


if __name__ == "__main__":
    unittest.main()
