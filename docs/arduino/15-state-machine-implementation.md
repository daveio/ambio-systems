# Step 5.3: State Machine Implementation

## Overview

Your audio recorder firmware, by this point, probably resembles a plate of spaghetti that someone has set fire to. There are flags for recording, flags for errors, flags for button presses, nested if-statements that reach geological depths, and at least one boolean variable that nobody quite remembers the purpose of but everyone is too afraid to remove.

State machines are the antidote. They force you to explicitly define what states your system can be in, what causes transitions between states, and what actions occur during those transitions. The result is code that's easier to understand, debug, and extend — qualities that future-you will appreciate when you return to this codebase in six months and wonder what on earth past-you was thinking.

## What You'll Learn

- State machine fundamentals and terminology
- Implementing a state machine in C++
- Designing states for audio recording
- Handling events and transitions
- Integrating with FreeRTOS tasks
- Adding new features without creating chaos

## Prerequisites

- Completed [Step 5.2: Error Handling & Robustness](14-error-handling-robustness.md)
- Working, if somewhat chaotic, audio recording system
- Mild despair at the state of your current codebase (optional but typical)

---

## Step 1: State Machine Fundamentals

### 1.1 What Is a State Machine?

A finite state machine (FSM) consists of:

- **States**: The distinct modes your system can be in (e.g., `IDLE`, `RECORDING`, `ERROR`)
- **Events**: Things that happen (e.g., button press, SD card failure, timer expiry)
- **Transitions**: Rules for moving between states in response to events
- **Actions**: Code that runs when entering/exiting states or during transitions

The key insight is that your system is always in exactly one state, and events cause deterministic transitions between states.

### 1.2 Why Bother?

Compare these approaches:

**Without state machine (the spaghetti method):**

```cpp
void loop() {
    if (buttonPressed && !recording && !error && sdReady) {
        startRecording();
        recording = true;
    } else if (buttonPressed && recording && !error) {
        stopRecording();
        recording = false;
    } else if (sdError && recording) {
        stopRecording();
        recording = false;
        error = true;
    } else if (buttonPressed && error) {
        attemptRecovery();
        // What state are we in now? Who knows!
    }
    // ... 47 more conditions ...
}
```

**With state machine:**

```cpp
void loop() {
    Event event = pollEvents();
    currentState = currentState->handleEvent(event);
}
```

The state machine version is boringly predictable. That's a feature.

### 1.3 State Diagram for Audio Recorder

```
                    ┌──────────────┐
                    │  POWER_ON    │
                    └──────┬───────┘
                           │ init complete
                           ▼
            ┌──────────────────────────────┐
   ┌────────│           IDLE               │◄──────────┐
   │        └──────────────┬───────────────┘           │
   │                       │ button press              │
   │                       ▼                           │
   │        ┌──────────────────────────────┐           │
   │        │         RECORDING            │──────────►│
   │        └──────────────┬───────────────┘           │
   │                       │ button press / duration   │
   │                       │ complete / error          │
   │                       ▼                           │
   │        ┌──────────────────────────────┐  recovery │
   │        │       FINALISING             │───────────┘
   │        └──────────────┬───────────────┘
   │                       │ SD failure
   │                       ▼
   │        ┌──────────────────────────────┐
   └───────►│          ERROR               │
            └──────────────────────────────┘
```

---

## Step 2: Basic State Machine Implementation

### 2.1 State Enumeration

Start by defining your states:

```cpp
enum class State {
    POWER_ON,       // Initial boot, hardware init
    IDLE,           // Ready, waiting for user input
    RECORDING,      // Actively recording audio
    FINALISING,     // Closing file, updating WAV header
    ERROR,          // Something went wrong
    SLEEPING        // Deep sleep mode
};

const char* stateToString(State state) {
    switch (state) {
        case State::POWER_ON:    return "POWER_ON";
        case State::IDLE:        return "IDLE";
        case State::RECORDING:   return "RECORDING";
        case State::FINALISING:  return "FINALISING";
        case State::ERROR:       return "ERROR";
        case State::SLEEPING:    return "SLEEPING";
        default:                 return "UNKNOWN";
    }
}
```

