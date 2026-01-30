from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO
import datetime
import tensorflow as tf
import numpy as np

# 🔥 ADDED (no removals)
import csv
import os
import time

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# ================= ANN LOAD =================
interpreter = tf.lite.Interpreter(model_path="lighting_model.tflite")
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("ANN model loaded on server")

# ================= STATE =================
current_mode = "auto"
manual_brightness = 45
manual_power = True

# ================= MANUAL LOGGING =================
manual_log_until = 0
CSV_FILE = "data_log.csv"

if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "lux", "brightness", "hour", "mode"])

# ================= DASHBOARD =================
@app.route("/")
def dashboard():
    return render_template("dashboard.html")

# ================= ANN PREDICTION =================
@app.route("/predict", methods=["POST"])
def predict():
    data = request.json

    lux  = float(data["lux"])
    hour = int(data["hour"])

    # -------- FEATURES (MATCH TRAINING) --------
    night_mode = 1 if 0 <= hour < 6 else 0

    weather_foggy = 0
    weather_rainy = 0
    weather_sunny = 1

    month = datetime.datetime.now().month
    season_winter = 1 if month in [11, 12, 1, 2] else 0

    # -------- SCALE NUMERIC FEATURES --------
    lux_s  = lux / 500.0
    hour_s = hour / 23.0

    # -------- EXACT FEATURE ORDER (7) --------
    x = np.array([[ 
        lux_s,
        hour_s,
        0,
        weather_foggy,
        weather_rainy,
        weather_sunny,
        season_winter
    ]], dtype=np.float32)

    interpreter.set_tensor(input_details[0]['index'], x)
    interpreter.invoke()

    brightness = float(
        interpreter.get_tensor(output_details[0]['index'])[0][0]
    )

    if lux >= 500:
        brightness = 0.0

    brightness = int(np.clip(brightness, 0, 90))
    return jsonify({"brightness": brightness})

# ================= SOCKET DATA PUSH (EXISTING) =================
@app.route("/log-data", methods=["POST"])
def log_data():
    data = request.get_json()

    row = {
        "lux": float(data["lux"]),
        "brightness": int(data["brightness"]),
        "hour": int(data["hour"]),
        "mode": current_mode,
        "timestamp": datetime.datetime.now().strftime("%H:%M:%S")
    }

    socketio.emit("sensor_update", row)
    return jsonify({"status": "ok"})

# ================= ESP32 DATA PUSH (NEW) =================
@app.route("/push-data", methods=["POST"])
def push_data():
    global manual_log_until

    data = request.json

    row = {
        "lux": float(data["lux"]),
        "brightness": int(data["brightness"]),
        "hour": int(data["hour"]),
        "mode": data["mode"],
        "timestamp": datetime.datetime.now().strftime("%H:%M:%S")
    }

    # 🔥 LIVE UPDATE
    socketio.emit("sensor_update", row)

    # 🔥 CSV LOGGING FOR 10 MIN IN MANUAL MODE
    if current_mode == "manual" and time.time() < manual_log_until:
        with open(CSV_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([
                row["timestamp"],
                row["lux"],
                row["brightness"],
                row["hour"],
                row["mode"]
            ])

    return jsonify({"status": "ok"})

# ================= MANUAL CONTROL =================
@app.route("/set-manual", methods=["POST"])
def set_manual():
    global current_mode
    current_mode = "manual"
    return jsonify({"status": "manual"})

@app.route("/set-brightness", methods=["POST"])
def set_brightness():
    global manual_brightness, manual_log_until
    manual_brightness = int(request.json.get("brightness", manual_brightness))
    manual_log_until = time.time() + 600  # 🔥 10 minutes
    return jsonify({"status": "ok"})

@app.route("/manual-power", methods=["POST"])
def manual_power_control():
    global manual_power, manual_log_until
    manual_power = request.json.get("state") == "on"
    manual_log_until = time.time() + 600  # 🔥 10 minutes
    return jsonify({"power": manual_power})

@app.route("/auto-mode", methods=["POST"])
def auto_mode():
    global current_mode
    current_mode = "auto"
    return jsonify({"status": "auto"})

@app.route("/get-mode")
def get_mode():
    return current_mode

@app.route("/get-brightness")
def get_brightness():
    return str(manual_brightness)

@app.route("/get-power")
def get_power():
    return "1" if manual_power else "0"

# ================= RUN =================
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)
