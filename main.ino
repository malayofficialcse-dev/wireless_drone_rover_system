#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>

// --- Drone Configuration ---
#define MIN_THROTTLE 1000 // Minimum throttle (ESC arming)
#define MAX_THROTTLE 2000 // Maximum throttle
#define BASE_THROTTLE 1100 // Hover throttle (adjust based on your drone)
#define THROTTLE_STEP 50 // Speed adjustment step

// Drone ESC Pins
#define ESC1_PIN D7
#define ESC2_PIN D8
#define ESC3_PIN D0
#define ESC4_PIN 3  // RX Pin

Servo esc1, esc2, esc3, esc4;
int currentThrottle = BASE_THROTTLE;
bool isArmed = false;

// --- Rover Configuration ---
// Motor Control Pins
#define IN1 D1 // Left motor forward
#define IN2 D2 // Left motor backward
#define IN3 D3 // Right motor forward
#define IN4 D4 // Right motor backward
#define ENA D5 // Left motor speed (PWM)
#define ENB D6 // Right motor speed (PWM)

// Speed settings (0-1023)
int motorSpeed = 800; // Default speed (about 75%)
int turnSpeed = 600; // Turning speed

// --- WiFi Configuration ---
const char* ssid = "Redmi Note 9 Pro";
const char* password = "00000000";
const char* hostname = "wifirover";

ESP8266WebServer server(80);

// --- Functions ---
void setAllEsc(int value) {
  esc1.writeMicroseconds(value);
  esc2.writeMicroseconds(value);
  esc3.writeMicroseconds(value);
  esc4.writeMicroseconds(value);
}

void armDrone() {
  setAllEsc(MIN_THROTTLE);
  delay(1000);
  setAllEsc(BASE_THROTTLE);
  isArmed = true;
  Serial.println("Drone Armed! Throttle=" + String(BASE_THROTTLE));
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  Serial.println("Rover: Moving Forward");
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  Serial.println("Rover: Moving Backward");
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, turnSpeed);
  analogWrite(ENB, turnSpeed);
  Serial.println("Rover: Turning Left");
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, turnSpeed);
  analogWrite(ENB, turnSpeed);
  Serial.println("Rover: Turning Right");
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  Serial.println("Rover: Motors Stopped");
}

// Forward Declarations for Web Handlers
void handleRoot();
void handleForward();
void handleBackward();
void handleLeft();
void handleRight();
void handleStop();
void handleSpeed();
void handleDroneArm();
void handleDroneUp();
void handleDroneDown();
void handleDronePitchFwd();
void handleDronePitchBwd();
void handleDroneRollLeft();
void handleDroneRollRight();
void handleDroneStop();
void handleStatus();

void setup() {
  // Initialize serial communication (TX only if we use RX for ESC, but we can just use 115200)
  Serial.begin(115200);
  delay(1000);
  
  // Initialize Drone ESCs
  esc1.attach(ESC1_PIN);
  esc2.attach(ESC2_PIN);
  esc3.attach(ESC3_PIN);
  esc4.attach(ESC4_PIN);
  setAllEsc(MIN_THROTTLE); // Initialization signal
  
  // Initialize Rover Motor Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  stopMotors();

  Serial.println("\n\nBooting WiFi Drone & Rover Controller...");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.hostname(hostname);
  
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (MDNS.begin(hostname)) {
      Serial.println("mDNS responder started");
    }
  } else {
    Serial.println("\nFailed to connect to WiFi! Restarting...");
    delay(5000);
    ESP.restart();
  }
  
  // Rover Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/forward", HTTP_GET, handleForward);
  server.on("/backward", HTTP_GET, handleBackward);
  server.on("/left", HTTP_GET, handleLeft);
  server.on("/right", HTTP_GET, handleRight);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/speed", HTTP_GET, handleSpeed);
  
  // Drone Routes
  server.on("/drone/arm", HTTP_GET, handleDroneArm);
  server.on("/drone/up", HTTP_GET, handleDroneUp);
  server.on("/drone/down", HTTP_GET, handleDroneDown);
  server.on("/drone/pitch_fwd", HTTP_GET, handleDronePitchFwd);
  server.on("/drone/pitch_bwd", HTTP_GET, handleDronePitchBwd);
  server.on("/drone/roll_left", HTTP_GET, handleDroneRollLeft);
  server.on("/drone/roll_right", HTTP_GET, handleDroneRollRight);
  server.on("/drone/stop", HTTP_GET, handleDroneStop);
  
  server.on("/status", HTTP_GET, handleStatus);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  MDNS.update();
}

