# Drone Watcher 🛸

<img width="1354" height="1022" alt="HowItWorks" src="https://github.com/user-attachments/assets/5b4ee928-46bf-4b4f-8cdb-3cf047ed9eea" />


Detect and log drones using **SenseCAP Watcher + Wio Terminal**

![status](https://img.shields.io/badge/status-working-green)
![platform](https://img.shields.io/badge/platform-arduino-blue)
![hardware](https://img.shields.io/badge/hardware-seeedstudio-orange)
![license](https://img.shields.io/badge/license-MIT-lightgrey)

---

## Overview

A compact system that detects drones in real time using **computer vision (TinyML or Vision LLM)** and logs events to a **Wio Terminal**.

Designed to be:

* Simple to replicate
* Low-cost
* Modular and hackable

---

## Features

* Real-time drone detection (Vision LLM or TinyML)
* Visual + audio alerts (screen, LED, speaker)
* UART communication between devices
* On-screen event log (Wio Terminal)
* Persistent logging to microSD (`/drone_log.txt`)
* Tripod-ready enclosure
* No custom ML training required (optional)

---

## How it works

1. **Detection**
   SenseCAP Watcher monitors the scene through its camera.

2. **AI Processing**
   Uses either:

   * Custom TinyML model
   * Default Vision LLM

3. **Trigger**
   On detection:

   * Screen updates
   * LED flashes
   * Sound alert

4. **Logging**
   Event is sent via UART to Wio Terminal and stored on microSD.

---

## Hardware

* SenseCAP Watcher (AI camera)
* Wio Terminal (SAMD51)
* Jumper wires (male-male)
* microSD card
* 3D printed enclosure (optional)
* Tripod (1/4”-20 mount)

---

## Wiring (UART)

| SenseCAP Watcher | Wio Terminal |
| ---------------- | ------------ |
| GND              | GND          |
| 5V               | 5V           |
| Pin 19 (TX)      | Pin 10 (RX)  |
| Pin 20 (RX)      | Pin 8 (TX)   |

---

## Software

### Arduino IDE setup

Add board manager URL:

```
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```

Select:

```
Seeeduino Wio Terminal
```

Install libraries:

```cpp
#include <Seeed_Arduino_FS.h>
#include <ArduinoJson.h>
```

---

## SenseCAP Configuration

Using **SenseCraft App**:

1. Pair device
2. Select *SenseCAP Watcher*
3. Define detection in natural language

Example:

```
"If you see a drone, notify"
```

Then configure:

* Model: TinyML or Vision LLM
* Actions: LED / sound / screen / UART
* Capture frequency

---

## Usage

* Device starts monitoring automatically on power
* Wio Terminal displays detection log
* Use right button to scroll
* Press button to clear log

Log file:

```
/drone_log.txt
```

---

## Project Structure

```
/src          → Arduino code (Wio Terminal)
/enclosure    → Fusion360 + STL files
/docs         → Images and diagrams
README.md
```

---

## References

* https://wiki.seeedstudio.com/sensecraft-ai/tutorials/sensecraft-ai-pretrained-models-for-watcher/
* https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

---

## Notes

* Vision LLM works well for standard objects like drones
* TinyML is useful for custom detections
* UART allows easy integration with other systems

---

## License

MIT

