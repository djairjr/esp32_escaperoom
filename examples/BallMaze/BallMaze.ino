/**
 * EXAMPLE: BALL MAZE - MAZE WITH LIS3DH ACCELEROMETER
 * ----------------------------------------------------
 * Physical maze puzzle where the player tilts a box
 * to move a virtual ball across a 2D plane. The LIS3DH
 * accelerometer built into the PropMaker detects the tilt and the software
 * integrates the ball's position. The ball must visit N target zones
 * to solve the puzzle.
 *
 * Hardware:
 *   - LIS3DH Accelerometer (I2C, built into the Feather)
 *   - External NeoPixel for visual feedback (2D mapping)
 *   - MP3 audio via I2S (LittleFS)
 *   - Relay/Lock on the configured pin (lockPin)
 *
 * Configuration via /ballmaze.json on LittleFS.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (added by the ESP32 port)
// ============================================================================
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
HASensor haSensorState("state");

void initHA() {
  netConfig.begin();
  if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");
  haSensorState.setName("State");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================================================
// PHYSICAL CONSTANTS
// ============================================================================
#define GRAVITY_MS2 9.81f
#define MAX_TARGETS 8
#define PHYSICS_DT 0.0167f     // ~60 Hz
#define IDLE_CHIME_INTERVAL 15000

// ============================================================================
// JSON CONFIGURATION
// ============================================================================
struct TargetZone {
  float cx, cy;   // center
  float radius;   // detection radius
  bool hit;       // already visited
};

struct BallMazeConfig {
  // Physics
  float accelScale = 600.0f;   // sensitivity (tilt -> acceleration)
  float damping   = 3.5f;      // velocity damping
  float bounce    = 0.3f;      // wall restitution
  float maxSpeed  = 250.0f;    // terminal velocity
  float deadZone  = 0.25f;     // tilt dead zone (normalized)

  // Maze
  float mazeW = 200.0f;
  float mazeH = 200.0f;
  float startX = 100.0f;
  float startY = 100.0f;

  // Targets
  TargetZone targets[MAX_TARGETS];
  uint8_t targetCount = 3;
  float targetRadius = 12.0f;
  bool requireOrder = false;

  // Hardware
  uint8_t lockPin = 10;
  uint8_t neoCount = 30;
  uint8_t volume = 128;

  // Audio
  char sfxTargetHit[24] = "/sfx/target.mp3";
  char sfxSolved[24]    = "/sfx/solved.mp3";
  char sfxStart[24]     = "/sfx/start.mp3";
} config;

static void generateDefaultTargets() {
  float minDist = 40.0f;
  for (uint8_t i = 0; i < config.targetCount; i++) {
    bool ok = false;
    for (int att = 0; att < 50 && !ok; att++) {
      float x = random(15, (int)(config.mazeW - 15));
      float y = random(15, (int)(config.mazeH - 15));
      ok = true;
      float ds = sqrt(pow(x - config.startX, 2) + pow(y - config.startY, 2));
      if (ds < minDist * 2) ok = false;
      for (uint8_t j = 0; j < i && ok; j++) {
        float dt = sqrt(pow(x - config.targets[j].cx, 2) + pow(y - config.targets[j].cy, 2));
        if (dt < minDist * 2) ok = false;
      }
      if (ok) {
        config.targets[i].cx = x;
        config.targets[i].cy = y;
        config.targets[i].radius = config.targetRadius;
        config.targets[i].hit = false;
      }
    }
  }
}

static void loadConfig() {
  if (!LittleFS.exists("/ballmaze.json")) {
    Serial.println(F("[CONFIG] /ballmaze.json not found, using default"));
    generateDefaultTargets();
    return;
  }
  File f = LittleFS.open("/ballmaze.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[CONFIG] JSON error: %s\n", err.c_str());
    generateDefaultTargets();
    return;
  }

  // Physics
  config.accelScale = doc["phys"]["accel"] | config.accelScale;
  config.damping    = doc["phys"]["damp"]  | config.damping;
  config.bounce     = doc["phys"]["bounce"] | config.bounce;
  config.maxSpeed   = doc["phys"]["maxSpd"] | config.maxSpeed;
  config.deadZone   = doc["phys"]["deadZ"]  | config.deadZone;

  // Maze
  config.mazeW = doc["maze"]["w"] | config.mazeW;
  config.mazeH = doc["maze"]["h"] | config.mazeH;
  config.startX = doc["startX"] | config.startX;
  config.startY = doc["startY"] | config.startY;

  // Targets
  config.requireOrder = doc["requireOrder"] | config.requireOrder;
  config.targetRadius = doc["targetRadius"] | config.targetRadius;

  if (doc["targets"].is<JsonArray>()) {
    auto arr = doc["targets"].as<JsonArray>();
    config.targetCount = min((uint8_t)arr.size(), (uint8_t)MAX_TARGETS);
    for (uint8_t i = 0; i < config.targetCount; i++) {
      config.targets[i].cx = arr[i]["x"] | (15.0f + i * 40);
      config.targets[i].cy = arr[i]["y"] | (15.0f + i * 40);
      config.targets[i].radius = arr[i]["r"] | config.targetRadius;
      config.targets[i].hit = false;
    }
  } else {
    config.targetCount = doc["targetCount"] | config.targetCount;
    if (config.targetCount > MAX_TARGETS) config.targetCount = MAX_TARGETS;
    generateDefaultTargets();
  }

  // Hardware
  config.lockPin  = doc["lockPin"]  | config.lockPin;
  config.neoCount = doc["neoCount"] | config.neoCount;
  config.volume   = doc["volume"]   | config.volume;

  // Audio
  strncpy(config.sfxTargetHit, doc["audio"]["target"] | config.sfxTargetHit, sizeof(config.sfxTargetHit));
  strncpy(config.sfxSolved,    doc["audio"]["solved"] | config.sfxSolved,   sizeof(config.sfxSolved));
  strncpy(config.sfxStart,     doc["audio"]["start"]  | config.sfxStart,    sizeof(config.sfxStart));

  Serial.printf("[CONFIG] Maze: %.0fx%.0f, targets: %d, lockPin: %d\n",
                config.mazeW, config.mazeH, config.targetCount, config.lockPin);
}

// ============================================================================
// ACCELEROMETER CALIBRATION
// ============================================================================
struct AccelCalibration {
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  float offsetZ = 9.81f;
  bool done = false;
};

static AccelCalibration cal;

static void calibrateAccelerometer() {
  float sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 15; i++) {
    sensors_event_t ev;
    hw.accelerometer().getEvent(&ev);
    sx += ev.acceleration.x;
    sy += ev.acceleration.y;
    sz += ev.acceleration.z;
    delay(20);
  }
  cal.offsetX = sx / 15.0f;
  cal.offsetY = sy / 15.0f;
  cal.offsetZ = sz / 15.0f;
  cal.done = true;
  Serial.printf("[CALIB] offsetX=%.2f offsetY=%.2f offsetZ=%.2f\n",
                cal.offsetX, cal.offsetY, cal.offsetZ);
}

// ============================================================================
// DISPLAY WRAPPER
// ============================================================================
class DisplayWrapper {
public:
  bool checkDisplay() { return hw.hasDisplay(); }

  void printNumber(int num) {
    if (checkDisplay()) hw.display().printNum(num);
    else Serial.printf("[DISPLAY] Numero: %d\n", num);
  }

  void printString(const char* str) {
    if (checkDisplay()) {
      hw.display().clear();
      hw.display().printString(str);
    } else Serial.printf("[DISPLAY] Texto: %s\n", str);
  }

  void setBrightness(uint8_t b) {
    if (checkDisplay()) hw.display().setBrightness(b);
  }
};
DisplayWrapper display;

// ============================================================================
// PUZZLE
// ============================================================================
class BallMazePuzzle : public PuzzleBase {
public:
  BallMazePuzzle() : PuzzleBase(20) {}

  void setupHardware() override {
    loadConfig();

    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);

    if (config.neoCount > 0) {
      setPixelRange(0, config.neoCount);
    }

    display.setBrightness(15);
    display.printString("BALL");
    hw.setLightAll(pixels()->Color(0, 0, 20));

    // Initialize physics
    resetBall();

    // Calibrate accelerometer
    if (hw.hasAccelerometer()) {
      calibrateAccelerometer();
      Serial.println(F("[BALLMAZE] Accelerometer calibrated"));
    } else {
      Serial.println(F("[BALLMAZE] ERROR: Accelerometer not detected"));
      setState(PuzzleState::Error);
      return;
    }

    geraAlvos();

    Serial.printf("[BALLMAZE] Ready! Tilt to move the ball (%d targets)\n", config.targetCount);
  }

  void update() override {
    hw.sound().loop();
    if (!pixels() || !hw.hasAccelerometer()) return;

    switch (getState()) {
      case PuzzleState::Idle:
        updateIdle();
        break;
      case PuzzleState::Running:
        updateRunning();
        break;
      case PuzzleState::Solved:
        updateSolved();
        break;
      case PuzzleState::Error:
        updateError();
        break;
      default:
        break;
    }
  }

private:
  // =======================================================================
  // BALL STATE
  // =======================================================================
  float ballX_, ballY_;
  float velX_, velY_;
  bool ballMoving_ = false;
  bool audioStarted_ = false;

  // =======================================================================
  // SOLVED STATE
  // =======================================================================
  unsigned long solvedAt_ = 0;

  // =======================================================================
  // IDLE STATE (periodic chime)
  // =======================================================================
  unsigned long lastIdleChime_ = 0;

  // =======================================================================
  // PHYSICS TIMESTEP
  // =======================================================================
  unsigned long lastPhysicsUs_ = 0;

  void resetBall() {
    ballX_ = config.startX;
    ballY_ = config.startY;
    velX_ = 0;
    velY_ = 0;
    ballMoving_ = false;
  }

  void geraAlvos() {
    if (config.targetCount == 0) {
      config.targetCount = 3;
      generateDefaultTargets();
    }
  }

  void resetAlvos() {
    for (uint8_t i = 0; i < config.targetCount; i++) {
      config.targets[i].hit = false;
    }
  }

  // =======================================================================
  // ACCELEROMETER READING
  // =======================================================================
  void readTilt(float& tx, float& ty) {
    sensors_event_t ev;
    hw.accelerometer().getEvent(&ev);

    float ax = ev.acceleration.x - cal.offsetX;
    float ay = ev.acceleration.y - cal.offsetY;

    // Dead zone
    if (fabs(ax) < config.deadZone) ax = 0;
    if (fabs(ay) < config.deadZone) ay = 0;

    // Normalize to -1..1 (considering that max = 9.81 on an axis at 90°)
    tx = constrain(ax / GRAVITY_MS2, -1.0f, 1.0f);
    ty = constrain(ay / GRAVITY_MS2, -1.0f, 1.0f);
  }

  // =======================================================================
  // PHYSICS (Euler)
  // =======================================================================
  void updatePhysics(float dt, float tiltX, float tiltY) {
    // Acceleration from the tilt
    float ax = tiltX * config.accelScale;
    float ay = tiltY * config.accelScale;

    // Integrate velocity
    velX_ += ax * dt;
    velY_ += ay * dt;

    // Damping
    velX_ *= (1.0f - config.damping * dt);
    velY_ *= (1.0f - config.damping * dt);

    // Terminal velocity
    float spd = sqrt(velX_ * velX_ + velY_ * velY_);
    if (spd > config.maxSpeed) {
      velX_ = (velX_ / spd) * config.maxSpeed;
      velY_ = (velY_ / spd) * config.maxSpeed;
    }

    // Detect whether it is moving
    ballMoving_ = (spd > 5.0f);

    // Integrate position
    ballX_ += velX_ * dt;
    ballY_ += velY_ * dt;

    // Collision with walls (with bounce)
    if (ballX_ < 0) { ballX_ = 0; velX_ = -velX_ * config.bounce; }
    if (ballX_ > config.mazeW) { ballX_ = config.mazeW; velX_ = -velX_ * config.bounce; }
    if (ballY_ < 0) { ballY_ = 0; velY_ = -velY_ * config.bounce; }
    if (ballY_ > config.mazeH) { ballY_ = config.mazeH; velY_ = -velY_ * config.bounce; }

    // Safety against NaN
    if (isnan(ballX_) || isnan(ballY_)) resetBall();
  }

  // =======================================================================
  // TARGET DETECTION
  // =======================================================================
  void checkTargets() {
    for (uint8_t i = 0; i < config.targetCount; i++) {
      if (config.targets[i].hit) continue;
      float dx = ballX_ - config.targets[i].cx;
      float dy = ballY_ - config.targets[i].cy;
      if (sqrt(dx * dx + dy * dy) < config.targets[i].radius) {
        onTargetHit(i);
        return;  // only one target per tick
      }
    }
  }

  void onTargetHit(uint8_t idx) {
    config.targets[idx].hit = true;
    Serial.printf("[BALLMAZE] TARGET %d hit! (%d/%d)\n",
                  idx, countHits(), config.targetCount);
    hw.sound().playOnce(config.sfxTargetHit);

    if (countHits() >= config.targetCount) {
      Serial.println(F("[BALLMAZE] ALL TARGETS! Puzzle solved!"));
      setState(PuzzleState::Solved);
      solvedAt_ = millis();
      hw.digitalWrite(config.lockPin, LOW);
      hw.sound().playOnce(config.sfxSolved);
      display.printString("donE");
    }
  }

  uint8_t countHits() {
    uint8_t n = 0;
    for (uint8_t i = 0; i < config.targetCount; i++) {
      if (config.targets[i].hit) n++;
    }
    return n;
  }

  // =======================================================================
  // 2D -> LED STRIP MAPPING
  // =======================================================================
  uint8_t posToLed(float x, float y) {
    uint8_t cols = pixels()->count() / 2;
    if (cols < 1) cols = 1;
    uint8_t c = (uint8_t)map(x, 0, config.mazeW, 0, cols - 1);
    if (y > config.mazeH / 2.0f) c += cols;  // second row
    if (c >= pixels()->count()) c = pixels()->count() - 1;
    return c;
  }

  // =======================================================================
  // NEOPIXEL RENDERING
  // =======================================================================
  void renderNeoPixels() {
    auto* px = pixels();
    px->clear();

    uint32_t cBall = px->Color(255, 255, 255);
    uint32_t cTarget = px->Color(20, 20, 0);
    uint32_t cTargetHit = px->Color(0, 10, 0);

    // Targets not hit
    for (uint8_t i = 0; i < config.targetCount; i++) {
      if (config.targets[i].hit) continue;
      uint8_t idx = posToLed(config.targets[i].cx, config.targets[i].cy);
      px->setPixel(idx, cTarget);

      // Proximity glow
      float dx = ballX_ - config.targets[i].cx;
      float dy = ballY_ - config.targets[i].cy;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist < config.targets[i].radius * 3) {
        uint8_t g = (uint8_t)map(constrain(dist, 0, config.targets[i].radius * 3),
                                  0, config.targets[i].radius * 3, 60, 10);
        px->setPixel(idx, px->Color(g, g, 0));
      }
    }

    // Targets already hit (dim green)
    for (uint8_t i = 0; i < config.targetCount; i++) {
      if (!config.targets[i].hit) continue;
      px->setPixel(posToLed(config.targets[i].cx, config.targets[i].cy), cTargetHit);
    }

    // Ball (white)
    px->setPixel(posToLed(ballX_, ballY_), cBall);

    px->show();
  }

  // =======================================================================
  // SOLVED ANIMATION (rainbow chase)
  // =======================================================================
  void renderSolvedAnimation() {
    auto* px = pixels();
    unsigned long elapsed = millis() - solvedAt_;
    uint8_t hue = (elapsed / 30) % 256;
    for (uint8_t i = 0; i < px->count(); i++) {
      uint8_t h = (hue + i * 8) % 256;
      px->setPixel(i, px->strip()->ColorHSV((uint16_t)h << 8, 255, 80));
    }
    px->show();
  }

  // =======================================================================
  // UPDATE IDLE
  // =======================================================================
  void updateIdle() {
    unsigned long now = millis();

    // Timestep
    if (lastPhysicsUs_ == 0) lastPhysicsUs_ = now;
    float dt = (now - lastPhysicsUs_) / 1000.0f;
    lastPhysicsUs_ = now;
    if (dt > 0.05f) dt = PHYSICS_DT;

    // Read tilt and update physics (shows the ball even in Idle)
    float tx, ty;
    readTilt(tx, ty);
    updatePhysics(dt, tx, ty);

    renderNeoPixels();

    // Periodic chime in Idle
    if (now - lastIdleChime_ > IDLE_CHIME_INTERVAL) {
      lastIdleChime_ = now;
    }

    // Shake starts the puzzle
    if (hw.isShaking(15.0f)) {
      Serial.println(F("[BALLMAZE] Shake detected -> starting"));
      startGame();
    }
  }

  void startGame() {
    resetAlvos();
    resetBall();
    lastPhysicsUs_ = 0;
    hw.sound().playOnce(config.sfxStart);
    display.printString("Go  ");
    setState(PuzzleState::Running);
  }

  // =======================================================================
  // UPDATE RUNNING
  // =======================================================================
  void updateRunning() {
    unsigned long now = millis();

    // Timestep
    if (lastPhysicsUs_ == 0) lastPhysicsUs_ = now;
    float dt = (now - lastPhysicsUs_) / 1000.0f;
    lastPhysicsUs_ = now;
    if (dt > 0.05f) dt = PHYSICS_DT;

    // Read tilt
    float tx, ty;
    readTilt(tx, ty);

    // Update physics
    updatePhysics(dt, tx, ty);

    // Check targets
    checkTargets();

    // Render
    renderNeoPixels();

    // Periodic debug
    static unsigned long lastDbg = 0;
    if (now - lastDbg > 500) {
      lastDbg = now;
      Serial.printf("[BALLMAZE] Ball: (%.0f, %.0f) vel=(%.0f, %.0f) tilt=(%.2f, %.2f) targets=%d/%d\n",
                    ballX_, ballY_, velX_, velY_, tx, ty,
                    countHits(), config.targetCount);
    }

    // Update display with progress
    static uint8_t lastHits = 0;
    uint8_t hits = countHits();
    if (hits != lastHits) {
      lastHits = hits;
      if (hits == 0) display.printString("Go  ");
      else display.printNumber(hits);
    }
  }

  // =======================================================================
  // UPDATE SOLVED
  // =======================================================================
  void updateSolved() {
    renderSolvedAnimation();

    if (millis() - solvedAt_ > 10000) {
      // Auto-reset
      hw.digitalWrite(config.lockPin, HIGH);
      resetBall();
      resetAlvos();
      lastPhysicsUs_ = 0;
      display.printString("BALL");
      setState(PuzzleState::Idle);
      Serial.println(F("[BALLMAZE] Auto-reset -> Idle"));
    }
  }

  // =======================================================================
  // UPDATE ERROR
  // =======================================================================
  void updateError() {
    // Blink red
    unsigned long now = millis();
    if ((now / 500) % 2 == 0) {
      hw.setLightAll(pixels()->Color(50, 0, 0));
    } else {
      hw.setLightAll(0);
    }
  }
};

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(20);
BallMazePuzzle puzzle;

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);
  hw.begin();

  master.addPuzzle(&puzzle);
  master.begin();
}

void loop() {
  loopHA();
  master.update();
}
