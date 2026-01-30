// ================= SOCKET =================
const socket = io();

// ================= DOM =================
const slider = document.getElementById("brightnessSlider");
const sliderValue = document.getElementById("sliderValue");
const powerToggle = document.getElementById("powerToggle");

const luxEl = document.getElementById("lux");
const hourEl = document.getElementById("hour");
const brightnessEl = document.getElementById("brightness");
const modeEl = document.getElementById("mode");

// ================= CHART : BRIGHTNESS =================
const ctx = document.getElementById("chart").getContext("2d");

const brightnessChart = new Chart(ctx, {
  type: "line",
  data: {
    labels: [],
    datasets: [{
      label: "Brightness (%)",
      data: [],
      borderWidth: 2,
      tension: 0.3
    }]
  },
  options: {
    animation: false,
    scales: {
      y: { min: 0, max: 90 }
    }
  }
});

// ================= POWER MODEL =================
const MAX_LED_CURRENT = 0.30;     // A
const BOOST_EFFICIENCY = 0.85;
const FIXED_LED_POWER = 2.25;     // W

// ================= CUMULATIVE ENERGY =================
let smartEnergy = 0;  // Wh
let fixedEnergy = 0;
let lastEnergyUpdate = Date.now();

// ================= BRIGHTNESS → VOLTAGE =================
function getVoltage(brightness) {
  if (brightness >= 90) return 7.5;
  if (brightness >= 80) return 7.4;
  if (brightness >= 70) return 7.2;
  if (brightness >= 60) return 7.0;
  if (brightness >= 50) return 6.9;
  if (brightness >= 40) return 6.7;
  if (brightness >= 30) return 6.6;
  if (brightness >= 20) return 6.4;
  if (brightness >= 10) return 6.3;
  return 0.0;
}

// ================= POWER CALCULATION =================
function calculateSmartPower(brightness) {
  if (brightness <= 0) return 0;
  const voltage = getVoltage(brightness);
  const current = (brightness / 90) * MAX_LED_CURRENT;
  return voltage * current;
}

// ================= ENERGY INTEGRATION (UNCHANGED) =================
function updateEnergy(smartPower) {
  const now = Date.now();
  const deltaHrs = (now - lastEnergyUpdate) / 3600000;
  lastEnergyUpdate = now;

  smartEnergy += smartPower * deltaHrs;
  fixedEnergy += FIXED_LED_POWER * deltaHrs;
}

// ================= ENERGY PER HOUR (NEW) =================
function calculateEnergyPerHour(power) {
  // Wh consumed if current power is held for 1 hour
  return power * 1.0;
}

// ================= CHART : POWER =================
const powerCtx = document.getElementById("powerChart").getContext("2d");

const powerChart = new Chart(powerCtx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Smart Light Power (W)",
        data: [],
        borderWidth: 2,
        tension: 0.3
      },
      {
        label: "Fixed LED Power (W)",
        data: [],
        borderWidth: 2,
        borderDash: [6, 6]
      }
    ]
  },
  options: {
    animation: false,
    scales: {
      y: { min: 0 }
    }
  }
});

// ================= CHART : ENERGY =================
const energyCtx = document.getElementById("energyChart").getContext("2d");

const energyChart = new Chart(energyCtx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Smart Light Energy (Wh – Cumulative)",
        data: [],
        borderWidth: 2,
        tension: 0.3
      },
      {
        label: "Fixed Light Energy (Wh – Cumulative)",
        data: [],
        borderWidth: 2,
        borderDash: [6, 6]
      },
      {
        label: "Smart Light Energy (Wh / hour)",
        data: [],
        borderWidth: 2,
        tension: 0.3,
        borderDash: [3, 3]
      },
      {
        label: "Fixed Light Energy (Wh / hour)",
        data: [],
        borderWidth: 2,
        borderDash: [3, 3]
      }
    ]
  },
  options: {
    animation: false,
    scales: {
      y: {
        min: 0,
        title: {
          display: true,
          text: "Energy (Wh)"
        }
      }
    }
  }
});

// ================= SLIDER =================
slider.addEventListener("input", () => {
  sliderValue.innerText = slider.value;

  fetch("/set-brightness", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ brightness: slider.value })
  });
});

// ================= MODE BUTTONS =================
function manualMode() {
  fetch("/set-manual", { method: "POST" })
    .then(() => modeEl.innerText = "MANUAL");
}

function autoMode() {
  fetch("/auto-mode", { method: "POST" })
    .then(() => modeEl.innerText = "AUTO");
}

// ================= TOGGLE SWITCH =================
function togglePower() {
  fetch("/manual-power", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ state: powerToggle.checked ? "on" : "off" })
  });
}

// ================= LIVE DATA =================
socket.on("sensor_update", data => {
  luxEl.innerText = Number(data.lux).toFixed(1);
  hourEl.innerText = data.hour;
  brightnessEl.innerText = data.brightness;
  modeEl.innerText = data.mode.toUpperCase();

  // -------- BRIGHTNESS --------
  brightnessChart.data.labels.push(data.timestamp);
  brightnessChart.data.datasets[0].data.push(data.brightness);

  if (brightnessChart.data.labels.length > 60) {
    brightnessChart.data.labels.shift();
    brightnessChart.data.datasets[0].data.shift();
  }
  brightnessChart.update();

  // -------- POWER --------
  const smartPower = calculateSmartPower(data.brightness);
  updateEnergy(smartPower);

  powerChart.data.labels.push(data.timestamp);
  powerChart.data.datasets[0].data.push(Number(smartPower.toFixed(2)));
  powerChart.data.datasets[1].data.push(FIXED_LED_POWER);

  if (powerChart.data.labels.length > 60) {
    powerChart.data.labels.shift();
    powerChart.data.datasets[0].data.shift();
    powerChart.data.datasets[1].data.shift();
  }
  powerChart.update();

  // -------- ENERGY --------
  const smartWhPerHour = calculateEnergyPerHour(smartPower);
  const fixedWhPerHour = FIXED_LED_POWER;

  energyChart.data.labels.push(data.timestamp);
  energyChart.data.datasets[0].data.push(Number(smartEnergy.toFixed(4)));
  energyChart.data.datasets[1].data.push(Number(fixedEnergy.toFixed(4)));
  energyChart.data.datasets[2].data.push(Number(smartWhPerHour.toFixed(2)));
  energyChart.data.datasets[3].data.push(Number(fixedWhPerHour.toFixed(2)));

  if (energyChart.data.labels.length > 60) {
    energyChart.data.labels.shift();
    energyChart.data.datasets.forEach(ds => ds.data.shift());
  }
  energyChart.update();

  // -------- SYNC CONTROLS --------
  if (data.mode === "manual") {
    slider.value = data.brightness;
    sliderValue.innerText = data.brightness;
  }
});
