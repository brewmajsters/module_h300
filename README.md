# Module for H300 VFD

**Manufacturer:**  Forin.sk
**Model:** H300-OD75S2G
**Module type (code):**  VFD_H300
**Protocol:** Modbus-RTU
**Author:** Martin Stradiot

**Address format:**
Modbus-RTU address: number 1 to 255

**Datapoints:**

| Datapoint name | Datapoint code | Units | Writable | Description |
|:-:|:-:|:-:|:-:|:-:|
| Speed | SPEED | % * 100| true | Used to set frequency in percentage (two decimals precision) of maximum allowed frequency. Sign dependent on rotation direction. Possible values: <-10000,10000> for -100,00% - 100.00%. |
| Motion setting | SET_MOTION | - | true | Write-only datapoint used to control motor motion. Possible values: 1 - forward, 2 - reverse, 3 - forward jogging, 4 - reverse jogging, 5 - break-free stop, 6 - stop using breaks, 7 - error reset. |
| Motion state | GET_MOTION | - | false | Used to get motion state. Possible values: 1 - running forward, 2 - running reverse, 3 - stop |
| Statecode | STATE | - | false | Used to determine error state of VFD. Possible values: 0000: ok 0001: reserved 0002: overcurrent during acceleration 0003: overcurrent during deceleration 0004: overcurrent during constant speed 0005: overvoltage during acceleration 0006: overvoltage during deceleration 0007: overvoltage during constant speed 0008: control voltage dropout 0009: undervoltage 000A: VFD overdrive 000B: motor overdrive 000C: input phase dropout 000D: output phase dropout 000E: power module overheating 000F: external error 0010: communication error 0011: contactor error 0012: current detection error 0013: Autotunning error 0014: encoder or PG card error 0015: VFD EEPROM error 0016: VFD hardware error 0017: motor ground shortcut 0018: reserved 0019: reserved 001A: accumulative run time reached 001B: user input error 1 001C: user input error 2 001D: accumulative input voltage connection time reached 001E: zero load 001F: PID regulator feedback lost 0028: pulse control current limit violated 0029: runtime motor switch error 002A: speed error toleration violated 002B: motor speed limit violated 002D: motor overheating 005A: encoder setting error 005B: encoder disconnected 005C: init position error 005E: speed feedback error.
| Main frequency | MAIN_FREQUENCY | Hz * 100 | false | Used to read actual main frequency. Possible values: <0,+/- MAXIMAL_ALLOWED_MAIN_FREQENCY> (dependent on rotation direction). |
| Auxiliary frequency | AUX_FREQUENCY | Hz * 100 | false | Used to read actual aux frequency. Possible values: <0,+/- MAXIMAL_ALLOWED_AUX_FREQENCY> (dependent on rotation direction). |

**Wiring scheme**:
![Wiring scheme](/wiring.png)