### 2.2 Event Enumeration

Define what can happen:

```cpp
enum class Event {
    NONE,               // No event
    INIT_COMPLETE,      // Hardware initialisation finished
    INIT_FAILED,        // Hardware initialisation failed
    BUTTON_PRESS,       // User pressed the button
    BUTTON_LONG_PRESS,  // User held button for 3+ seconds
    RECORDING_COMPLETE, // Reached max duration
    SD_ERROR,           // SD card failure
    SD_RECOVERED,       // SD card working again
    LOW_BATTERY,        // Battery critically low
    TIMER_TICK,         // Periodic timer (for timeouts, etc.)
    SLEEP_TIMEOUT       // Idle timeout - time to sleep
};

const char* eventToString(Event event) {
    switch (event) {
        case Event::NONE:               return "NONE";
        case Event::INIT_COMPLETE:      return "INIT_COMPLETE";
        case Event::INIT_FAILED:        return "INIT_FAILED";
        case Event::BUTTON_PRESS:       return "BUTTON_PRESS";
        case Event::BUTTON_LONG_PRESS:  return "BUTTON_LONG_PRESS";
        case Event::RECORDING_COMPLETE: return "RECORDING_COMPLETE";
        case Event::SD_ERROR:           return "SD_ERROR";
        case Event::SD_RECOVERED:       return "SD_RECOVERED";
        case Event::LOW_BATTERY:        return "LOW_BATTERY";
        case Event::TIMER_TICK:         return "TIMER_TICK";
        case Event::SLEEP_TIMEOUT:      return "SLEEP_TIMEOUT";
        default:                        return "UNKNOWN";
    }
}
```

### 2.3 Simple Transition Table Implementation

The most straightforward approach — a function that returns the new state:

```cpp
class AudioRecorderStateMachine {
private:
    State currentState = State::POWER_ON;

public:
    State getCurrentState() const { return currentState; }

    void handleEvent(Event event) {
        State previousState = currentState;

        // Skip NONE events
        if (event == Event::NONE) return;

        Serial.printf("[SM] Event: %s in state %s\n",
                     eventToString(event), stateToString(currentState));

        // Determine new state
        State newState = getNextState(currentState, event);

        // Handle transition if state changed
        if (newState != currentState) {
            onExitState(currentState);
            currentState = newState;
            onEnterState(currentState);

            Serial.printf("[SM] Transition: %s -> %s\n",
                         stateToString(previousState),
                         stateToString(currentState));
        }
    }

private:
    State getNextState(State state, Event event) {
        switch (state) {
            case State::POWER_ON:
                if (event == Event::INIT_COMPLETE) return State::IDLE;
                if (event == Event::INIT_FAILED)   return State::ERROR;
                break;

            case State::IDLE:
                if (event == Event::BUTTON_PRESS)      return State::RECORDING;
                if (event == Event::BUTTON_LONG_PRESS) return State::SLEEPING;
                if (event == Event::SLEEP_TIMEOUT)     return State::SLEEPING;
                if (event == Event::LOW_BATTERY)       return State::ERROR;
                break;

            case State::RECORDING:
                if (event == Event::BUTTON_PRESS)       return State::FINALISING;
                if (event == Event::RECORDING_COMPLETE) return State::FINALISING;
                if (event == Event::SD_ERROR)           return State::ERROR;
                if (event == Event::LOW_BATTERY)        return State::FINALISING;
                break;

            case State::FINALISING:
                // Finalising is handled in onEnterState, transitions via timer
                if (event == Event::TIMER_TICK) return State::IDLE;
                if (event == Event::SD_ERROR)   return State::ERROR;
                break;

            case State::ERROR:
                if (event == Event::BUTTON_PRESS) return State::POWER_ON; // Retry
                if (event == Event::SD_RECOVERED) return State::IDLE;
                break;

            case State::SLEEPING:
                // Wake handled by hardware, returns to POWER_ON
                break;
        }

        // No transition - stay in current state
        return state;
    }

    void onEnterState(State state) {
        Serial.printf("[SM] Entering state: %s\n", stateToString(state));

        switch (state) {
            case State::POWER_ON:
                setLED(LED_YELLOW);
                initializeHardware();
                break;

            case State::IDLE:
                setLED(LED_GREEN);
                startIdleTimeout();
                break;

            case State::RECORDING:
                setLED(LED_RED);
                startRecording();
                break;

            case State::FINALISING:
                setLED(LED_YELLOW);
                finaliseRecording();
                break;

            case State::ERROR:
                setLED(LED_RED_BLINK);
                logError();
                break;

            case State::SLEEPING:
                setLED(LED_OFF);
                enterDeepSleep();
                break;
        }
    }

    void onExitState(State state) {
        Serial.printf("[SM] Exiting state: %s\n", stateToString(state));

        switch (state) {
            case State::IDLE:
                cancelIdleTimeout();
                break;

            case State::RECORDING:
                // Recording cleanup handled in FINALISING
                break;

            default:
                break;
        }
    }
};
```