// --- Web Server Handlers ---

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Drone & Rover Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 10px; background: #111; color: #fff;}
    .control-panel { max-width: 400px; margin: 0 auto; background: #222; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px #000; margin-bottom: 20px;}
    .btn {
      width: 80px; height: 80px; margin: 5px; 
      font-size: 16px; border-radius: 10px;
      border: none; background: #4CAF50; color: white; cursor: pointer;
    }
    .btn:active { background: #45a049; }
    .btn-stop { background: #f44336; }
    .btn-stop:active { background: #d32f2f; }
    .btn-arm { background: #ff9800; width: 100%; margin: 10px 0; height: 50px;}
    .btn-arm:active { background: #e68a00; }
    .speed-control { margin: 20px 0; }
    .status { margin-top: 20px; padding: 10px; background: #333; border-radius: 5px;}
    h2 { border-bottom: 1px solid #444; padding-bottom: 10px; }
  </style>
</head>
<body>
  <h1>Dual Controller</h1>
  
  <div class="control-panel">
    <h2>Rover Controls</h2>
    <button class="btn" onclick="control('/forward')">Fwd ↑</button><br>
    <button class="btn" onclick="control('/left')">L ←</button>
    <button class="btn btn-stop" onclick="control('/stop')">STOP</button>
    <button class="btn" onclick="control('/right')">R →</button><br>
    <button class="btn" onclick="control('/backward')">Bwd ↓</button>
    
    <div class="speed-control">
      <h3>Rover Speed</h3>
      <input type="range" min="300" max="1023" value="800" id="speedSlider" onchange="setSpeed(this.value)">
      <span id="speedValue">800</span>
    </div>
  </div>

  <div class="control-panel">
    <h2>Drone Controls</h2>
    <button class="btn btn-arm" onclick="control('/drone/arm')">ARM DRONE</button>
    <div>
      <button class="btn" onclick="control('/drone/pitch_fwd')">Pitch ↑</button><br>
      <button class="btn" onclick="control('/drone/roll_left')">Roll ←</button>
      <button class="btn" onclick="control('/drone/up')">Up ↑</button>
      <button class="btn" onclick="control('/drone/roll_right')">Roll →</button><br>
      <button class="btn" onclick="control('/drone/pitch_bwd')">Pitch ↓</button>
      <button class="btn" onclick="control('/drone/down')">Down ↓</button>
    </div>
    <button class="btn btn-stop" style="width: 100%; margin-top: 10px; height: 50px;" onclick="control('/drone/stop')">DISARM & STOP DRONE</button>
  </div>
  
  <div class="control-panel status">
    <p>IP: )=====";
  html += WiFi.localIP().toString();
  html += R"=====(</p>
    <p>Signal: )=====";
  html += WiFi.RSSI();
  html += R"=====( dBm</p>
  </div>
  
  <script>
    function control(route) {
      fetch(route).then(r => console.log('Command ' + route + ' sent'));
    }
    
    function setSpeed(speed) {
      document.getElementById('speedValue').textContent = speed;
      fetch('/speed?value=' + speed).then(r => console.log('Speed set to ' + speed));
    }
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

// Rover Route Functions
void handleForward() { moveForward(); server.send(200, "text/plain", "Rover Forward"); }
void handleBackward() { moveBackward(); server.send(200, "text/plain", "Rover Backward"); }
void handleLeft() { turnLeft(); server.send(200, "text/plain", "Rover Left"); }
void handleRight() { turnRight(); server.send(200, "text/plain", "Rover Right"); }
void handleStop() { stopMotors(); server.send(200, "text/plain", "Rover Stopped"); }
void handleSpeed() {
  if (server.hasArg("value")) {
    motorSpeed = server.arg("value").toInt();
    turnSpeed = motorSpeed * 0.75;
    server.send(200, "text/plain", "Speed set to " + String(motorSpeed));
  } else {
    server.send(400, "text/plain", "Missing speed value");
  }
}

// Drone Route Functions
void handleDroneArm() {
  armDrone();
  server.send(200, "text/plain", "Drone Armed");
}

void handleDroneUp() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  currentThrottle = constrain(currentThrottle + THROTTLE_STEP, MIN_THROTTLE, MAX_THROTTLE);
  setAllEsc(currentThrottle);
  server.send(200, "text/plain", "Throttle: " + String(currentThrottle));
}

void handleDroneDown() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  currentThrottle = constrain(currentThrottle - THROTTLE_STEP, MIN_THROTTLE, MAX_THROTTLE);
  setAllEsc(currentThrottle);
  server.send(200, "text/plain", "Throttle: " + String(currentThrottle));
}

void handleDronePitchFwd() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  esc1.writeMicroseconds(currentThrottle + 100);
  esc2.writeMicroseconds(currentThrottle + 100);
  esc3.writeMicroseconds(currentThrottle);
  esc4.writeMicroseconds(currentThrottle);
  server.send(200, "text/plain", "Pitch Fwd");
}

void handleDronePitchBwd() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  esc1.writeMicroseconds(currentThrottle);
  esc2.writeMicroseconds(currentThrottle);
  esc3.writeMicroseconds(currentThrottle + 100);
  esc4.writeMicroseconds(currentThrottle + 100);
  server.send(200, "text/plain", "Pitch Bwd");
}

void handleDroneRollLeft() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  esc1.writeMicroseconds(currentThrottle);
  esc2.writeMicroseconds(currentThrottle + 100);
  esc3.writeMicroseconds(currentThrottle + 100);
  esc4.writeMicroseconds(currentThrottle);
  server.send(200, "text/plain", "Roll Left");
}

void handleDroneRollRight() {
  if(!isArmed) { server.send(400, "text/plain", "Not Armed"); return; }
  esc1.writeMicroseconds(currentThrottle + 100);
  esc2.writeMicroseconds(currentThrottle);
  esc3.writeMicroseconds(currentThrottle);
  esc4.writeMicroseconds(currentThrottle + 100);
  server.send(200, "text/plain", "Roll Right");
}

void handleDroneStop() {
  setAllEsc(MIN_THROTTLE);
  isArmed = false;
  currentThrottle = BASE_THROTTLE;
  server.send(200, "text/plain", "Drone Stopped");
}

void handleStatus() {
  String status = "WiFi Car & Drone Status\n";
  status += "IP: " + WiFi.localIP().toString() + "\n";
  status += "Drone Armed: " + String(isArmed ? "Yes" : "No") + "\n";
  status += "Rover Speed: " + String(motorSpeed) + "/1023\n";
  server.send(200, "text/plain", status);
}
