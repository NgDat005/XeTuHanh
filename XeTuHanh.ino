#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ===== WiFi AP =====
const char* ap_ssid = "ESP32-vehicle-control";
const char* ap_password = "12345678";

WebServer server(80);

// ===== ĐỘNG CƠ =====
const int LeftMotorForward  = 27;   
const int LeftMotorBackward = 26;   
const int RightMotorForward = 25;   
const int RightMotorBackward= 33;   

// ===== SIÊU ÂM + SERVO + LED =====
#define TRIG_PIN 32
#define ECHO_PIN 35
#define LED_PIN  2  
Servo servo_motor;

// ===== CẢM BIẾN DÒ LINE (5 mắt) =====
#define LINE1 14
#define LINE2 12
#define LINE3 4
#define LINE4 16
#define LINE5 17

// ===== BIẾN TRẠNG THÁI =====
int distance = 100;
int speedMotor = 200;
bool manualMode = true;
bool autoMode = false;
bool lineMode = false;

enum Direction {STOP, FORWARD, BACKWARD, LEFT, RIGHT};
Direction lastLineDirection = FORWARD;
unsigned long lostLineTime = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LeftMotorForward, OUTPUT);
  pinMode(LeftMotorBackward, OUTPUT);
  pinMode(RightMotorForward, OUTPUT);
  pinMode(RightMotorBackward, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  servo_motor.attach(13, 500, 2400); 
  servo_motor.write(90);

  pinMode(LINE1, INPUT);
  pinMode(LINE2, INPUT);
  pinMode(LINE3, INPUT);
  pinMode(LINE4, INPUT);
  pinMode(LINE5, INPUT);

  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("ESP32 da tao WiFi AP");
  Serial.print("SSID: "); Serial.println(ap_ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // ===== ROUTES WEB =====
  server.on("/", handleRoot);

  server.on("/forward", [](){ if(manualMode) moveForward(); server.send(200, "text/plain", "Forward"); });
  server.on("/backward", [](){ if(manualMode) moveBackward(); server.send(200, "text/plain", "Backward"); });
  server.on("/left", [](){ if(manualMode) turnLeft(); server.send(200, "text/plain", "Left"); });
  server.on("/right", [](){ if(manualMode) turnRight(); server.send(200, "text/plain", "Right"); });
  server.on("/stop", [](){ moveStop(); server.send(200, "text/plain", "Stop"); });

  server.on("/manual", [](){ setMode(true, false, false); server.send(200, "text/plain", "Manual Mode"); });
  server.on("/auto",   [](){ setMode(false, true, false); server.send(200, "text/plain", "Auto Obstacle Mode"); });
  server.on("/line",   [](){ setMode(false, false, true); server.send(200, "text/plain", "Line Follower Mode"); });

  server.on("/speed", [](){
    if(server.hasArg("val")){
      speedMotor = server.arg("val").toInt();
      speedMotor = constrain(speedMotor, 0, 255);
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/led", [](){
    if(distance <= 30) server.send(200, "text/plain", "ON");
    else server.send(200, "text/plain", "OFF");
  });

  server.begin();
  Serial.println("WebServer da san sang!");
}

void loop() {
  server.handleClient();

  distance = readPing();
  if (distance <= 30) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if (autoMode) {
    runAutoObstacle();
  } else if (lineMode) {
    runLineFollower();
  }
}

// ===== CHUYỂN CHẾ ĐỘ =====
void setMode(bool manual, bool autoObs, bool line) {
  moveStop();
  manualMode = manual;
  autoMode = autoObs;
  lineMode = line;
  Serial.printf("Mode: Manual=%d, Auto=%d, Line=%d\n", manualMode, autoMode, lineMode);
}

// ===== TRÁNH VẬT CẢN (giống code gốc) =====
void runAutoObstacle() {
  int distanceRight = 0;
  int distanceLeft = 0;

  distance = readPing();
  if (distance <= 25) {
    moveStop();
    delay(200);
    moveBackward();
    delay(400);
    moveStop();
    delay(200);

    distanceRight = lookRight();
    delay(200);
    distanceLeft = lookLeft();
    delay(200);

    if (distanceRight >= distanceLeft) {
      turnRight();
      delay(400);
    } else {
      turnLeft();
      delay(400);
    }
    moveStop();
    delay(100);
  } else {
    moveForward();
  }
}

int lookRight() {
  servo_motor.write(0);
  delay(400);
  int d = readPing();
  servo_motor.write(90);
  return d;
}

int lookLeft() {
  servo_motor.write(180);
  delay(400);
  int d = readPing();
  servo_motor.write(90);
  return d;
}

int readPing() {
  long duration;
  int cm;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  cm = duration * 0.034 / 2;
  if (cm == 0 || cm > 400) cm = 400;
  return cm;
}

// ===== DÒ LINE =====
void runLineFollower() {
  int s1 = digitalRead(LINE1);
  int s2 = digitalRead(LINE2);
  int s3 = digitalRead(LINE3);
  int s4 = digitalRead(LINE4);
  int s5 = digitalRead(LINE5);

  Serial.printf("Line: %d %d %d %d %d\n", s1, s2, s3, s4, s5);

  // ===== LOGIC MỚI: 1 = thấy line đen (active HIGH - phổ biến nhất) =====
  if ((s2 == 1 && s3 == 1) || s3 == 1 || (s3 == 1 && s4 == 1)) {
    setLineDirection(FORWARD);
  }
  else if (s1 == 1 || (s1 == 1 && s2 == 1)) {
    setLineDirection(RIGHT);
  }
  else if (s2 == 1) {
    setLineDirection(RIGHT);
  }
  else if (s5 == 1 || (s4 == 1 && s5 == 1)) {
    setLineDirection(LEFT);
  }
  else if (s4 == 1) {
    setLineDirection(LEFT);
  }
  else {
    setLineDirection(lastLineDirection);
    if (millis() - lostLineTime > 1000) {
      setLineDirection(STOP);
    }
  }

  if (s1 == 1 || s2 == 1 || s3 == 1 || s4 == 1 || s5 == 1) {
    lostLineTime = millis();
  }

  delay(20);
}

void setLineDirection(Direction dir) {
  motorControl(dir);
  if (dir != STOP) lastLineDirection = dir;
}

void moveForward()  { 
  motorControl(FORWARD); 
  }

void moveBackward() { 
  motorControl(BACKWARD); }

void turnLeft()     { 
  motorControl(LEFT); 
}

void turnRight()    {
   motorControl(RIGHT); 
   }
   
void moveStop()     { 
  motorControl(STOP); 
  }

// ===== TRANG WEB =====
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      body { background:#202020; color:white; font-family:Arial; text-align:center; }
      .joystick { width:200px; height:200px; background:#444; border-radius:50%; margin:20px; display:inline-block; position:relative; touch-action:none; }
      .stick { width:80px; height:80px; background:#1e90ff; border-radius:50%; position:absolute; top:60px; left:60px; }
      button { width:120px; height:50px; margin:10px; font-size:18px; border-radius:12px; border:none; }
      .mode { background:#ff9800; color:white; }
      .stop { background:red; color:white; font-weight:bold; }
      #led { width:30px; height:30px; border-radius:50%; display:inline-block; margin:10px; background:green; }
      #speedValue { font-size:18px; margin-top:10px; }
    </style>
  </head>
  <body>
    <h2>ESP32 Vehicle Control</h2>
    
    <div>
      <div id="joystick1" class="joystick"><div id="stick1" class="stick"></div></div>
      <div id="joystick2" class="joystick"><div id="stick2" class="stick"></div></div>
    </div>

    <div id="buttons">
      <button class="mode" onclick="sendCmd('manual')">Manual Mode</button>
      <button class="mode" onclick="sendCmd('auto')">Auto Obstacle</button>
      <button class="mode" onclick="sendCmd('line')">Line Follower</button><br>
      <button class="stop" onclick="sendCmd('stop')">STOP</button>
    </div>

    <h3>Obstacle Detector</h3>
    <div id="led"></div>

    <h3>Speed Control (Manual)</h3>
    <input type="range" min="0" max="255" value="200" id="speedSlider" onchange="updateSpeed(this.value)">
    <div id="speedValue">Speed: 200</div>

    <script>
      function setupJoystick(joyId, stickId, axis) {
        let joy = document.getElementById(joyId);
        let stick = document.getElementById(stickId);
        let centerX = joy.offsetWidth/2, centerY = joy.offsetHeight/2;
        let active=false, currentCmd="stop";
        
        joy.addEventListener("touchstart", e=>{ active=true; e.preventDefault(); });
        joy.addEventListener("touchend", ()=>{ active=false; resetStick(); sendCmd("stop"); currentCmd="stop"; });
        joy.addEventListener("touchmove", e=>{
          if(!active) return;
          let rect=joy.getBoundingClientRect();
          let x=e.touches[0].clientX-rect.left, y=e.touches[0].clientY-rect.top;
          let dx=x-centerX, dy=y-centerY;
          let dist=Math.sqrt(dx*dx+dy*dy), maxDist=70;
          if(dist>maxDist){ dx*=maxDist/dist; dy*=maxDist/dist; }
          stick.style.left=(centerX+dx-40)+"px";
          stick.style.top=(centerY+dy-40)+"px";
          
          let cmd="stop";
          if(axis==="Y"){
            if(dy<-20) cmd="forward";
            else if(dy>20) cmd="backward";
          } else {
            if(dx<-20) cmd="left";
            else if(dx>20) cmd="right";
          }
          if(cmd!==currentCmd){ currentCmd=cmd; sendCmd(cmd); }
        });
        
        function resetStick(){
          stick.style.left="60px"; stick.style.top="60px";
        }
      }
      
      setupJoystick("joystick1","stick1","Y");  // Trái-phải: tiến/lùi
      setupJoystick("joystick2","stick2","X");  // Phải: trái/phải

      function sendCmd(cmd){
        fetch("/"+cmd).catch(err=>console.log(err));
      }

      function updateSpeed(val){
        document.getElementById("speedValue").innerText = "Speed: " + val;
        fetch("/speed?val="+val);
      }

      setInterval(()=>{
        fetch("/led").then(r=>r.text()).then(state=>{
          document.getElementById("led").style.background = (state=="ON") ? "red" : "green";
        });
      }, 500);
    </script>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void motorControl(Direction dir) {
  switch (dir) {
    case FORWARD:
      digitalWrite(LeftMotorForward, HIGH);
      digitalWrite(LeftMotorBackward, LOW);
      digitalWrite(RightMotorBackward, HIGH);
      digitalWrite(RightMotorForward, LOW);
      break;
    case BACKWARD:
      digitalWrite(LeftMotorBackward, HIGH);
      digitalWrite(LeftMotorForward, LOW);
      digitalWrite(RightMotorForward, HIGH);
      digitalWrite(RightMotorBackward, LOW);
      break;
    case LEFT:
      digitalWrite(RightMotorBackward, HIGH);
      digitalWrite(RightMotorForward, LOW);
      digitalWrite(LeftMotorForward, LOW);
      digitalWrite(LeftMotorBackward, HIGH);
      break;
    case RIGHT:
      digitalWrite(LeftMotorForward, HIGH);
      digitalWrite(LeftMotorBackward, LOW);
      digitalWrite(RightMotorForward, HIGH);
      digitalWrite(RightMotorBackward, LOW);
      break;
    case STOP:
    default:
      digitalWrite(LeftMotorForward, LOW);
      digitalWrite(LeftMotorBackward, LOW);
      digitalWrite(RightMotorForward, LOW);
      digitalWrite(RightMotorBackward, LOW);
      break;
  }
}