---

## Step 3: Event Polling

The state machine needs events. Here's how to generate them:

### 3.1 Button Handling

```cpp
class ButtonHandler {
private:
    const int buttonPin;
    const uint32_t DEBOUNCE_MS = 50;
    const uint32_t LONG_PRESS_MS = 3000;

    bool lastReading = HIGH;
    bool buttonState = HIGH;
    uint32_t lastDebounceTime = 0;
    uint32_t pressStartTime = 0;
    bool longPressTriggered = false;

public:
    ButtonHandler(int pin) : buttonPin(pin) {
        pinMode(buttonPin, INPUT_PULLUP);
    }

    Event poll() {
        bool reading = digitalRead(buttonPin);
        Event event = Event::NONE;

        // Debounce
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }
        lastReading = reading;

        if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
            // Reading is stable
            if (reading != buttonState) {
                buttonState = reading;

                if (buttonState == LOW) {
                    // Button pressed
                    pressStartTime = millis();
                    longPressTriggered = false;
                } else {
                    // Button released
                    if (!longPressTriggered) {
                        // Short press
                        event = Event::BUTTON_PRESS;
                    }
                }
            }

            // Check for long press while held
            if (buttonState == LOW && !longPressTriggered) {
                if ((millis() - pressStartTime) > LONG_PRESS_MS) {
                    longPressTriggered = true;
                    event = Event::BUTTON_LONG_PRESS;
                }
            }
        }

        return event;
    }
};
```

### 3.2 Timer Events

```cpp
class TimerManager {
private:
    uint32_t idleTimeoutStart = 0;
    uint32_t idleTimeoutDuration = 0;
    bool idleTimeoutActive = false;

    uint32_t recordingStart = 0;
    uint32_t maxRecordingDuration = 0;
    bool recordingTimerActive = false;

public:
    void startIdleTimeout(uint32_t durationMs) {
        idleTimeoutStart = millis();
        idleTimeoutDuration = durationMs;
        idleTimeoutActive = true;
    }

    void cancelIdleTimeout() {
        idleTimeoutActive = false;
    }

    void startRecordingTimer(uint32_t maxDurationMs) {
        recordingStart = millis();
        maxRecordingDuration = maxDurationMs;
        recordingTimerActive = true;
    }

    void cancelRecordingTimer() {
        recordingTimerActive = false;
    }

    Event poll() {
        uint32_t now = millis();

        if (idleTimeoutActive && (now - idleTimeoutStart >= idleTimeoutDuration)) {
            idleTimeoutActive = false;
            return Event::SLEEP_TIMEOUT;
        }

        if (recordingTimerActive && (now - recordingStart >= maxRecordingDuration)) {
            recordingTimerActive = false;
            return Event::RECORDING_COMPLETE;
        }

        return Event::NONE;
    }
};
```

### 3.3 Event Queue

Collect events from multiple sources:

