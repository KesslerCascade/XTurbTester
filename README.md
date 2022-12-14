# XTurbTester
![turbfunction](https://user-images.githubusercontent.com/117657415/200605854-22c64373-8801-4993-953a-1b41dc83e4db.png)

This program tests the fire rate of turbo controllers.

It supports controllers that use the XInput API such as XBox and compatible
controllers. It is also capable of using raw input to read non-XInput
controllers that support the HID standard (most USB and Bluetooth gamepads).

## Usage
Simply make sure the controller you want to measure is selected from the drop-down list and start mashing buttons!

By default the first connected controller is selected. The **Refresh** button can be used to rescan for devices, which
is useful if you plug in or connect a controller after the program is running.

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
