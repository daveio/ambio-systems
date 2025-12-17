# Pendant

## Notes

- TBD in the diagram: Bluetooth proxy for call transcription.
- Manual switch should be one of those really funky buttons with a display on them so that they can reflect the current state as it changes.
  - The [NKK `ISC15ANP4`][nkk-button] might work.

## Diagram

```mermaid
---
config:
  look: neo
  layout: elk
  theme: neo-dark
id: 824b7c2c-b7c8-4d31-b8a3-c5ca4e5234b2
---
flowchart TB
  subgraph PendantGroup["Wearable Pendant"]
    Pendant["M5 Capsule (ESP32-S3, Mic)"]
    PendantSD["microSD"]
  end
  subgraph DesktopGroup["Desktop Device"]
    Desktop["Hi-Fi Mics"]
    DesktopSD["microSD"]
  end
  subgraph Capture["Audio Capture Devices"]
    direction LR
      PendantGroup
      DesktopGroup
  end
  subgraph TVGuard["TV Guard"]
    Passthrough["Audio/HDMI/Analog Pass-through"]
    LightSensor["Light Sensor"]
  end
  subgraph MusicGuard["Music Guard"]
    MPassthrough["Audio/HDMI/Analog Pass-through"]
  end
  subgraph ManualGuard["Manual Switch"]
    Switch["Off/On Button"]
  end
  subgraph Control["Capture Control"]
    TVGuard
    MusicGuard
    ManualGuard
    ControlUnit["Control Unit"]
  end
  subgraph Aggregation["Aggregation"]
    Phone["Phone App"]
  end
  subgraph Processing["Processing"]
    Decision{"LAN broadcast active?"}
    LocalProc["Local Processing (Whisper + GPT-OSS)"]
    CloudProc["Cloud Processing"]
  end
  subgraph Storage["Storage"]
    LocalStorage["Local Storage"]
    CloudStorage["Cloud Storage"]
  end

  %% Connections
  Desktop == "Ultrasonic shutdown" ==> Pendant
  ControlUnit == "Ultrasonic shutdown" ==> Pendant
  Passthrough -- "Detects audio" --> ControlUnit
  LightSensor -- "Detects TV light" --> ControlUnit
  MPassthrough -- "Detects audio" --> ControlUnit
  Switch -- "Manually controlled" --> ControlUnit
  Pendant -- "Records → SD" --> PendantSD
  Desktop -- "Records → SD" --> DesktopSD
  Pendant -- Bluetooth --> Phone
  Pendant -. "Optional: WiFi" .-> Decision
  Desktop -- WiFi --> Decision
  Phone -- "Bluetooth Recordings" --> Decision
  Decision -- Yes --> LocalProc
  Decision -- No --> CloudProc
  LocalProc --> LocalStorage
  CloudProc --> CloudStorage
  LocalStorage -. Optional .-> CloudStorage
  LocalStorage --> Phone
  CloudStorage --> Phone
  ControlUnit == "Ultrasonic shutdown" ==> Desktop
```

[nkk-button]: https://www.nkkswitches.com/wp-content/themes/impress-blank/search/inc/part.php?part_no=ISC15ANP4