```cpp
#include <queue>

class EventQueue {
private:
    std::queue<Event> events;
    ButtonHandler& button;
    TimerManager& timers;

public:
    EventQueue(ButtonHandler& btn, TimerManager& tmr)
        : button(btn), timers(tmr) {}

    // Push an event directly (e.g., from ISR or another task)
    void push(Event event) {
        if (event != Event::NONE) {
            events.push(event);
        }
    }

    // Poll for next event
    Event poll() {
        // First return any queued events
        if (!events.empty()) {
            Event event = events.front();
            events.pop();
            return event;
        }

        // Poll button
        Event buttonEvent = button.poll();
        if (buttonEvent != Event::NONE) {
            return buttonEvent;
        }

        // Poll timers
        Event timerEvent = timers.poll();
        if (timerEvent != Event::NONE) {
            return timerEvent;
        }

        return Event::NONE;
    }
};
```

---

## Step 4: Complete Integration

### 4.1 Main Application Structure

```cpp
#include <Arduino.h>

// Forward declarations
class AudioRecorder;

// Global instances
ButtonHandler button(BUTTON_PIN);
TimerManager timers;
EventQueue eventQueue(button, timers);
AudioRecorderStateMachine stateMachine;
AudioRecorder* recorder = nullptr;

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("=== Audio Recorder Starting ===");

    // State machine starts in POWER_ON, which triggers hardware init
    // via onEnterState. When init completes, we fire INIT_COMPLETE.
}

void loop() {
    // Poll for events
    Event event = eventQueue.poll();

    // Let state machine handle it
    stateMachine.handleEvent(event);

    // State-specific periodic work
    doStateWork(stateMachine.getCurrentState());

    // Feed watchdog
    esp_task_wdt_reset();

    // Small delay to prevent busy-looping
    delay(10);
}

void doStateWork(State state) {
    switch (state) {
        case State::RECORDING:
            // Process audio buffers
            if (recorder) {
                bool ok = recorder->processBuffers();
                if (!ok) {
                    eventQueue.push(Event::SD_ERROR);
                }
            }
            break;

        default:
            break;
    }
}
```

### 4.2 State Actions Implementation

```cpp
// Called from state machine's onEnterState
void initializeHardware() {
    Serial.println("Initialising hardware...");

    // SD card
    SDStatus sdStatus = initializeSDCard();
    if (sdStatus != SD_OK) {
        Serial.printf("SD init failed: %d\n", sdStatus);
        eventQueue.push(Event::INIT_FAILED);
        return;
    }

    // I2S
    if (!initializeI2S()) {
        Serial.println("I2S init failed");
        eventQueue.push(Event::INIT_FAILED);
        return;
    }

    // Create recorder instance
    recorder = new AudioRecorder();

    Serial.println("Hardware initialised successfully");
    eventQueue.push(Event::INIT_COMPLETE);
}

void startRecording() {
    if (!recorder) {
        Serial.println("No recorder instance!");
        eventQueue.push(Event::INIT_FAILED);
        return;
    }

    String filename = generateFilename();
    if (!recorder->begin(filename.c_str())) {
        eventQueue.push(Event::SD_ERROR);
        return;
    }

    // Start recording timer (e.g., 5 minute max)
    timers.startRecordingTimer(5 * 60 * 1000);

    Serial.printf("Recording started: %s\n", filename.c_str());
}

void finaliseRecording() {
    if (recorder) {
        recorder->stop();
        Serial.println("Recording finalised");
    }

    timers.cancelRecordingTimer();

    // Short delay then return to idle
    delay(500);
    eventQueue.push(Event::TIMER_TICK);
}

void startIdleTimeout() {
    // Sleep after 30 seconds of inactivity
    timers.startIdleTimeout(30 * 1000);
}

void cancelIdleTimeout() {
    timers.cancelIdleTimeout();
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep...");

    // Clean up
    if (recorder) {
        delete recorder;
        recorder = nullptr;
    }
    deinitializeI2S();
    deinitializeSD();

    // Configure wake sources
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0);

    Serial.flush();
    esp_deep_sleep_start();
}

void logError() {
    // Log to RTC memory and SD (if available)
    logErrorToRTC("Error state entered");
}
```

