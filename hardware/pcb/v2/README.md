# Hermit V2 PCB

## Background

Version 1 successfully validated the core power architecture. The ESP32-S3 operated correctly from both USB and battery power, and the 3.3 V rail reliably powered all I2C peripherals. A temporary block-terminal test point inserted in the battery input path also enabled current consumption measurements, which matched the original design expectations. As a result, no functional changes to the power system were required.

### Issues with V1

- ESP32 received power, but was not recognized as a valid device by USB.
- Data lines were not properly impedance matched (the USB D+ and D− signals must be routed as a 90 Ω ± 10% differential pair).

## Changes in V2

- Added power indicator LEDs for immediate visual confirmation of each power rail, eliminating the need to probe with a DMM during bring-up.
- Removed the temporary block-terminal current measurement point used during V1 validation.
- Optimized component placement and routing to reduce the overall PCB footprint.
- Used layer stackup information from our fabricator (JLC PCB) and Altium's trace impedance calculator to determine the exact trace width and spacing needed to meet USB impedance requirements.

## Altium Layouts

<table>
  <tr>
    <td align="center">
      <img src="Hermit_V2_2DLayout.png" alt="2D Layout" width="100%"><br>
      2D Layout
    </td>
    <td align="center">
      <img src="Hermit_V2_3DLayout.png" alt="3D Layout" width="100%"><br>
      3D Layout
    </td>
  </tr>
</table>

