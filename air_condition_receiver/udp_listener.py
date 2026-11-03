#!/usr/bin/env python3
"""
UDP Listener for Air Quality Receiver
=====================================
Dashboard mode - displays real-time data from up to 4 devices
with colored PM values and statistics.

Usage:
    python3 udp_listener.py

Press Ctrl+C to exit.
"""

import socket
import json
import time
import sys
import os
from datetime import datetime

# ============================================
# CONSTANTS
# ============================================
UDP_PORT = 5000
MAX_DEVICES = 4

# ANSI color codes
class Colors:
    RESET = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    
    # Status colors
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GRAY = '\033[90m'
    
    # UI colors
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BLUE = '\033[94m'

# PM thresholds (value, color)
PM25_THRESHOLDS = [(35, Colors.GREEN), (75, Colors.YELLOW), (999, Colors.RED)]
PM10_THRESHOLDS = [(50, Colors.GREEN), (100, Colors.YELLOW), (999, Colors.RED)]
PM1_THRESHOLDS = [(25, Colors.GREEN), (50, Colors.YELLOW), (999, Colors.RED)]

# ============================================
# HELPER FUNCTIONS
# ============================================

def get_pm_color(value, thresholds):
    """Returns ANSI color code based on PM value and thresholds."""
    for threshold, color in thresholds:
        if value <= threshold:
            return color
    return Colors.RED

