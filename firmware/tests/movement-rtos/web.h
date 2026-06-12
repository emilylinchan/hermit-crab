#pragma once

#include <Arduino.h>

// ======================================================================
// WEB INTERFACE
// ======================================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Hermit Crab</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #111;
      color: #eee;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px 12px;
      gap: 18px;
      min-height: 100vh;
    }
    h1 { font-size: 22px; color: #fff; letter-spacing: 1px; }

    .status-bar {
      font-size: 13px;
      color: #aaa;
      background: #1e1e1e;
      padding: 6px 16px;
      border-radius: 20px;
      border: 1px solid #333;
    }
    .status-bar span { color: #4fc; font-weight: 600; }

    .card {
      background: #1a1a1a;
      border: 1px solid #2e2e2e;
      border-radius: 14px;
      padding: 16px;
      width: 100%;
      max-width: 420px;
    }
    .card-title {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      color: #666;
      margin-bottom: 12px;
    }

    /* D-pad */
    .dpad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      grid-template-rows: repeat(2, 1fr);
      gap: 10px;
      max-width: 240px;
      margin: 0 auto 12px;
    }
    .dpad button {
      font-size: 26px;
      aspect-ratio: 1;
      background: #2a2a2a;
      border: 1px solid #3a3a3a;
      border-radius: 10px;
      color: #fff;
      cursor: pointer;
      transition: background 0.1s, transform 0.1s;
      touch-action: manipulation;
    }
    .dpad button:active { background: #3a5a3a; transform: scale(0.94); }
    .spacer { visibility: hidden; }

    /* Stop button */
    .btn-stop {
      width: 100%;
      padding: 14px;
      font-size: 16px;
      font-weight: 700;
      letter-spacing: 2px;
      background: #5a1a1a;
      border: 1px solid #a33;
      border-radius: 10px;
      color: #fff;
      cursor: pointer;
      transition: background 0.1s;
      touch-action: manipulation;
    }
    .btn-stop:active { background: #7a2a2a; }

    /* Pose grid */
    .pose-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }
    .btn-pose {
      padding: 10px 6px;
      font-size: 13px;
      background: #1f3a2a;
      border: 1px solid #2a5a3a;
      border-radius: 9px;
      color: #cec;
      cursor: pointer;
      transition: background 0.1s;
      touch-action: manipulation;
    }
    .btn-pose:active { background: #2a5a3a; }

    /* Direct motor */
    .motor-row {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-top: 8px;
    }
    .motor-row label { font-size: 12px; color: #888; width: 60px; flex-shrink: 0; }
    .motor-row input[type=range] {
      flex: 1;
      accent-color: #4fc;
    }
    .motor-row .motor-val { font-size: 12px; color: #aaa; width: 36px; text-align: right; }

    footer { font-size: 11px; color: #444; margin-top: 4px; }
  </style>
</head>
<body>

<h1>🦀 Hermit Crab</h1>

<div class="status-bar">Command: <span id="status">idle</span></div>

<!-- Movement -->
<div class="card">
  <div class="card-title">Movement</div>
  <div class="dpad">
    <div class="spacer"></div>
    <button onmousedown="move('walk')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('walk')" ontouchend="sendCmd('stop')">▲</button>
    <div class="spacer"></div>
    <button onmousedown="move('left')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('left')" ontouchend="sendCmd('stop')">◀</button>
    <button onmousedown="move('back')"  onmouseup="sendCmd('stop')"
            ontouchstart="move('back')" ontouchend="sendCmd('stop')">▼</button>
    <button onmousedown="move('right')" onmouseup="sendCmd('stop')"
            ontouchstart="move('right')" ontouchend="sendCmd('stop')">▶</button>
  </div>
  <button class="btn-stop" onclick="sendCmd('stop')">■ STOP</button>
</div>

<!-- Poses -->
<div class="card">
  <div class="card-title">Poses &amp; Animations</div>
  <div class="pose-grid">
    <button class="btn-pose" onclick="sendCmd('rest')">Rest</button>
    <button class="btn-pose" onclick="sendCmd('stand')">Stand</button>
    <button class="btn-pose" onclick="sendCmd('wave')">Wave</button>
    <button class="btn-pose" onclick="sendCmd('dance')">Dance</button>
    <button class="btn-pose" onclick="sendCmd('swim')">Swim</button>
    <button class="btn-pose" onclick="sendCmd('point')">Point</button>
    <button class="btn-pose" onclick="sendCmd('pushup')">Pushup</button>
    <button class="btn-pose" onclick="sendCmd('bow')">Bow</button>
    <button class="btn-pose" onclick="sendCmd('cute')">Cute</button>
    <button class="btn-pose" onclick="sendCmd('freaky')">Freaky</button>
    <button class="btn-pose" onclick="sendCmd('worm')">Worm</button>
    <button class="btn-pose" onclick="sendCmd('shake')">Shake</button>
    <button class="btn-pose" onclick="sendCmd('shrug')">Shrug</button>
    <button class="btn-pose" onclick="sendCmd('dead')">Dead</button>
    <button class="btn-pose" onclick="sendCmd('crab')">Crab</button>
  </div>
</div>

<!-- Direct Motor Control -->
<div class="card">
  <div class="card-title">Direct Motor Control</div>
  <div id="motorSliders"></div>
</div>

<footer>192.168.0.1 · Serial CLI also active</footer>

<script>
const LABELS = ['R1','R2','L1','L2','R4','R3','L3','L4'];

// Build motor sliders
const container = document.getElementById('motorSliders');
LABELS.forEach((label, i) => {
  const row = document.createElement('div');
  row.className = 'motor-row';
  row.innerHTML = `
    <label>S${i} ${label}</label>
    <input type="range" min="0" max="180" value="90"
           oninput="motorMove(${i}, this.value, this.nextElementSibling)">
    <span class="motor-val">90°</span>`;
  container.appendChild(row);
});

let moveInterval = null;

function move(cmd) {
  sendCmd(cmd);
  // Re-send every 200 ms while held — keeps state alive
  // if a future watchdog is added
  moveInterval = setInterval(() => sendCmd(cmd), 200);
}

function sendCmd(cmd) {
  if (moveInterval && cmd === 'stop') {
    clearInterval(moveInterval);
    moveInterval = null;
  }
  fetch('/cmd?c=' + cmd)
    .then(r => r.text())
    .then(t => document.getElementById('status').textContent = t)
    .catch(() => document.getElementById('status').textContent = 'error');
}

function motorMove(idx, val, display) {
  display.textContent = val + '°';
  fetch(`/motor?i=${idx}&a=${val}`).catch(()=>{});
}
</script>
</body>
</html>
)rawliteral";