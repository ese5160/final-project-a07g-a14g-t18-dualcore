# a07g-exploring-the-CLI

* **Team Number: 18**
* **Team Name: DualCore**
* **Team Members: Peng Su, Yuanchao Zhang**
* **GitHub Repository URL: https://github.com/ese5160/a00g-devices-designs-diagrams-s25-dualcore**
* **Description of hardware (embedded hardware, laptop, etc): (1)Apple M3 Pro, macOS Sonoma (2) Apple M1 Pro - Macbook**

## 1. Software Architecture

## Part (1) : Update the requirements of  HRS & SRS  based on the latest modifications

## Hardware Requirements Specification (HRS)

#### HRS 01 – Microcontroller & PCB Base

The project shall be based on a custom PCB integrating a suitable microcontroller (SAM W25). This PCB must provide:

- General-purpose I/O for motor control and sensor data acquisition.
  
- At least one communication interface (e.g., UART/SPI/I²C) for the LCD screen.
  
- Wireless connectivity (Wi-Fi/Bluetooth) for smartphone control, or headers for an external module.

#### HRS 02 – Sensors  

The system shall incorporate:  

- A **Temperature-Humidity Sensor** (e.g., DHTxx/SHTxx) to measure ambient conditions.
  
- An **Air Quality Sensor** (e.g., MQ-series, CCS811, or similar) capable of detecting smoke or poor air quality.  

All sensors must operate under the expected environmental conditions (e.g., indoor ranges of temperature and humidity) and provide reliable, stable readings.

#### HRS 03 – Motor Driver & Window Actuator

A motor driver circuit (integrated on the custom PCB or via a module) shall deliver the necessary current/voltage to a DC or Continuous Rotation Servo that adjusts the window’s opening angle. This includes:

- Support for PWM or position feedback control when we use Continuous Rotation Servo.
  
- Overcurrent or thermal protection mechanisms.
  
- Secure mechanical coupling to the window’s frame or rod.

#### HRS 04 – PDLC Film Power and Control

The PCB shall feature a step-up converter to approximately 48 V DC, plus a DC-to-AC conversion stage, enabling segmented control of the PDLC film for time display:

- Each 7-segment “digit” on the glass must be individually switchable.
  
- The system shall refresh the displayed time at a minimum of once per minute (using the onboard RTC).
  
- Safe isolation and protective measures against high-voltage lines are required.

#### HRS 05 – Switch Reed Integration

The system shall incorporate a reed switch to ensure precise detection of the window’s fully closed state. This includes:

- Interrupt-based or polling-based detection mechanism integrated with the microcontroller.

- Automatic stopping of the servo motor when the closed position is detected.

- Reliable and durable placement to avoid false triggers or misalignment over time.

#### HRS 06 – Steel Flat-Edge Sliding Window Roller Assembly

- The system shall include a steel flat-edge sliding window roller assembly to ensure smooth and fluent movement during window opening and closing. This includes:

- High-durability rollers capable of supporting the window's weight.

- Minimal friction design for seamless motion.

- Secure mounting and alignment to prevent jamming or misalignment.

#### HRS 07 – Voice System  

The hardware design shall allow for an optional voice module, including:  

- A small speaker or amplifier on the PCB or connected via headers.
  
- A microcontroller interface (e.g., UART or I²S) for TTS or pre-recorded audio playback.
  
- Adequate power supply to drive the speaker without distortion.

#### HRS 08 – Housing & Mounting

All PCB components must fit within the acrylic window frame enclosure to protect electronics. Additionally:  

- Mounting points on the PCB for screws or standoffs to secure it in place.
  
- The LCD shall be flush-mounted or otherwise integrated into the frame for user viewing.
  
- Proper ventilation or heat dissipation strategies for voltage regulators and motor drivers.

#### HRS 09 – Safety & Protection  

The hardware design shall include protection features such as:  

- Overvoltage/overcurrent safeguards on motor driver and PDLC driver stages.
  
- Reverse-polarity protection or fuses as needed.
  
- Adequate insulation for high-voltage lines (48 V AC) powering the PDLC film.

## Software Requirements Specification (SRS)

**SRS 01 - Window and Environment Data Retrieval:** 

The system shall enable users to access real-time window status (open/close/angle) and environmental data (temperature, humidity, air quality) via a smartphone app.

**SRS 02 - Window Time Adjustment:** 

The system shall allow users to modify automatic window operating times (e.g., morning open, evening close) through the app.

**SRS 03 - Manual Window Opening:** 

The app shall provide a feature to open the window to a desired angle using an “Open Window” button.

**SRS 04 - Automated Window Opening by Time:** 

Users shall be able to set specific times for automatic window opening in the app.

**SRS 05 - Automated Window Opening by Smoke:** 

If the air quality sensor detects smoke or poor air quality, the system shall immediately open the window for ventilation (highest priority).

**SRS 06 - Manual Window Closing:** 

The app shall include a "Open Window" and "Close Window" button for manual window closure or angle adjustment.

**SRS 07 - Automated Window Closing by Time:** 

Users shall be able to program specific times for automatic window closure in the app.

**SRS 08 - Automated Window Closing by Rain:** 

When rain or high humidity is detected, the system shall close the window to prevent water from entering. This has a lower priority than smoke detection but higher than standard scheduling.

**SRS 09 - PDLC Film Time and Weather Display：** 

Using PDLC film segments on the glass, the system shall display the current time and weather in a 7-segment style (HH: MM), driven by an internal or external RTC.

**SRS 10 - Voice System / Spoken Alerts:**

The system shall support a voice module to announce time, environmental alerts (smoke/rain), or user notifications.

**SRS 11 - Window Opening/Closing Constraints:**

The system shall prevent opening if the window is already fully open and prevent closing if it is already fully closed, avoiding redundant motor actions.

## Part (2) : A block diagram outlining the different tasks

![alt text](Block_Diagram_Outline.drawio.svg)