---

## Step 5: Extending the State Machine

The beauty of state machines is how easy they are to extend.

### 5.1 Adding a New State: PAUSED

Suppose you want to add a pause feature:

```cpp
// 1. Add the state
enum class State {
    // ... existing states ...
    PAUSED,         // Recording paused
};

// 2. Add relevant events
enum class Event {
    // ... existing events ...
    DOUBLE_PRESS,   // Double button press to pause/resume
};

// 3. Add transitions in getNextState()
case State::RECORDING:
    if (event == Event::DOUBLE_PRESS) return State::PAUSED;
    // ... existing transitions ...
    break;

case State::PAUSED:
    if (event == Event::DOUBLE_PRESS) return State::RECORDING;
    if (event == Event::BUTTON_PRESS) return State::FINALISING;
    if (event == Event::SLEEP_TIMEOUT) return State::FINALISING;
    break;

// 4. Add enter/exit actions
void onEnterState(State state) {
    // ...
    case State::PAUSED:
        setLED(LED_YELLOW_BLINK);
        pauseRecording();
        startIdleTimeout();  // Auto-finalise if paused too long
        break;
}

void onExitState(State state) {
    // ...
    case State::PAUSED:
        cancelIdleTimeout();
        break;
}
```

### 5.2 Adding Sub-States (Hierarchical State Machine)

For complex states, use sub-states:

```cpp
enum class RecordingSubState {
    CAPTURING,      // Normal recording
    BUFFERING,      // SD card busy, buffering audio
    FLUSHING        // Periodic flush to SD
};

class RecordingState {
private:
    RecordingSubState subState = RecordingSubState::CAPTURING;

public:
    void enter() {
        subState = RecordingSubState::CAPTURING;
        // ... init recording ...
    }

    Event process() {
        switch (subState) {
            case RecordingSubState::CAPTURING:
                if (bufferNearlyFull()) {
                    subState = RecordingSubState::BUFFERING;
                }
                captureAudio();
                break;

            case RecordingSubState::BUFFERING:
                if (sdCardReady()) {
                    subState = RecordingSubState::FLUSHING;
                }
                captureToOverflowBuffer();
                break;

            case RecordingSubState::FLUSHING:
                flushBuffer();
                if (bufferEmpty()) {
                    subState = RecordingSubState::CAPTURING;
                }
                break;
        }

        return Event::NONE;  // Or return error event if needed
    }
};
```

---

## Step 6: FreeRTOS Integration

For more complex systems, run the state machine in its own task:

```cpp
// Event queue with thread-safe access
QueueHandle_t eventQueueHandle;

void stateMachineTask(void* parameter) {
    AudioRecorderStateMachine* sm = (AudioRecorderStateMachine*)parameter;
    Event event;

    esp_task_wdt_add(NULL);

    while (true) {
        // Wait for event with timeout
        if (xQueueReceive(eventQueueHandle, &event, pdMS_TO_TICKS(100))) {
            sm->handleEvent(event);
        }

        // Periodic work
        doStateWork(sm->getCurrentState());

        esp_task_wdt_reset();
    }
}

void buttonISR() {
    // Called from interrupt - must be quick
    Event event = Event::BUTTON_PRESS;  // Simplified - real impl would debounce
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(eventQueueHandle, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void setup() {
    // Create event queue
    eventQueueHandle = xQueueCreate(10, sizeof(Event));

    // Create state machine task
    static AudioRecorderStateMachine sm;
    xTaskCreatePinnedToCore(
        stateMachineTask,
        "StateMachine",
        4096,
        &sm,
        2,      // Priority
        NULL,
        1       // Core 1
    );

    // Attach button interrupt
    attachInterrupt(BUTTON_PIN, buttonISR, FALLING);
}
```

---

## Verification Checklist

Before declaring victory, verify:

