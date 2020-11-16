# Module for H300 VFD

**Manufacturer:**  Forin.sk
**Model:** H300-OD75S2G
**Module type (code):**  VFD_H300
**Protocol:** Modbus-RTU
**Author:** Martin Stradiot

**Address format:**
Modbus-RTU address: number 1 to 255

**Datapoints:**

| Datapoint name | Datapoint code | Units | Writable | Description | Value example |
|:-:|:-:|:-:|:-:|:-:|:-:|
| Speed | SPEED | % | true | Read/Write datapoint used to set frequency in percentage (single decimal precision) of set frequency (SET_FREQ). Possible values: <0.0,100.0>. | 50.0 |
| Motion setting | SET_MOTION | - | true | Write-only datapoint used to control motor motion. Possible values: "FWD" - forward, "REV" - reverse, "FWD_JOG" - forward jogging, "REV_JOG" - reverse jogging, "STOP" - break-free stop, "BREAK" - stop using breaks | FWD |
| Motion state | GET_MOTION | - | false | Used to get motion state. Possible values: "FWD" - running forward, "REV" - running reverse, "STOP" - stop | REV |
| Status | STATE | - | false | Used to determine error state of VFD. Possible values: "OK" - no error, "ERROR %N" - error code, where %N id number. | ERROR 1 |
| Actual frequency | GET_FREQ | Hz | false | Actual output frequency. Two decimal precision. | 12.34 |
| Frequency setpoint | SET_FREQ | Hz | true | Read/Write datapoint used to set output frequency (at 100% SPEED). Two decimal precision. Possible values: <0.00,500> | 30.00 |
| Acceleration time | ACCEL_TIME | s | true | Read/Write datapoint used to set accceleration time in seconds. Time to reach set SPEED from stop position. Integer numbers only. | 10 |
| Deceleration time | DECEL_TIME | s | true | Read/Write datapoint used to set decceleration time in seconds. Time for to reach zero SPEED using breaking. Integer numbers only. | 20 |
| Actual remaining running time | GET_TIMER | min | false | Remaining time until the motor starts slowing down until fully stopped. Timer starts immediately after SET_MOTION. ACCEL_TIME is included in this timer, DECEL_TIME is not. Single decimal precision (6s). | 2.5 |
| Timer setpoint | SET_TIMER | min | true | Read/Write datapoint used to set the timer. This is the max value for GET_TIMER. Value 0 menas no timer - endless operation until stopped manually. Unlike GET_TIMER, this value does not represent actual remaining time, but rather the setpoint and does not change itself automatically. Single decimal precision (6s). | 2.5 |

**Wiring scheme**:
![Wiring scheme](/wiring.png)
