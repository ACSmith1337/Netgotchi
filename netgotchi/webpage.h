// ============================================================================
// NETGOTCHI WEB INTERFACE - Matrix Cyberpunk Dashboard
// ============================================================================
// Full-featured dashboard with status monitoring, LED control, host scanning
// Mobile-friendly responsive design

#ifndef WEBPAGE_H
#define WEBPAGE_H

static const char PROGMEM pagehtml[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Netgotchi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: linear-gradient(135deg, #0a0e27 0%, #1a1f3a 100%);
            color: #00ff41;
            font-family: 'Courier New', monospace;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        header { text-align: center; margin-bottom: 25px; }
        h1 {
            font-size: 2.5em;
            color: #00ff41;
            text-shadow: 0 0 10px #00ff41, 0 0 20px #00ff41;
            animation: glow 2s ease-in-out infinite alternate;
        }
        @keyframes glow {
            from { text-shadow: 0 0 10px #00ff41, 0 0 20px #00ff41; }
            to { text-shadow: 0 0 20px #00ff41, 0 0 40px #00ff41, 0 0 60px #00ff41; }
        }
        .subtitle { color: #0f0; font-size: 0.9em; opacity: 0.7; margin-top: 5px; }
        .card {
            background: rgba(0, 20, 40, 0.8);
            border: 1px solid #00ff41;
            border-radius: 8px;
            padding: 18px;
            margin-bottom: 15px;
            box-shadow: 0 0 15px rgba(0, 255, 65, 0.2);
        }
        .card h2 {
            color: #00ff41;
            font-size: 1.1em;
            border-bottom: 1px solid rgba(0,255,65,0.3);
            padding-bottom: 8px;
            margin-bottom: 12px;
            text-transform: uppercase;
            letter-spacing: 2px;
        }
        canvas {
            image-rendering: pixelated;
            border: 1px solid #00ff41;
            background: #000;
            display: block;
            margin: 10px auto;
            transform: scale(2);
            transform-origin: center;
        }
        .canvas-wrap {
            display: flex;
            justify-content: center;
            padding: 20px;
            background: rgba(0,0,0,0.5);
            border-radius: 5px;
        }
        button {
            background: linear-gradient(135deg, #003300, #006600);
            color: #00ff41;
            border: 1px solid #00ff41;
            padding: 10px 18px;
            margin: 4px;
            border-radius: 4px;
            cursor: pointer;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 1px;
            transition: all 0.2s;
        }
        button:hover {
            background: linear-gradient(135deg, #006600, #00aa00);
            box-shadow: 0 0 15px rgba(0, 255, 65, 0.5);
            transform: translateY(-1px);
        }
        .btn-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
            gap: 6px;
        }
        .status-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 12px;
            background: rgba(0, 50, 0, 0.2);
            border: 1px solid rgba(0,255,65,0.2);
            border-radius: 4px;
            margin: 4px 0;
        }
        .status-label { font-size: 13px; color: #00cc33; }
        .status-value { font-size: 13px; font-weight: bold; }
        .badge {
            display: inline-block;
            padding: 3px 10px;
            border-radius: 12px;
            font-size: 11px;
            font-weight: bold;
            text-transform: uppercase;
        }
        .badge-green { background: rgba(0,255,0,0.2); color: #0f0; border: 1px solid #0f0; }
        .badge-red { background: rgba(255,0,0,0.2); color: #f00; border: 1px solid #f00; }
        .badge-blue { background: rgba(0,100,255,0.2); color: #66f; border: 1px solid #66f; }
        .badge-orange { background: rgba(255,165,0,0.2); color: #fa0; border: 1px solid #fa0; }
        .badge-purple { background: rgba(128,0,255,0.2); color: #c0f; border: 1px solid #c0f; }
        #headless {
            background: #000;
            color: #0f0;
            padding: 12px;
            border-radius: 4px;
            font-size: 12px;
            border: 1px solid rgba(0,255,65,0.3);
            line-height: 1.5;
            word-wrap: break-word;
        }
        #hosts {
            background: rgba(0,0,0,0.5);
            padding: 10px;
            border-radius: 4px;
            border: 1px solid rgba(0,255,65,0.3);
            color: #0f0;
            font-size: 12px;
            max-height: 300px;
            overflow-y: auto;
        }
        #hosts::-webkit-scrollbar { width: 8px; }
        #hosts::-webkit-scrollbar-track { background: #111; }
        #hosts::-webkit-scrollbar-thumb { background: #00ff41; border-radius: 4px; }
        .grid-2 {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 15px;
        }
        .led-indicator {
            width: 30px;
            height: 30px;
            border-radius: 50%;
            display: inline-block;
            vertical-align: middle;
            margin-right: 10px;
            border: 2px solid rgba(255,255,255,0.3);
            transition: all 0.3s;
        }
        .led-controls { margin-top: 10px; }
        .led-btn {
            width: 35px;
            height: 35px;
            border-radius: 50%;
            padding: 0;
            border: 2px solid rgba(255,255,255,0.3);
            cursor: pointer;
            transition: all 0.2s;
        }
        .led-btn:hover { transform: scale(1.2); }
        .led-btn-green { background: #0f0; }
        .led-btn-red { background: #f00; }
        .led-btn-blue { background: #00f; }
        .led-btn-off { background: #333; }
        .led-btn-auto { background: linear-gradient(90deg,#f00,#0f0,#00f,#f0f,#0ff); }
        footer {
            text-align: center;
            margin-top: 20px;
            padding: 15px;
            border-top: 1px solid rgba(0,255,65,0.2);
            opacity: 0.6;
            font-size: 11px;
        }
        @media (max-width: 600px) {
            h1 { font-size: 1.8em; }
            .btn-grid { grid-template-columns: repeat(2, 1fr); }
            canvas { transform: scale(1.5); }
        }
    </style>
</head>
<body>
<div class="container">
    <header>
        <h1>NETGOTCHI</h1>
        <div class="subtitle">NETWORK SECURITY MATRIX v<span id="version">1.66</span></div>
    </header>

    <div class="grid-2">
        <!-- DISPLAY -->
        <div class="card">
            <h2>[ DISPLAY ]</h2>
            <div class="canvas-wrap">
                <canvas id="canvas" width="128" height="64"></canvas>
            </div>
        </div>

        <!-- CONTROLS -->
        <div class="card">
            <h2>[ CONTROLS ]</h2>
            <div class="btn-grid">
                <button onclick="sendCommand('left')">LEFT</button>
                <button onclick="sendCommand('right')">RIGHT</button>
                <button onclick="sendCommand('A')">A</button>
                <button onclick="sendCommand('B')">B</button>
                <button onclick="sendCommand('ON')">PIN ON</button>
                <button onclick="sendCommand('OFF')">PIN OFF</button>
                <button onclick="sendCommand('TIMEPLUS')">TIME+</button>
                <button onclick="sendCommand('TIMEMINUS')">TIME-</button>
            </div>
        </div>
    </div>

    <!-- STATUS -->
    <div class="card">
        <h2>[ SYSTEM STATUS ]</h2>
        <div id="headless">Initializing...</div>
        <div style="margin-top:10px;">
            <div class="status-row">
                <span class="status-label">WiFi RSSI</span>
                <span class="status-value" id="rssi">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">Uptime</span>
                <span class="status-value" id="uptime">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">IP Address</span>
                <span class="status-value" id="ipaddr">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">SSID</span>
                <span class="status-value" id="ssid">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">Pin D0 Status</span>
                <span class="status-value"><span id="pinStatus">--</span></span>
            </div>
        </div>
    </div>

    <!-- SECURITY -->
    <div class="grid-2">
        <div class="card">
            <h2>[ SECURITY ]</h2>
            <div class="status-row">
                <span class="status-label">Honeypot</span>
                <span class="status-value"><span id="honeypotBadge" class="badge badge-green">OK</span></span>
            </div>
            <div class="status-row">
                <span class="status-label">Evil Twin</span>
                <div style="display:flex;align-items:center;gap:8px;">
                    <label style="font-size:16px;cursor:pointer;">
                        <input type="checkbox" id="evilTwinToggle" onchange="toggleEvilTwin()" checked>
                    </label>
                    <span id="evilTwinScanBadge" class="badge badge-green">ENABLED</span>
                </div>
            </div>
            <div class="status-row">
                <span class="status-label">Hosts Found</span>
                <span class="status-value" id="hostsCount">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">Vulnerabilities</span>
                <span class="status-value"><span id="vulnBadge" class="badge badge-green">0</span></span>
            </div>
        </div>

        <!-- LED STATUS -->
        <div class="card">
            <h2>[ LED STATUS ]</h2>
            <div class="status-row">
                <div style="display:flex;align-items:center;">
                    <span class="led-indicator" id="ledIndicator"></span>
                    <div>
                        <span class="status-label" id="ledStateLabel">Auto Mode</span><br>
                        <span style="font-size:11px;color:#888;" id="ledStateName">Idle</span>
                    </div>
                </div>
            </div>
            <div class="led-controls">
                <span style="font-size:11px;color:#888;display:block;margin-bottom:5px;">Manual Override:</span>
                <button class="led-btn led-btn-green" onclick="ledCommand('green')" title="Green"></button>
                <button class="led-btn led-btn-red" onclick="ledCommand('red')" title="Red"></button>
                <button class="led-btn led-btn-orange" onclick="ledCommand('amber')" title="Amber" style="background:#ffa500;"></button>
                <button class="led-btn led-btn-off" onclick="ledCommand('off')" title="Off"></button>
                <button class="led-btn led-btn-auto" onclick="ledCommand('auto')" title="Auto"></button>
            </div>
        </div>
    </div>

    <!-- HOSTS -->
    <div class="card">
        <h2>[ NETWORK HOSTS ]</h2>
        <button onclick="getHosts()" style="width:100%;">SCAN HOSTS</button>
        <div id="hosts">Click SCAN HOSTS to retrieve data...</div>
    </div>

    <footer>
        <p>Netgotchi by MXZZ | ESP8266 Port</p>
        <p><a href="https://github.com/MXZZ/Netgotchi/" target="_blank" style="color:#00ff41;text-decoration:none;">GitHub</a></p>
    </footer>
</div>
    <script>
        function updateCanvas() {
            fetch('/matrix')
                .then(r => r.json())
                .then(matrix => {
                    const c = document.getElementById('canvas');
                    const ctx = c.getContext('2d');
                    ctx.fillStyle = 'black';
                    ctx.fillRect(0, 0, c.width, c.height);
                    ctx.fillStyle = 'white';
                    for (let y = 0; y < matrix.length; y++)
                        for (let x = 0; x < matrix[y].length; x++)
                            if (matrix[y][x] === 1) ctx.fillRect(x, y, 1, 1);
                })
                .catch(() => {});
        }
        function sendCommand(cmd) {
            fetch('/command/' + cmd)
                .then(r => {
                    if (cmd === 'ON' || cmd === 'OFF') setTimeout(getPinStatus, 100);
                })
                .catch(() => {});
        }
        function getHosts() {
            fetch('/hosts')
                .then(r => r.text())
                .then(d => document.getElementById('hosts').innerHTML = d)
                .catch(() => {});
        }
        function getHeadlessStatus() {
            fetch('/headless')
                .then(r => r.text())
                .then(d => {
                    document.getElementById('headless').innerHTML = d;
                    // Parse the status string for badges
                    const parts = d.split(' ');
                    parts.forEach(p => {
                        if (p.includes('Honeypot:breached')) {
                            document.getElementById('honeypotBadge').textContent = 'BREACHED';
                            document.getElementById('honeypotBadge').className = 'badge badge-red';
                        } else if (p.includes('Honeypot:')) {
                            document.getElementById('honeypotBadge').textContent = 'OK';
                            document.getElementById('honeypotBadge').className = 'badge badge-green';
                        }
                        if (p.includes('EvilTwin:detected')) {
                            document.getElementById('vulnBadge').className = 'badge badge-purple';
                        }
                        if (p.includes('Host-Found:')) {
                            const n = p.split(':')[1] || '--';
                            document.getElementById('hostsCount').textContent = n;
                        }
                        if (p.includes('Vulnerabilities:')) {
                            const n = p.split(':')[1] || '0';
                            const vb = document.getElementById('vulnBadge');
                            vb.textContent = n;
                            vb.className = n > 0 ? 'badge badge-orange' : 'badge badge-green';
                        }
                    });
                })
                .catch(() => {});
        }
        function getSystemInfo() {
            fetch('/system')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('rssi').textContent = d.rssi ? d.rssi + ' dBm' : '--';
                    document.getElementById('uptime').textContent = d.uptime ? formatUptime(d.uptime) : '--';
                    document.getElementById('ipaddr').textContent = d.ip || '--';
                    document.getElementById('ssid').textContent = d.ssid || '--';
                })
                .catch(() => {});
        }
        function getPinStatus() {
            fetch('/pin/status')
                .then(r => r.json())
                .then(d => {
                    const el = document.getElementById('pinStatus');
                    const on = d.state === 'HIGH';
                    el.textContent = on ? 'HIGH (ON)' : 'LOW (OFF)';
                    el.style.color = on ? '#0f0' : '#888';
                })
                .catch(() => {});
        }
        function getEvilTwinStatus() {
            fetch('/eviltwin/status')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('evilTwinToggle').checked = d.enabled;
                    const b = document.getElementById('evilTwinScanBadge');
                    b.textContent = d.enabled ? 'ENABLED' : 'DISABLED';
                    b.className = d.enabled ? 'badge badge-green' : 'badge badge-red';
                })
                .catch(() => {});
        }
        function toggleEvilTwin() {
            const c = document.getElementById('evilTwinToggle');
            fetch('/eviltwin/' + (c.checked ? 'enable' : 'disable'))
                .then(() => getEvilTwinStatus())
                .catch(() => {});
        }
        function getLedStatus() {
            fetch('/led/status')
                .then(r => r.json())
                .then(d => {
                    const ind = document.getElementById('ledIndicator');
                    const lbl = document.getElementById('ledStateLabel');
                    const nm = document.getElementById('ledStateName');
                    if (d.enabled) {
                        ind.style.background = d.color;
                        ind.style.boxShadow = '0 0 15px ' + d.color;
                        nm.textContent = d.name || 'Idle';
                        lbl.textContent = 'LED Active';
                    } else {
                        ind.style.background = '#333';
                        ind.style.boxShadow = 'none';
                        nm.textContent = 'Not Configured';
                        lbl.textContent = 'LEDs Disabled';
                    }
                })
                .catch(() => {
                    document.getElementById('ledStateLabel').textContent = 'LEDs Disabled';
                    document.getElementById('ledIndicator').style.background = '#333';
                });
        }
        function ledCommand(cmd) {
            fetch('/led/' + cmd)
                .then(() => {
                    setTimeout(getLedStatus, 200);
                    if (cmd === 'auto') {
                        document.getElementById('ledStateLabel').textContent = 'Auto Mode';
                    }
                })
                .catch(() => {});
        }
        function formatUptime(s) {
            const h = Math.floor(s / 3600);
            const m = Math.floor((s % 3600) / 60);
            const sec = s % 60;
            return h + 'h ' + m + 'm ' + sec + 's';
        }
        // Polling
        setInterval(updateCanvas, 2000);
        setInterval(getHeadlessStatus, 2000);
        setInterval(getSystemInfo, 5000);
        setInterval(getPinStatus, 3000);
        setInterval(getEvilTwinStatus, 5000);
        setInterval(getLedStatus, 3000);
        // Initial
        updateCanvas();
        getHeadlessStatus();
        getSystemInfo();
        getPinStatus();
        getEvilTwinStatus();
        getLedStatus();
    </script>
</body></html>
)rawliteral";

#endif // WEBPAGE_H
