# Deterministic Real-Time Firmware on ESP32

A FreeRTOS demo that shows how **priority-based preemption** keeps time-critical work (an LED heartbeat and a servo sweep) running on schedule while a heavy, CPU-hogging workload (audio feature extraction and neural-network inference) runs in the background on the same core.

The heavy workload is **simulated** with a 200 ms busy-wait, so the timing behaviour can be studied in isolation, without needing a trained model or a microphone.

---

## Why this exists

In a single `loop()`, anything that takes 200 ms blocks everything else for 200 ms. A servo that needs an update every 15 ms and an LED that must toggle every 500 ms both miss their deadlines the moment an inference call starts. Replacing delays with `millis()` checks does not fix this, because the long computation still cannot be interrupted.

An RTOS scheduler fixes it by **preempting** the long task whenever a higher-priority task becomes ready, then resuming it afterwards.

---

## Hardware

| Component | Role | Pin |
|---|---|---|
| ESP32 dev board | Microcontroller | n/a |
| LED (onboard or external) | 1 Hz heartbeat | GPIO 2 |
| Hobby servo (e.g. SG90) | Continuous 0°–180° sweep | GPIO 18 |

**Wiring notes**
- Power the servo from a separate 5 V supply (not the ESP32's 3.3 V pin) and connect its ground to the ESP32 ground.
- GPIO 2 drives the onboard LED on most ESP32 dev boards. If yours has none, wire an LED with a resistor to that pin.

---

## Task design

All three tasks are pinned to **Core 1** so they must compete for the same CPU.

| Task | Priority | Behaviour | Period |
|---|---|---|---|
| `vTaskBlinkLED` | 3 (highest) | Toggles the LED, then blocks | 500 ms (1 Hz blink) |
| `vTaskServoControl` | 2 (medium) | Moves the servo 1° per step, reverses at 0° and 180° | 15 ms per degree |
| `vTaskAudioML` | 1 (lowest) | Busy-waits 200 ms to mimic MFCC extraction and inference, prints a message, yields briefly | about 210 ms per cycle |

**How preemption plays out**
1. `vTaskAudioML` runs whenever nothing more urgent is ready, burning CPU for 200 ms.
2. When the 15 ms or 500 ms delay of a higher-priority task expires, the scheduler interrupts `vTaskAudioML` immediately.
3. The servo or LED task does its short job and goes back to sleep with `vTaskDelay`.
4. `vTaskAudioML` resumes where it left off.

`loop()` is intentionally empty and deletes itself with `vTaskDelete(NULL)`, so the Arduino loop task does not compete for CPU time.

---

## Build and flash

**Requirements**
- Arduino IDE with the ESP32 board package installed
- Library: **ESP32Servo** (Library Manager)

**Steps**
1. Open the sketch in the Arduino IDE.
2. Select your board (for example, *ESP32 Dev Module*).
3. Upload, then open the Serial Monitor at **115200 baud**.

---

## Expected behaviour

- The LED blinks at a steady 1 Hz.
- The servo sweeps smoothly back and forth, taking about 2.7 s for a full 0° to 180° pass.
- The Serial Monitor prints `Keyword spotting inference complete.` roughly every 210 ms.
- None of this should change while the simulated inference load is running.

---

## Design notes

- **Single-core pinning:** `xTaskCreatePinnedToCore(..., 1)` forces real contention. On a dual-core ESP32, leaving tasks unpinned would let the scheduler hide the problem by spreading them across cores.
- **Yielding in the heavy task:** the 10 ms `vTaskDelay` at the end of `vTaskAudioML` lets the FreeRTOS idle task run and feed the watchdog timer.
- **Stack sizes:** 1 KB (LED), 2 KB (servo), 4 KB (ML task, larger to leave room for real feature-extraction buffers later).
- **Tick rate:** `pdMS_TO_TICKS` converts milliseconds to ticks, so the code works regardless of the configured tick frequency, though timing resolution depends on it (1 ms on the default Arduino-ESP32 build).

---

## Limitations

- The ML workload is a **placeholder busy-wait**, not real inference. No microphone, MFCC code, or TensorFlow Lite model is used in this sketch.
- Timing has not been measured with instrumentation. Smoothness is judged by eye.

## Roadmap

- Replace the busy-wait with real I2S capture (INMP441), MFCC extraction, and a TensorFlow Lite Micro keyword-spotting model.
- Measure servo step jitter and LED toggle jitter with a logic analyzer or timestamped GPIO logging, and record the numbers here.
- Add a short video or GIF of the running hardware.

---

## Project structure

```
.
├── <sketch>.ino   # All three tasks plus setup()
└── README.md
```