def format_duration(seconds):
    """Formats seconds as HH:MM:SS."""
    hours = int(seconds // 3600)
    minutes = int((seconds % 3600) // 60)
    secs = int(seconds % 60)
    return f"{hours:02d}:{minutes:02d}:{secs:02d}"

def format_last_seen(seconds):
    """Formats last seen time."""
    if seconds < 0:
        return "--"
    elif seconds < 60:
        return f"{int(seconds)}s"
    elif seconds < 3600:
        return f"{int(seconds // 60)}m"
    else:
        return f"{int(seconds // 3600)}h"

def clear_screen():
    """Clears terminal screen."""
    os.system('cls' if os.name == 'nt' else 'clear')

# ============================================
# DASHBOARD DRAWING
# ============================================

def draw_dashboard(devices, stats, last_source):
    """Draws the dashboard with device data."""
    clear_screen()
    
    c = Colors
    
    # All lines are exactly 80 characters wide
    print(f"{c.CYAN}{c.BOLD}")
    print("╔══════════════════════════════════════════════════════════════════════════════╗")
    print(f"║  AIR QUALITY UDP LISTENER                            Packets: {stats['packets']:>6}         ║")
    print(f"║  Listening on port {UDP_PORT:<5}                             Uptime: {format_duration(stats['uptime'])}        ║")
    print("╠════╤══════════════╤═══════╤══════╤═══════╤═══════╤═══════╤═══════╤═══════════╣")
    print(f"║{c.WHITE} #  │ Name         │ Temp  │ Hum  │ Press │ PM1.0 │ PM2.5 │ PM10  │ Last      {c.CYAN}║")
    print("╠════╪══════════════╪═══════╪══════╪═══════╪═══════╪═══════╪═══════╪═══════════╣")
    print(f"{c.RESET}", end="")
    
    # Device rows  
    for i in range(MAX_DEVICES):
        if i < len(devices) and devices[i] is not None:
            dev = devices[i]
            
            # Get colors for PM values
            pm1_color = get_pm_color(dev['pm1_0'], PM1_THRESHOLDS)
            pm25_color = get_pm_color(dev['pm2_5'], PM25_THRESHOLDS)
            pm10_color = get_pm_color(dev['pm10'], PM10_THRESHOLDS)
            
            # Status indicator (use ASCII for consistent width)
            if dev['active']:
                status = c.GREEN + "*" + c.RESET
            else:
                status = c.GRAY + "." + c.RESET
            
            # Format values (fixed widths)
            name = dev['name'][:12].ljust(12)
            temp = f"{dev['temperature']:.1f}".rjust(5)
            hum = f"{dev['humidity']:.0f}%".rjust(4)
            press = f"{dev['pressure']}".rjust(5)
            pm1_val = f"{dev['pm1_0']:>5}"
            pm25_val = f"{dev['pm2_5']:>5}"
            pm10_val = f"{dev['pm10']:>5}"
            last = format_last_seen(dev['last_seen_sec']).rjust(9)
            
            # Row with pressure
            print(f"{c.CYAN}║{c.RESET} {status}{i+1} │ {name} │ {temp} │ {hum} │ {press} │ {pm1_color}{pm1_val}{c.RESET} │ {pm25_color}{pm25_val}{c.RESET} │ {pm10_color}{pm10_val}{c.RESET} │ {last} {c.CYAN}║{c.RESET}")
        else:
            # Empty row with pressure placeholder
            print(f"{c.CYAN}║{c.GRAY} .{i+1} │ --           │    -- │  --  │    -- │    -- │    -- │    -- │        -- {c.CYAN}║{c.RESET}")
    
    # Footer
    print(f"{c.CYAN}{c.BOLD}╚══════════════════════════════════════════════════════════════════════════════╝{c.RESET}")
    
    # Last update info
    if last_source:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"{c.DIM}Last update: {timestamp} from {last_source}{c.RESET}")
    else:
        print(f"{c.DIM}Waiting for data...{c.RESET}")
    
    print(f"\n{c.DIM}Press Ctrl+C to exit{c.RESET}")

# ============================================
# MAIN
# ============================================

def main():
    """Main function - UDP listener loop."""
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(1.0)  # 1 second timeout for Ctrl+C handling
    
    try:
        sock.bind(('0.0.0.0', UDP_PORT))
    except OSError as e:
        print(f"{Colors.RED}Error: Cannot bind to port {UDP_PORT}: {e}{Colors.RESET}")
        print(f"Make sure no other process is using this port.")
        sys.exit(1)
    
    # State
    devices = [None] * MAX_DEVICES
    stats = {
        'packets': 0,
        'uptime': 0,
        'start_time': time.time()
    }
    last_source = None
    
    # Initial draw
    draw_dashboard(devices, stats, last_source)
    
    print(f"\n{Colors.GREEN}Listening on UDP port {UDP_PORT}...{Colors.RESET}")
    
    try:
        while True:
            try:
                data, addr = sock.recvfrom(4096)
                
                # Parse JSON
                try:
                    payload = json.loads(data.decode('utf-8'))
                except json.JSONDecodeError:
                    continue
                
                # Update stats
                stats['packets'] += 1
                stats['uptime'] = time.time() - stats['start_time']
                last_source = addr[0]
                
                # Extract devices from payload
                if 'devices' in payload:
                    for i, dev_data in enumerate(payload['devices'][:MAX_DEVICES]):
                        devices[i] = {
                            'name': dev_data.get('name', f"Device {i+1}"),
                            'mac': dev_data.get('mac', ''),
                            'active': dev_data.get('active', False),
                            'temperature': dev_data.get('temperature', 0),
                            'humidity': dev_data.get('humidity', 0),
                            'pressure': dev_data.get('pressure', 0),
                            'pm1_0': dev_data.get('pm1_0', 0),
                            'pm2_5': dev_data.get('pm2_5', 0),
                            'pm10': dev_data.get('pm10', 0),
                            'lora_rssi': dev_data.get('lora_rssi', 0),
                            'lora_snr': dev_data.get('lora_snr', 0),
                            'last_seen_sec': dev_data.get('last_seen_sec', -1)
                        }
                
                # Redraw dashboard
                draw_dashboard(devices, stats, last_source)
                
            except socket.timeout:
                # Update uptime even without data
                stats['uptime'] = time.time() - stats['start_time']
                draw_dashboard(devices, stats, last_source)
                continue
                
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Shutting down...{Colors.RESET}")
    finally:
        sock.close()
    
    print(f"{Colors.GREEN}Received {stats['packets']} packets in {format_duration(stats['uptime'])}{Colors.RESET}")

if __name__ == "__main__":
    main()
