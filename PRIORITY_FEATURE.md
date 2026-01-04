# Priority-Based Sensor Scheduling

## Overview
This feature implements intelligent sensor scheduling based on priority levels and timing constraints.

## Features Implemented

### 1. **Timing Tracking**
- Tracks the duration of each sensor read operation
- Calculates average read time across all operations
- Uses this data to estimate whether there's enough time in the update interval

### 2. **Priority Levels**
Datapoints can be assigned one of three priority levels:
- **Priority 1 (High)**: Always queried in every update cycle
- **Priority 2 (Medium)**: Queried when time permits **DEFAULT**
- **Priority 3 (Low)**: Queried only when sufficient time is available

### 3. **Adaptive Scheduling**
- Low priority sensors are queued first (so they can be skipped if needed)
- High priority sensors are always queued
- Medium/low priority sensors are skipped if estimated time exceeds the update interval
- Prevents queue overflow and ensures critical sensors are always updated

## Configuration

### Component Level
```yaml
vitoconnect:
  protocol: P300
  queue_size: 30          # Optional, default: 20
  update_interval: 60s    # How often to query sensors
```

### Sensor Level
```yaml
sensor:
  - platform: vitoconnect
    vitoconnect_id: optolink
    address: 0x0800
    length: 2
    priority: 1           # Optional, default: 2 (medium priority)
    # priority: 1 = high (always queried)
    # priority: 2 = medium (queried when time permits)
    # priority: 3 = low (queried only if sufficient time)
    name: "Outdoor Temperature"

binary_sensor:
  - platform: vitoconnect
    vitoconnect_id: optolink
    address: 0x2301
    priority: 2           # Optional, default: 2 (medium priority)
    name: "Burner Status"
```
