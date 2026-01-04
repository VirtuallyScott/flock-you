# Data Export

Flock You supports exporting detection data via the web interface for analysis, mapping, or sharing.

## Export Formats

| Format | Extension | Best For |
|--------|-----------|----------|
| **CSV** | `.csv` | Spreadsheets, data analysis |
| **KML** | `.kml` | Google Earth, mapping |

## How to Export

### Via Web Interface

1. Open the dashboard at `http://localhost:5000`
2. Click the **Export** button
3. Select format (CSV or KML)
4. Download the file

### Via API

```bash
# Export as CSV
curl http://localhost:5000/api/export/csv -o detections.csv

# Export as KML
curl http://localhost:5000/api/export/kml -o detections.kml
```

## CSV Format

Comma-separated values compatible with Excel, Google Sheets, and data analysis tools.

### Columns

| Column | Type | Description |
|--------|------|-------------|
| `timestamp` | ISO 8601 | Detection time |
| `type` | String | Device type (e.g., "Flock Safety") |
| `mac` | String | Device MAC address |
| `ssid` | String | WiFi network name |
| `rssi` | Integer | Signal strength (dBm) |
| `method` | String | Detection method |
| `latitude` | Float | GPS latitude (if available) |
| `longitude` | Float | GPS longitude (if available) |

### Example

```csv
timestamp,type,mac,ssid,rssi,method,latitude,longitude
2024-01-15T14:30:00Z,Flock Safety,3C:71:BF:12:34:56,FLOCK-S3-1234,-62,wifi_ssid,37.7749,-122.4194
2024-01-15T14:35:00Z,Raven,AA:BB:CC:DD:EE:FF,,-65,ble_uuid,37.7751,-122.4189
```

## KML Format

Keyhole Markup Language for geographic visualization.

### Structure

```xml
<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <name>Flock You Detections</name>
    
    <Placemark>
      <name>Flock Safety</name>
      <description>
        MAC: 3C:71:BF:12:34:56
        SSID: FLOCK-S3-1234
        RSSI: -62 dBm
        Method: wifi_ssid
      </description>
      <TimeStamp>
        <when>2024-01-15T14:30:00Z</when>
      </TimeStamp>
      <Point>
        <coordinates>-122.4194,37.7749,0</coordinates>
      </Point>
    </Placemark>
    
  </Document>
</kml>
```

### Compatible Applications

KML files can be imported into:

- Google Earth
- Google Maps (My Maps)
- QGIS
- ArcGIS
- GPS Visualizer

## Data Analysis

### Opening in Excel

1. Export as CSV
2. Open Excel and select **File > Open**
3. Choose the CSV file
4. Data will populate in columns

### Opening in Python

```python
import pandas as pd

# Load CSV
df = pd.read_csv('detections.csv')

# Filter by type
flock_detections = df[df['type'] == 'Flock Safety']

# Count by type
print(df['type'].value_counts())
```

### Visualizing on Map

```python
import folium
import pandas as pd

df = pd.read_csv('detections.csv')

# Create map centered on detections
m = folium.Map(
    location=[df['latitude'].mean(), df['longitude'].mean()],
    zoom_start=12
)

# Add markers
for _, row in df.iterrows():
    if pd.notna(row['latitude']):
        folium.Marker(
            [row['latitude'], row['longitude']],
            popup=f"{row['type']}<br>RSSI: {row['rssi']} dBm",
            icon=folium.Icon(color='red' if 'Flock' in row['type'] else 'blue')
        ).add_to(m)

m.save('detections_map.html')
```

## Adding GPS Data

The web interface can capture GPS coordinates if running on a device with location services:

1. Enable location in browser when prompted
2. Detections will include lat/long coordinates
3. Export will include geographic data

For mobile use, consider running the web server on a laptop while driving.

## Privacy Notice

!!! warning "Location Data"
    Exported files may contain GPS coordinates revealing where you detected surveillance devices. Consider privacy implications before sharing.

**Recommendations:**

- Only share with trusted parties
- Consider removing or obfuscating exact locations
- Don't post raw exports publicly

## Next Steps

- [Set up the web interface](web-interface.md)
- [Learn about detection types](detection-types.md)
- [View project architecture](architecture.md)