- [ ] All states are reachable (trace through the diagram)
- [ ] All states have at least one exit (no dead ends)
- [ ] Button press behaviour is correct in each state
- [ ] Errors transition to ERROR state appropriately
- [ ] Recovery from ERROR state works
- [ ] Sleep entry and wake work correctly
- [ ] No memory leaks when transitioning states
- [ ] LED indicates current state correctly
- [ ] Serial output shows all transitions (for debugging)

---

## Common Issues

### State Machine Seems Stuck

**Symptom:** No transitions occurring

**Causes:**

- Event polling not running
- Events being generated but not processed
- Transition rules don't cover the case

**Solutions:**

```cpp
// Add verbose logging
void handleEvent(Event event) {
    Serial.printf("[SM] Received event %s in state %s\n",
                 eventToString(event), stateToString(currentState));
    // ...
}
```

### Unexpected State Transitions

**Symptom:** System jumps to wrong state

**Causes:**

- Event generated at wrong time
- Missing condition in transition logic
- Multiple events queued causing cascade

**Solutions:**

```cpp
// Log all transitions
State newState = getNextState(currentState, event);
if (newState != currentState) {
    Serial.printf("[SM] TRANSITION: %s + %s -> %s\n",
                 stateToString(currentState),
                 eventToString(event),
                 stateToString(newState));
}
```

### Actions Running at Wrong Time

**Symptom:** Recording starts when it shouldn't, etc.

**Causes:**

- Action in wrong state handler
- onEnter/onExit called incorrectly

**Solutions:**

- Review onEnterState/onExitState carefully
- Ensure actions only run once per transition

---

## What's Next

Congratulations! You've completed the entire learning pathway. You now have:

1. A PlatformIO development environment
2. Working SD card and I2S audio capture
3. WAV file recording with buffered writing
4. Power management for battery life
5. Robust error handling
6. A clean state machine architecture

From here, you might want to:

- Add Bluetooth audio streaming to a phone app
- Implement voice activity detection to record only when speaking
- Add the ultrasonic control signals from your original design
- Build a proper enclosure for your pendant

The M5 Capsule is now less of a mysterious black box and more of a tool you understand. May your recordings be glitch-free and your batteries long-lasting.

---

## Quick Reference

### State Machine Pattern

```cpp
enum class State { A, B, C };
enum class Event { X, Y, Z };

State getNextState(State current, Event event) {
    switch (current) {
        case State::A:
            if (event == Event::X) return State::B;
            break;
        // ...
    }
    return current;  // No transition
}
```

### Event Queue (FreeRTOS)

```cpp
QueueHandle_t queue = xQueueCreate(10, sizeof(Event));
xQueueSend(queue, &event, 0);
xQueueReceive(queue, &event, pdMS_TO_TICKS(100));
```

### State Diagram Symbols

```
┌─────────┐
│  State  │    Rectangle = State
└─────────┘

    ────►      Arrow = Transition

event / action   Label = Trigger / Action
```

---

## Complete Series Index

1. [PlatformIO Environment Setup](01-platformio-environment-setup.md)
2. [First ESP32-S3 Project](02-first-esp32-project.md)
3. [Serial Debugging](03-serial-debugging.md)
4. [SPI Basics](04-spi-basics.md)
5. [SD Card Library](05-sd-card-library.md)
6. [File Management Patterns](06-file-management-patterns.md)
7. [I2S Protocol Understanding](07-i2s-protocol-understanding.md)
8. [ESP-IDF I2S Driver](08-esp-idf-i2s-driver.md)
9. [Basic Audio Capture Sketch](09-basic-audio-capture-sketch.md)
10. [WAV File Format](10-wav-file-format.md)
11. [Buffered Writing](11-buffered-writing.md)
12. [Putting It All Together](12-putting-it-all-together.md)
13. [Power Management](13-power-management.md)
14. [Error Handling & Robustness](14-error-handling-robustness.md)
15. [State Machine Implementation](15-state-machine-implementation.md) ← You are here
