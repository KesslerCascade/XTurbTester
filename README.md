# XTurbTester
This program was written as a quick and dirty way to test the fire rate of turbo controllers. It supports controllers that use the XInput API such as XBox and compatible  controllers, and also is capable of using raw input to read non-XInput controllers that support the HID standard (most USB and Bluetooth gamepads).

## Usage
Simply make sure the controller you want to measure is selected from the drop-down and start pressing buttons. By default the first connected controller is selected. The Refresh button can be used to rescan the list if the hardware configuration is changed by plugging in or connecting a device.

## Readouts
| Name       | Description          |
| ---------- | -------------------- |
| **Calculated Rate (Hz)** | Turbo rates calculated based on the time between inputs. |
| Instant    | Instantaneous measurement. This is calculated based on the time elapsed between the last input and the input immediately prior to it. The formula is simply 1/dt, where dt is elapsed time in seconds. |
| Avg        | Running average of the instant rate. The time period automatically extends to try to capture inputs occurring at a similar frequency. |
| **Measured Rate (Hz)** | Actual measured turbo rates over various spans of time. |
| 1 sec      | The number of input events counted within the last second. |
| 2 sec      | The number of input events counted within the last two seconds, divided by 2. |
| ...        | The same applies for the other time period buckets. Just a count of how many inputs, divided by time. |
| **Input Spacing** | Measurement of the distribution of input events over the last 2 seconds. |
| Min        | Smallest amount of time between two inputs. |
| Avg        | Average amount of time between inputs. |
| Max        | Largest amount of time between two inputs. |
