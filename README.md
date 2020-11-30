# Module for H300 VFD

**Manufacturer:**  Forin.sk
**Model:** H300-OD75S2G
**Module type (code):**  VFD\_H300
**Protocol:** Modbus-RTU
**Author:** Martin Stradiot

**Address format:**
Modbus-RTU address: number 1 to 255

**Datapoints:**

| Datapoint name | Datapoint code | Units | Writable | Description | Value example |
|:-:|:-:|:-:|:-:|:-:|:-:|
| Speed | SPEED | % | true | Read/Write datapoint used to set frequency in percentage (single decimal precision) of set frequency (SET\_FREQ). Possible values: <0.0,100.0>. | 50.0 |
| Motion setting | SET\_MOTION | - | true | Write-only datapoint used to control motor motion. Possible values: "FWD" - forward, "REV" - reverse, "FWD\_JOG" - forward jogging, "REV\_JOG" - reverse jogging, "STOP" - break-free stop, "BREAK" - stop using breaks | FWD |
| Motion state | GET\_MOTION | - | false | Used to get motion state. Possible values: "FWD" - running forward, "REV" - running reverse, "STOP" - stop | REV |
| Status | STATE | - | false | Used to determine error state of VFD. Possible values: "OK" - no error, "ERROR %N" - error code, where %N id number. | ERROR 1 |
| Actual frequency | GET\_FREQ | Hz | false | Actual output frequency. Two decimal precision. | 12.34 |
| Frequency setpoint | SET\_FREQ | Hz | true | Read/Write datapoint used to set output frequency (at 100% SPEED). Two decimal precision. Possible values: <0.00,500> | 30.00 |
| Acceleration time | ACCEL\_TIME | s | true | Read/Write datapoint used to set accceleration time in seconds. Time to reach set SPEED from stop position. Integer numbers only. | 10 |
| Deceleration time | DECEL\_TIME | s | true | Read/Write datapoint used to set decceleration time in seconds. Time for to reach zero SPEED using breaking. Integer numbers only. | 20 |
| Actual remaining running time | GET\_TIMER | min | false | Remaining time until the motor starts slowing down until fully stopped. Timer starts immediately after SET\_MOTION. ACCEL\_TIME is included in this timer, DECEL\_TIME is not. Single decimal precision (6s). | 2.5 |
| Timer setpoint | SET\_TIMER | min | true | Read/Write datapoint used to set the timer. This is the max value for GET\_TIMER. Value 0 menas no timer - endless operation until stopped manually. Unlike GET\_TIMER, this value does not represent actual remaining time, but rather the setpoint and does not change itself automatically. Single decimal precision (6s). | 2.5 |
| Actual RPM | RPM | rpm | false | Used to get actual motor rpm (rounds per minute). This datapoint is calculated and represents value in ideal conditions with no load attached to the motor. | 420 |

**Wiring scheme**:
![Wiring scheme](/wiring.png)
