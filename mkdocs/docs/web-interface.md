# Web Interface

Flock You includes a Python-based web dashboard for real-time monitoring, detection history, and data export.

## Overview

| Feature | Description |
|---------|-------------|
| **Live Dashboard** | Real-time detection feed |
| **Serial Terminal** | Device output in browser |
| **Detection History** | Browse past detections |
| **Data Export** | CSV and KML formats |
| **Device Info** | Signal strength and details |

## Prerequisites

| Requirement | Version |
|-------------|---------|
| **Python** | 3.8+ |
| **Oui-Spy Device** | Connected via USB |

## Installation

### 1. Navigate to API Directory

```bash
cd flock-you/api
```

### 2. Create Virtual Environment

=== "macOS/Linux"

    ```bash
    python3 -m venv venv
    source venv/bin/activate
    ```

=== "Windows"

    ```bash
    python -m venv venv
    venv\Scripts\activate
    ```

### 3. Install Dependencies

```bash
pip install -r requirements.txt
```

## Running the Server

### Start the Web Server

```bash
python flockyou.py
```

### Access the Dashboard

Open your browser to: **http://localhost:5000**

## Dashboard Features

### Real-Time Detection Feed

The main dashboard shows live detections as they occur:

- **Device Type**: Flock Safety, Ring, Verkada, etc.
- **Signal Strength**: RSSI in dBm
- **MAC Address**: Device identifier
- **SSID**: WiFi network name (if applicable)
- **Timestamp**: When detected

### Serial Terminal

View raw device output in real-time:

```json
{"type":"Flock Safety","mac":"3C:71:BF:12:34:56","rssi":-62}
```

### Detection History

Browse and search past detections:

- Filter by device type
- Sort by time or signal strength
- View detection details

## Data Export

### Export Formats

| Format | Use Case |
|--------|----------|
| **CSV** | Spreadsheets, analysis |
| **KML** | Google Earth, mapping |

### Exporting Data

1. Click the **Export** button
2. Select format (CSV or KML)
3. Download the file

### CSV Format

```csv
timestamp,type,mac,ssid,rssi,latitude,longitude
2024-01-15T14:30:00Z,Flock Safety,3C:71:BF:12:34:56,FLOCK-S3-1234,-62,37.7749,-122.4194
```

### KML Format

Compatible with Google Earth for visualizing detection locations on a map.

## Configuration

### Server Settings

Edit `flockyou.py` to customize:

```python
# Server configuration
HOST = '0.0.0.0'  # Listen on all interfaces
PORT = 5000       # Default port
DEBUG = True      # Enable debug mode
```

### Serial Port

The server auto-detects the Oui-Spy device. If needed, specify manually:

```python
SERIAL_PORT = '/dev/ttyUSB0'  # Linux
# SERIAL_PORT = '/dev/tty.usbserial-xxx'  # macOS
# SERIAL_PORT = 'COM3'  # Windows
```

## API Endpoints

### REST API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Dashboard HTML |
| `/api/detections` | GET | Recent detections JSON |
| `/api/export/csv` | GET | Export as CSV |
| `/api/export/kml` | GET | Export as KML |

### WebSocket

Real-time updates via WebSocket at `/ws`:

```javascript
const ws = new WebSocket('ws://localhost:5000/ws');
ws.onmessage = (event) => {
  const detection = JSON.parse(event.data);
  console.log('New detection:', detection);
};
```

## Troubleshooting

### Server Won't Start

| Error | Solution |
|-------|----------|
| Port in use | Change PORT in config or kill existing process |
| Module not found | Ensure virtual environment is activated |
| Serial error | Check device is connected and not in use by another program |

### No Detections Showing

1. Verify device is flashing firmware correctly
2. Check serial monitor for output
3. Ensure USB cable is data-capable

### WebSocket Disconnects

- Check browser console for errors
- Verify firewall allows WebSocket connections
- Try refreshing the page

## Running in Production

For persistent deployment:

### Using systemd (Linux)

```ini
[Unit]
Description=Flock You Web Server
After=network.target

[Service]
User=pi
WorkingDirectory=/home/pi/flock-you/api
ExecStart=/home/pi/flock-you/api/venv/bin/python flockyou.py
Restart=always

[Install]
WantedBy=multi-user.target
```

### Using Screen

```bash
screen -S flockyou
python flockyou.py
# Press Ctrl+A, D to detach
```

## Next Steps

- Learn about [detection types](detection-types.md) supported
- Understand the [data export](data-export.md) formats
- Explore the [architecture](architecture.md) of the system
