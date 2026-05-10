#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>
#include <math.h>

/* =========================
 *  画面・入出力設定
 * ========================= */
#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);

/* ボタン（内部プルアップ：押下=LOW） */
const int PIN_BTN_JUMP  = 5;  // D5: タイトルで開始 / 射撃で上昇
const int PIN_BTN_DASH  = 6;  // D6: タイトルで切替 / 射撃
/* ジャンピングゲーム専用ボタン */
const int PIN_JUMPING_LEFT  = A3; // A3: ジャンプで左移動
const int PIN_JUMPING_RIGHT = 5;  // D5: ジャンプで右移動

/* =========================
 *  プレイヤー（20×20ビットマップ）
 * ========================= */
 /*Default
  0x1f,0xff,0x80,0x20,0x00,0x40,0x2a,0x8a,0xc0,0x20,0x00,0x40,0x28,0x00,0xc0,0x23,
  0xcf,0x40,0x21,0x4a,0x40,0x20,0x84,0x40,0x2c,0x01,0xc0,0x2c,0x39,0xc0,0x60,0x00,
  0x70,0x60,0x00,0x50,0xae,0x92,0xd0,0xaa,0x10,0x50,0xee,0x92,0xf0,0xde,0x10,0x70,
  0x1f,0xff,0x80,0x01,0x02,0x00,0x03,0x03,0x00,0x07,0x03,0x80
*/

  // ｽﾀｯｸﾁｬﾝ
  //※ｽﾀｯｸﾁｬﾝはししかわ様が開発、公開している、 手乗りサイズのｽｰﾊﾟｰｶﾜｲｲコミュニケーションロボットです。 
  const uint8_t PLAYER_BMP[] PROGMEM = {
    0x3f, 0xfe, 0x00, 0x60, 0x01, 0xc0, 0xd0, 0x00, 0x20, 0x8f, 0xff, 0xf0, 
    0x8a, 0x00, 0x10, 0x8a, 0xff, 0xd0, 0x8a, 0xff, 0xd0, 0x8a, 0xee, 0xd0, 
    0x8a, 0xff, 0xd0, 0x8a, 0xf1, 0xd0, 0x8a, 0xff, 0xd0, 0x8a, 0xff, 0xd0, 
    0x8a, 0x92, 0x50, 0x4a, 0xff, 0xd0, 0x2a, 0x00, 0x10, 0x1f, 0xff, 0xf0, 
    0x01, 0x09, 0x00, 0x03, 0x08, 0x80, 0x06, 0x3c, 0x40, 0x07, 0xe7, 0xc0
  };

/* =========================
 *  ボス（40×40ビットマップ）
 * ========================= */
const uint8_t PROGMEM BOSS_BMP[200] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x02,
  0x00,0x80,0x00,0x00,0x04,0x1c,0x40,0x00,0x00,0x0c,0x0c,0x40,0x00,0x00,0x08,0x00,
  0x30,0x00,0x00,0x08,0x00,0x18,0x00,0x00,0x10,0x00,0x0f,0xf8,0x00,0x10,0x00,0x00,
  0x0e,0x00,0x10,0x00,0x00,0x01,0x00,0x1f,0xe0,0x00,0x02,0x80,0x11,0xb0,0x00,0x3e,
  0x40,0x13,0x98,0x00,0xe4,0x20,0x17,0xc6,0x07,0xe4,0x20,0x1d,0xc9,0x0b,0x84,0x20,
  0x1f,0xb8,0x85,0xc8,0x20,0x0f,0xfc,0x42,0x18,0x10,0x0f,0xfc,0x61,0xe0,0x10,0x0f,
  0xff,0xd0,0x04,0x10,0x07,0xff,0xd8,0x08,0x10,0x01,0xff,0xc8,0x00,0x10,0x00,0x3f,
  0xc8,0x00,0x10,0x07,0x67,0xf8,0x3c,0x10,0x05,0xc7,0x9c,0x44,0x10,0x04,0x83,0x8f,
  0xaa,0x10,0x3d,0x03,0x87,0x92,0x10,0x25,0x01,0x88,0xab,0xa0,0x26,0x00,0x98,0x6c,
  0x60,0x24,0x07,0xe0,0x38,0x42,0x1c,0x04,0xc0,0x00,0x42,0x18,0x04,0x40,0x00,0x92,
  0x08,0x04,0x80,0x01,0x10,0x08,0x3f,0x00,0x02,0x24,0x08,0x26,0x00,0x7c,0x4c,0x0c,
  0x24,0x00,0xc1,0x99,0x06,0x28,0x03,0x80,0x33,0x01,0xe0,0x1e,0x00,0x66,0x00,0x3f,
  0xe0,0x00,0x88,0x00,0x00,0x00,0x00,0x00
};
#define BOSS_W 40
#define BOSS_H 40

/* =========================
 *  ブザー音（D9=OC1A / Timer1）
 * ========================= */
struct Beep { uint16_t freq; uint16_t ms; };
bool     BUZZER_ENABLED   = true;
int8_t   BUZZER_SEMITONES = 0;

static volatile uint32_t g_matches_left = 0;
static volatile bool     g_running      = false;

ISR(TIMER1_COMPA_vect){
  if (!g_running) return;
  if (g_matches_left > 0) {
    if (--g_matches_left == 0) {
      TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
      digitalWrite(9, LOW);
      g_running = false;
    }
  }
}
void buzzerPlayRaw(uint32_t freq, uint32_t dur_ms){
  if (freq == 0 || dur_ms == 0) return;
  pinMode(9, OUTPUT);
  TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
  uint32_t ocr; uint16_t prescBits; uint32_t presc;
  if ((ocr = (F_CPU/(2UL*1*freq))-1)      <= 65535) { prescBits=_BV(CS10);            presc=1; }
  else if ((ocr=(F_CPU/(2UL*8*freq))-1)   <= 65535) { prescBits=_BV(CS11);            presc=8; }
  else if ((ocr=(F_CPU/(2UL*64*freq))-1)  <= 65535) { prescBits=_BV(CS11)|_BV(CS10);  presc=64; }
  else if ((ocr=(F_CPU/(2UL*256*freq))-1) <= 65535) { prescBits=_BV(CS12);            presc=256; }
  else                                            { ocr=(F_CPU/(2UL*1024*freq))-1; prescBits=_BV(CS12)|_BV(CS10); presc=1024; }
  OCR1A = (uint16_t)ocr;
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(WGM12)|prescBits;
  uint64_t match_rate = (uint64_t)F_CPU / ((uint64_t)presc * ((uint64_t)OCR1A + 1));
  uint64_t matches = (match_rate * dur_ms) / 1000ULL;
  if (matches == 0) matches = 1;
  noInterrupts();
  g_matches_left = (uint32_t)matches;
  g_running = true;
  TIMSK1 = _BV(OCIE1A);
  interrupts();
}
static inline uint16_t transpose(uint16_t baseHz, int8_t semis){
  if (semis == 0) return baseHz;
  float f = (float)baseHz * powf(2.0f, semis / 12.0f);
  if (f < 1.0f) f = 1.0f; if (f > 20000.0f) f = 20000.0f;
  return (uint16_t)(f + 0.5f);
}
void playBeep(const Beep& s){
  if (!BUZZER_ENABLED) return;
  buzzerPlayRaw(transpose(s.freq, BUZZER_SEMITONES), s.ms);
}

/* 効果音（共通） */
Beep SND_SELECT   = { 900,  70};
Beep SND_START    = {1200, 90};
Beep SND_OK       = {1100, 60};
Beep SND_CANCEL   = { 400,120};
Beep SND_GAMEOVER = { 220,400};
Beep SND_RESTART  = {1000,100};

/* =========================
 *  入力エッジ検出
 * ========================= */
bool btnJumpPrev=false, btnDashPrev=false, btnSpecialPrev=false;
bool jumpingBtnLeftPrev=false, jumpingBtnRightPrev=false;
static inline bool btnEdge(int pin, bool &prev){
  bool now=(digitalRead(pin)==LOW); bool edge=(now && !prev); prev=now; return edge;
}

/* ★★★ 入力ロック時刻 ★★★ */
uint32_t shootingInputLockUntil = 0;

/* =========================
 *  共通HUD（中央寄せ文字描画）
 * ========================= */
void drawCenteredText(int16_t y, const __FlashStringHelper* txt, uint8_t size=1){
  display.setTextSize(size);
  display.setTextWrap(false);
  const char* p = (const char*)txt;
  uint16_t len = strlen_P(p);
  int16_t w = (int16_t)(len * 6 * size);
  int16_t x = (OLED_WIDTH - w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(txt);
}

/* =========================
 *  画面反転制御（ライフ0演出用）
 * ========================= */
bool g_displayInverted = true;  // ※このプロジェクトでは true を「通常表示」として扱う
static inline void setInvert(bool on){
  if (on != g_displayInverted) { display.invertDisplay(on); g_displayInverted = on; }
}

/* =========================
 *  タイトル
 * ========================= */
enum Mode : uint8_t { MODE_TITLE=0, MODE_SHOOTING, MODE_JUMPING };
Mode mode = MODE_TITLE;
uint8_t titleSelection = 0;
void resetTitle(){ display.setRotation(0); mode = MODE_TITLE; titleSelection = 0; }

/* =========================
 *  シューティング
 * ========================= */
const int   SHOOTING_PLAYER_X = 12;
const int   SHOOTING_PLAYER_W = 20;
const int   SHOOTING_PLAYER_H = 20;
const float SHOOTING_GRAVITY  = 0.7f;
const float SHOOTING_THRUST   = -2.6f;
const float SHOOTING_FRICTION = 0.98f;

const uint8_t SHOOTING_MAX_BULLETS    = 5;  // RAM節約のため削減 (8→5)
const uint8_t SHOOTING_MAX_ENEMIES    = 4;  // RAM節約のため削減 (6→4)
const uint8_t SHOOTING_MAX_BOSS_SHOTS = 5;  // RAM節約のため削減 (8→5)
const uint8_t SHOOTING_BULLET_W = 3, SHOOTING_BULLET_H = 3;
const float   SHOOTING_BULLET_VX = 3.2f;
const float   SHOOTING_ENEMY_BASE_SPEED = 1.2f;
const float   SHOOTING_ENEMY_SCORE_SPEED_COEF = 0.8f;

const uint16_t SHOOTING_SPAWN_BASE_INTERVAL_MS = 700;
const int16_t  SHOOTING_SPAWN_DEC_PER_STAGE_MS = 50;
const uint16_t SHOOTING_SPAWN_MIN_INTERVAL_MS  = 200;
const float    SHOOTING_SPEEDMUL_PER_STAGE     = 0.15f;

const int      SHOOTING_EXTRA_ENEMY_PER_STAGE_PC = 15;
const int      SHOOTING_EXTRA_ENEMY_MAX_PC       = 70;

const uint16_t SHOOTING_BOSS_SPAWN_SCORE       = 2500;
const uint8_t  SHOOTING_BOSS_HP                = 30;
const float    SHOOTING_BOSS_VY                = 0.7f;
const int16_t  SHOOTING_BOSS_ENTRY_OFFSET_X    = 25;
const int16_t  SHOOTING_BOSS_HOLD_X            = 90;
const int16_t  SHOOTING_BOSS_MIN_Y             = 1;
const int16_t  SHOOTING_BOSS_MARGIN_BOTTOM     = 1;
const uint8_t  SHOOTING_BOSS_SHOT_W = 6, SHOOTING_BOSS_SHOT_H = 6;
const float    SHOOTING_BOSS_SHOT_VX_BASE       = 1.8f;
const float    SHOOTING_BOSS_SHOT_VX_STAGE_COEF = 0.15f;
const uint16_t SHOOTING_BOSS_SHOT_BASE_INTERVAL_MS = 1500;
const int16_t  SHOOTING_BOSS_SHOT_DEC_PER_STAGE_MS = 200;
const uint16_t SHOOTING_BOSS_SHOT_MIN_INTERVAL_MS  = 800;

const uint8_t SHOOTING_LIVES_MAX = 3;
uint8_t shootingLives = SHOOTING_LIVES_MAX;
bool shootingHardOver = false;

Beep SND_SHOT     = {1300, 40};
Beep SND_HIT      = { 900, 70};
Beep SND_BOSS_HIT = {1000, 40};
Beep SND_BOSS_DIE = {1600,120};
Beep SND_SPECIAL  = {1800, 60};

// 特殊攻撃チャージ段階（発射時刻からの経過で決まる）
// 3s+: ビーム×0.7  5s+: ビーム×1.2  7s+: ビーム×1.2 × 2連射
uint32_t shootingSpecialLastFiredMs = 0; // millis()で初期化される

Beep SND_JUMPING_LEVELUP = {1800, 80};

struct Bullet   { float x,y,vx; uint8_t w,h; bool alive; };
struct Enemy    { float x,y; uint8_t w,h; bool alive; };
struct Boss     { float x,y; float vy; uint8_t w,h; uint8_t hp; bool alive; bool entering; };
struct BossShot { float x,y,vx; uint8_t w,h; bool alive; };

float   shootingPlayerY=OLED_HEIGHT/2, shootingVelocityY=0;
Bullet  shootingBullets[SHOOTING_MAX_BULLETS];
Enemy   shootingEnemies[SHOOTING_MAX_ENEMIES];
Boss    shootingBoss;
BossShot shootingBossShots[SHOOTING_MAX_BOSS_SHOTS];
bool    shootingBossSpawned=false;
bool    shootingBossDefeated=false;
bool    shootingClear=false;

uint32_t shootingLastSpawn=0;
uint32_t shootingScore=0;
bool     shootingGameOver=false;
bool     shootingGameOverSoundPlayed=false;

uint8_t  shootingStage=1;
uint16_t shootingSpawnInterval=SHOOTING_SPAWN_BASE_INTERVAL_MS;
float    shootingSpeedMul=1.0f;
uint16_t shootingBossSpawnScore=SHOOTING_BOSS_SPAWN_SCORE;
uint32_t shootingNextBossSpawnAt=SHOOTING_BOSS_SPAWN_SCORE;
uint32_t shootingStageScoreBase = 0;

uint32_t shootingBossLastShotMs=0;
uint16_t shootingBossShotInterval=SHOOTING_BOSS_SHOT_BASE_INTERVAL_MS;

static inline uint8_t countAliveEnemies(){ uint8_t c=0; for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++) if (shootingEnemies[i].alive) c++; return c; }

void applyShootingStageParams(){
  shootingSpeedMul = 1.0f + SHOOTING_SPEEDMUL_PER_STAGE * shootingStage;

  int16_t candidateSpawn = (int16_t)SHOOTING_SPAWN_BASE_INTERVAL_MS - SHOOTING_SPAWN_DEC_PER_STAGE_MS * (int16_t)shootingStage;
  shootingSpawnInterval = (candidateSpawn < (int16_t)SHOOTING_SPAWN_MIN_INTERVAL_MS) ? SHOOTING_SPAWN_MIN_INTERVAL_MS : (uint16_t)candidateSpawn;

  int16_t candidateBossShot = (int16_t)SHOOTING_BOSS_SHOT_BASE_INTERVAL_MS - SHOOTING_BOSS_SHOT_DEC_PER_STAGE_MS * (int16_t)shootingStage;
  shootingBossShotInterval = (candidateBossShot < (int16_t)SHOOTING_BOSS_SHOT_MIN_INTERVAL_MS) ? SHOOTING_BOSS_SHOT_MIN_INTERVAL_MS : (uint16_t)candidateBossShot;
}

void advanceShootingStage(){
  shootingStage += 1;
  applyShootingStageParams();

  shootingClear = false;
  shootingGameOver = false;
  shootingGameOverSoundPlayed = false;
  shootingHardOver = false;

  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++) shootingBullets[i]={0,0,0,SHOOTING_BULLET_W,SHOOTING_BULLET_H,false};
  for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++) shootingEnemies[i]={0,0,0,0,false};
  for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++) shootingBossShots[i]={0,0,0,0,0,false};

  shootingNextBossSpawnAt = shootingScore + shootingBossSpawnScore;
  shootingStageScoreBase  = shootingScore;

  shootingBoss = { (float)OLED_WIDTH + (float)SHOOTING_BOSS_ENTRY_OFFSET_X, 12.0f, SHOOTING_BOSS_VY, BOSS_W, BOSS_H, SHOOTING_BOSS_HP, false, false };
  shootingBossSpawned=false;
  shootingBossDefeated=false;

  shootingLastSpawn = millis();
  shootingBossLastShotMs = millis();
  shootingInputLockUntil = 0;
  shootingSpecialLastFiredMs = millis();

  setInvert(true);
  playBeep(SND_START);
}

void resetShootingGame(){
  shootingPlayerY = (OLED_HEIGHT - SHOOTING_PLAYER_H)/2; shootingVelocityY = 0; shootingScore=0;
  shootingGameOver=false; shootingGameOverSoundPlayed=false; shootingClear=false; shootingHardOver=false;

  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++) shootingBullets[i]={0,0,0,SHOOTING_BULLET_W,SHOOTING_BULLET_H,false};
  for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++) shootingEnemies[i]={0,0,0,0,false};
  for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++) shootingBossShots[i]={0,0,0,0,0,false};

  shootingStage = 1;
  applyShootingStageParams();

  shootingNextBossSpawnAt = shootingBossSpawnScore;
  shootingStageScoreBase  = 0;

  shootingBoss = { (float)OLED_WIDTH + (float)SHOOTING_BOSS_ENTRY_OFFSET_X, 12.0f, SHOOTING_BOSS_VY, BOSS_W, BOSS_H, SHOOTING_BOSS_HP, false, false };
  shootingBossSpawned=false; shootingBossDefeated=false;

  shootingLastSpawn = millis();
  shootingBossLastShotMs = millis();

  shootingInputLockUntil = 0;
  shootingSpecialLastFiredMs = millis();

  shootingLives = SHOOTING_LIVES_MAX;
  setInvert(true);
}

void commitShootingDeath(){
  if (!shootingGameOver){
    shootingGameOver = true;
    if (!shootingGameOverSoundPlayed){ playBeep(SND_GAMEOVER); shootingGameOverSoundPlayed=true; }
    if (shootingLives > 0) shootingLives--;
    if (shootingLives == 0){
      shootingHardOver = true;
      setInvert(false);
    }else{
      shootingHardOver = false;
    }
    shootingInputLockUntil = millis() + 500; btnJumpPrev = false; btnDashPrev = false;
  }
}

void spawnEnemy(){
  for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++){
    if (!shootingEnemies[i].alive){
      shootingEnemies[i].alive=true;
      shootingEnemies[i].x = OLED_WIDTH + random(0,25);
      shootingEnemies[i].y = 2 + random(0, OLED_HEIGHT - (2 + 8));
      shootingEnemies[i].w = 8 + random(0,6);
      shootingEnemies[i].h = 6 + random(0,8);
      return;
    }
  }
}
void firePlayerBullet(){
  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++){
    if (!shootingBullets[i].alive){
      shootingBullets[i].alive=true;
      shootingBullets[i].w = SHOOTING_BULLET_W; shootingBullets[i].h = SHOOTING_BULLET_H;
      shootingBullets[i].x = SHOOTING_PLAYER_X + SHOOTING_PLAYER_W;
      shootingBullets[i].y = shootingPlayerY + (SHOOTING_PLAYER_H/2) - (shootingBullets[i].h/2);
      shootingBullets[i].vx= SHOOTING_BULLET_VX;
      playBeep(SND_SHOT);
      return;
    }
  }
}
void fireSpecialBullets(){
  uint32_t elapsed = millis() - shootingSpecialLastFiredMs;
  if (elapsed < 2000UL) return;

  const uint8_t BASE_H = 27;
  uint8_t beamH;
  uint8_t count;
  if (elapsed >= 6000UL){
    beamH = (uint8_t)(BASE_H * 1.2f + 0.5f); // ~32px
    count = 2;
  } else if (elapsed >= 4000UL){
    beamH = (uint8_t)(BASE_H * 1.2f + 0.5f); // ~32px
    count = 1;
  } else {
    beamH = (uint8_t)(BASE_H * 0.7f + 0.5f); // ~19px
    count = 1;
  }
  if (beamH > OLED_HEIGHT) beamH = OLED_HEIGHT;

  float fy = shootingPlayerY + SHOOTING_PLAYER_H / 2.0f - beamH / 2.0f;
  if (fy < 0) fy = 0;
  if (fy > OLED_HEIGHT - beamH) fy = OLED_HEIGHT - beamH;

  // 空きスロットを事前収集し、count分揃っている場合のみ発射
  uint8_t slots[2]; uint8_t nSlots = 0;
  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS && nSlots<count;i++){
    if (!shootingBullets[i].alive) slots[nSlots++] = i;
  }
  if (nSlots < count) return; // スロット不足なら発射しない

  for(uint8_t f=0;f<count;f++){
    uint8_t i = slots[f];
    shootingBullets[i].alive = true;
    shootingBullets[i].w = SHOOTING_BULLET_W;
    shootingBullets[i].h = beamH;
    shootingBullets[i].x = (float)(SHOOTING_PLAYER_X + SHOOTING_PLAYER_W) + f * 8.0f;
    shootingBullets[i].y = fy;
    shootingBullets[i].vx = SHOOTING_BULLET_VX;
  }
  playBeep(SND_SPECIAL);
  shootingSpecialLastFiredMs = millis();
}
void fireBossBullet(){
  for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++){
    if (!shootingBossShots[i].alive){
      shootingBossShots[i].alive = true;
      shootingBossShots[i].w = SHOOTING_BOSS_SHOT_W; shootingBossShots[i].h = SHOOTING_BOSS_SHOT_H;
      int8_t dy = random(-10, 11);
      shootingBossShots[i].x = shootingBoss.x - 2;
      shootingBossShots[i].y = shootingBoss.y + (BOSS_H/2) + dy - (shootingBossShots[i].h/2);
      if (shootingBossShots[i].y < 0) shootingBossShots[i].y = 0;
      if (shootingBossShots[i].y > OLED_HEIGHT - shootingBossShots[i].h) shootingBossShots[i].y = OLED_HEIGHT - shootingBossShots[i].h;
      shootingBossShots[i].vx = -(SHOOTING_BOSS_SHOT_VX_BASE + SHOOTING_BOSS_SHOT_VX_STAGE_COEF * shootingStage);
      return;
    }
  }
}

void updateShootingGame(){
  bool thrust = (digitalRead(PIN_BTN_JUMP)==LOW);
  bool fireEdge = btnEdge(PIN_BTN_DASH, btnDashPrev);
  bool specialEdge = btnEdge(PIN_JUMPING_LEFT, btnSpecialPrev);

  if (shootingClear || shootingGameOver) {
    if (millis() < shootingInputLockUntil) return;

    if (shootingClear){
      if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) { advanceShootingStage(); }
      if (fireEdge) { playBeep(SND_CANCEL); setInvert(true); resetTitle(); mode = MODE_TITLE; }
    }else{
      if (shootingHardOver){
        if (fireEdge) {
          playBeep(SND_CANCEL);
          setInvert(true);
          resetTitle(); mode = MODE_TITLE;
        } else if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) {
          playBeep(SND_RESTART);
          resetShootingGame();
        }
      }else{
        if (fireEdge) {
          playBeep(SND_CANCEL);
          setInvert(true);
          resetTitle(); mode = MODE_TITLE;
        }
        if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) {
          playBeep(SND_RESTART);
          shootingPlayerY = (OLED_HEIGHT - SHOOTING_PLAYER_H)/2; shootingVelocityY = 0;
          for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++) shootingBullets[i]={0,0,0,SHOOTING_BULLET_W,SHOOTING_BULLET_H,false};
          for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++) shootingEnemies[i]={0,0,0,0,false};
          for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++) shootingBossShots[i]={0,0,0,0,0,false};
          shootingBossSpawned=false; shootingBossDefeated=false;
          shootingBoss = { (float)OLED_WIDTH + (float)SHOOTING_BOSS_ENTRY_OFFSET_X, 12.0f, SHOOTING_BOSS_VY, BOSS_W, BOSS_H, SHOOTING_BOSS_HP, false, false };
          shootingLastSpawn = millis();
          shootingBossLastShotMs = millis();
          shootingGameOver = false;
          shootingGameOverSoundPlayed = false;
          shootingInputLockUntil = 0;
          setInvert(true);
        }
      }
    }
    return;
  }

  if (thrust) shootingVelocityY += SHOOTING_THRUST;
  shootingVelocityY += SHOOTING_GRAVITY;
  shootingVelocityY *= SHOOTING_FRICTION;
  shootingPlayerY += shootingVelocityY;
  if (shootingPlayerY < 0){ shootingPlayerY=0; shootingVelocityY=0; }
  if (shootingPlayerY > OLED_HEIGHT - SHOOTING_PLAYER_H){ shootingPlayerY = OLED_HEIGHT - SHOOTING_PLAYER_H; shootingVelocityY=0; }

  if (fireEdge) firePlayerBullet();
  if (specialEdge) fireSpecialBullets();

  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++){
    if (!shootingBullets[i].alive) continue;
    shootingBullets[i].x += shootingBullets[i].vx;
    if (shootingBullets[i].x > OLED_WIDTH) shootingBullets[i].alive=false;
  }

  if (!shootingBossSpawned && !shootingBossDefeated && shootingScore >= shootingNextBossSpawnAt){
    shootingBossSpawned = true;
    shootingBoss.alive = true;
    shootingBoss.entering = true;
    shootingBoss.x = OLED_WIDTH + SHOOTING_BOSS_ENTRY_OFFSET_X;
    shootingBoss.y = 12;
    shootingBoss.vy = SHOOTING_BOSS_VY;
    shootingBoss.hp = SHOOTING_BOSS_HP;
  }

  uint32_t now = millis();
  if (!shootingBossSpawned || !shootingBoss.alive){
    if (now - shootingLastSpawn >= shootingSpawnInterval){
      shootingLastSpawn = now;
      if (countAliveEnemies() < SHOOTING_MAX_ENEMIES){
        spawnEnemy();
        int extraChance = SHOOTING_EXTRA_ENEMY_PER_STAGE_PC * (int)shootingStage;
        if (extraChance > SHOOTING_EXTRA_ENEMY_MAX_PC) extraChance = SHOOTING_EXTRA_ENEMY_MAX_PC;
        if (random(0,100) < extraChance && countAliveEnemies() < SHOOTING_MAX_ENEMIES) spawnEnemy();
      }
    }
  }

  for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++){
    if (!shootingEnemies[i].alive) continue;

    float scorePhase = 0.0f;
    if (shootingScore > shootingStageScoreBase) {
      scorePhase = (float)(shootingScore - shootingStageScoreBase) / (float)shootingBossSpawnScore;
      if (scorePhase > 1.0f) scorePhase = 1.0f;
    }
    float vx = SHOOTING_ENEMY_BASE_SPEED * shootingSpeedMul + SHOOTING_ENEMY_SCORE_SPEED_COEF * scorePhase;

    shootingEnemies[i].x -= vx;
    if (shootingEnemies[i].x + shootingEnemies[i].w < 0) shootingEnemies[i].alive=false;

    int px1=SHOOTING_PLAYER_X, py1=(int)shootingPlayerY;
    int px2=px1+SHOOTING_PLAYER_W-1, py2=py1+SHOOTING_PLAYER_H-1;
    int ex1=(int)shootingEnemies[i].x, ey1=(int)shootingEnemies[i].y;
    int ex2=ex1+shootingEnemies[i].w-1, ey2=ey1+shootingEnemies[i].h-1;
    bool hit = !(px2<ex1 || ex2<px1 || py2<ey1 || ey2<py1);
    if (hit){ commitShootingDeath(); return; }

    for(uint8_t b=0;b<SHOOTING_MAX_BULLETS;b++){
      if (!shootingBullets[b].alive) continue;
      int bx1=(int)shootingBullets[b].x, by1=(int)shootingBullets[b].y;
      int bx2=bx1 + shootingBullets[b].w - 1, by2=by1 + shootingBullets[b].h - 1;
      bool bHit = !(bx2<ex1 || ex2<bx1 || by2<ey1 || ey2<by1);
      if (bHit){
        shootingBullets[b].alive=false; shootingEnemies[i].alive=false; shootingScore += 100; playBeep(SND_HIT);
        break;
      }
    }
  }

  if (shootingBossSpawned && shootingBoss.alive){
    if (shootingBoss.entering){
      shootingBoss.x -= 1.2f;
      if (shootingBoss.x <= SHOOTING_BOSS_HOLD_X){ shootingBoss.x = SHOOTING_BOSS_HOLD_X; shootingBoss.entering = false; }
    }else{
      shootingBoss.y += shootingBoss.vy;
      if (shootingBoss.y < SHOOTING_BOSS_MIN_Y){ shootingBoss.y = SHOOTING_BOSS_MIN_Y; shootingBoss.vy = fabs(shootingBoss.vy); }
      if (shootingBoss.y > (OLED_HEIGHT - shootingBoss.h - SHOOTING_BOSS_MARGIN_BOTTOM)){ shootingBoss.y = (OLED_HEIGHT - shootingBoss.h - SHOOTING_BOSS_MARGIN_BOTTOM); shootingBoss.vy = -fabs(shootingBoss.vy); }

      if (now - shootingBossLastShotMs >= shootingBossShotInterval){
        shootingBossLastShotMs = now;
        fireBossBullet();
        if (shootingStage >= 3) fireBossBullet();
      }
    }

    int px1=SHOOTING_PLAYER_X, py1=(int)shootingPlayerY;
    int px2=px1+SHOOTING_PLAYER_W-1, py2=py1+SHOOTING_PLAYER_H-1;
    int bx1=(int)shootingBoss.x, by1=(int)shootingBoss.y;
    int bx2=bx1+shootingBoss.w-1, by2=by1+shootingBoss.h-1;
    bool bossHit = !(px2<bx1 || bx2<px1 || py2<by1 || by2<py1);
    if (bossHit){ commitShootingDeath(); return; }

    for(uint8_t b=0;b<SHOOTING_MAX_BULLETS;b++){
      if (!shootingBullets[b].alive) continue;
      int ex1=(int)shootingBullets[b].x, ey1=(int)shootingBullets[b].y;
      int ex2=ex1 + shootingBullets[b].w - 1, ey2=ey1 + shootingBullets[b].h - 1;
      bool bHit = !(ex2<bx1 || bx2<ex1 || ey2<by1 || by2<ey1);
      if (bHit){
        shootingBullets[b].alive=false;
        if (shootingBoss.hp > 0) shootingBoss.hp--;
        shootingScore += 120;
        playBeep(SND_BOSS_HIT);
        if (shootingBoss.hp == 0){
          shootingBoss.alive=false;
          shootingBossDefeated=true;
          playBeep(SND_BOSS_DIE);
          shootingScore += 2000;
          shootingClear = true;
          shootingInputLockUntil = millis() + 500; btnJumpPrev = false; btnDashPrev = false;
        }
        break;
      }
    }
  }

  for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++){
    if (!shootingBossShots[i].alive) continue;
    shootingBossShots[i].x += shootingBossShots[i].vx;
    if (shootingBossShots[i].x + shootingBossShots[i].w < 0) { shootingBossShots[i].alive=false; continue; }

    int px1=SHOOTING_PLAYER_X, py1=(int)shootingPlayerY;
    int px2=px1+SHOOTING_PLAYER_W-1, py2=py1+SHOOTING_PLAYER_H-1;
    int sx1=(int)shootingBossShots[i].x, sy1=(int)shootingBossShots[i].y;
    int sx2=sx1+shootingBossShots[i].w-1, sy2=sy1+shootingBossShots[i].h-1;
    bool shHit = !(px2<sx1 || sx2<px1 || py2<sy1 || sy2<py1);
    if (shHit){ commitShootingDeath(); return; }
  }
}

void drawShootingGame(){
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  if (shootingGameOver && shootingHardOver){
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(F("ST:")); display.print((int)shootingStage);
    display.setCursor(64,0);
    display.print(F("SC:")); display.print(shootingScore);
    display.setTextSize(2); drawCenteredText(22, F("GAME OVER"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Stage1  R:Title"), 1);
    display.display();
    return;
  }

  int x=SHOOTING_PLAYER_X, y=(int)shootingPlayerY;
  display.drawBitmap(x, y, PLAYER_BMP, SHOOTING_PLAYER_W, SHOOTING_PLAYER_H, SSD1306_WHITE);

  for(uint8_t i=0;i<SHOOTING_MAX_BULLETS;i++){
    if (shootingBullets[i].alive) display.fillRect((int)shootingBullets[i].x, (int)shootingBullets[i].y, shootingBullets[i].w, shootingBullets[i].h, SSD1306_WHITE);
  }

  for(uint8_t i=0;i<SHOOTING_MAX_ENEMIES;i++){
    if (!shootingEnemies[i].alive) continue;
    display.drawRect((int)shootingEnemies[i].x, (int)shootingEnemies[i].y, shootingEnemies[i].w, shootingEnemies[i].h, SSD1306_WHITE);
  }

  if (shootingBossSpawned && shootingBoss.alive){
    display.drawBitmap((int)shootingBoss.x, (int)shootingBoss.y, BOSS_BMP, BOSS_W, BOSS_H, SSD1306_WHITE);
  }

  for(uint8_t i=0;i<SHOOTING_MAX_BOSS_SHOTS;i++){
    if (shootingBossShots[i].alive) display.fillRect((int)shootingBossShots[i].x, (int)shootingBossShots[i].y, shootingBossShots[i].w, shootingBossShots[i].h, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(F("ST:")); display.print((int)shootingStage);
  display.setCursor(44,0);
  display.print(F("SC:")); display.print(shootingScore);
  display.setCursor(96,0);
  display.print(F("LF:")); display.print((int)shootingLives);

  // 特殊攻撃チャージゲージ（右下）3段階
  {
    const int8_t gw=40, gh=3;
    const int8_t gx=OLED_WIDTH-gw-1, gy=OLED_HEIGHT-gh-1;
    uint32_t elapsed2 = millis() - shootingSpecialLastFiredMs;
    if (elapsed2 > 6000UL) elapsed2 = 6000UL;
    int fillW = (int)((gw - 2) * (float)elapsed2 / 6000.0f);
    if (fillW < 0) fillW = 0;
    display.drawRect(gx, gy, gw, gh, SSD1306_WHITE);
    if (fillW > 0) display.fillRect(gx+1, gy+1, fillW, gh-2, SSD1306_WHITE);
    // 段階マーク（2s/4s の位置に縦線）
    const int8_t t1 = (int8_t)((gw-2)*2/6), t2 = (int8_t)((gw-2)*4/6);
    display.drawFastVLine(gx+1+t1, gy, gh, SSD1306_BLACK);
    display.drawFastVLine(gx+1+t2, gy, gh, SSD1306_BLACK);
    display.setCursor(gx - 14, gy-3);
    display.print(F("SP"));
  }

  if (shootingBossSpawned && shootingBoss.alive){
    const int barX = 0, barY = 8, barW = 120, barH = 6;
    display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
    uint8_t hp = shootingBoss.hp; if (hp > SHOOTING_BOSS_HP) hp = SHOOTING_BOSS_HP;
    int fillW = (int)((barW - 2) * ((float)hp / (float)SHOOTING_BOSS_HP) + 0.5f);
    if (fillW < 0) fillW = 0;
    display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
  }

  if (shootingClear){
    display.setTextSize(2); drawCenteredText(22, F("CLEAR!"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Next  R:Title"), 1);
  } else if (shootingGameOver){
    display.setTextSize(2); drawCenteredText(22, F("Miss!"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Retry  R:Title"), 1);
  }

  display.display();
}

/* =========================
 *  ジャンピング
 * ========================= */
const int   JUMPING_PLAYER_W = 20;
const int   JUMPING_PLAYER_H = 20;
const float JUMPING_MOVE_SPEED = 3.2f;
const float JUMPING_LV1_GRAVITY    = 0.4;  // 重力（易）
const float JUMPING_LVMAX_GRAVITY   = 1.7f;  // 重力（難）
const float JUMPING_LV1_JUMP_V     = -6.0f; // ジャンプ速度（易）
const float JUMPING_LVMAX_JUMP_V    = -16.0f;  // ジャンプ速度（難）
const int   JUMPING_SCROLL_TRIGGER_Y = 60;
const uint8_t JUMPING_LIVES_MAX = 3;
const uint8_t JUMPING_PLATFORM_COUNT = 8;  // 縦128px画面を埋めるために増量
const uint8_t JUMPING_PLATFORM_H = 3;
// レベル別難易度パラメータ（Lv1=易, LVMAX=難）
const uint8_t  JUMPING_MAX_LEVEL       = 50;
const uint16_t JUMPING_SCORE_PER_LEVEL = 100;
const uint8_t  JUMPING_LV1_W_MIN       = 14;  // 足場最小幅（易）
const uint8_t  JUMPING_LV1_W_MAX       = 22;  // 足場最大幅（易）
const uint8_t  JUMPING_LV1_GAP_MIN     = 25;  // 足場間隔最小（易）
const uint8_t  JUMPING_LV1_GAP_MAX     = 32;  // 足場間隔最大（易）
const uint8_t  JUMPING_LV1_SPREAD      = 35;  // 横ばらつき（易）
const uint8_t  JUMPING_LVMAX_W_MIN      = 8;   // 足場最小幅（難）
const uint8_t  JUMPING_LVMAX_W_MAX      = 12;;  // 足場最大幅（難）
const uint8_t  JUMPING_LVMAX_GAP_MIN    = 35;  // 足場間隔最小（難）
const uint8_t  JUMPING_LVMAX_GAP_MAX    = 50;  // 足場間隔最大（難）
const uint8_t  JUMPING_LVMAX_SPREAD     = 70;  // 横ばらつき（難）
const float JUMPING_LAND_OVERLAP_MIN = 3.0f; // 着地に必要な足場との重なり量（px）
const int JUMPING_SCREEN_W = 64;   // 縦向き（回転後）の画面幅
const int JUMPING_SCREEN_H = 128;  // 縦向き（回転後）の画面高さ

/* 縦向き（64px幅）用中央寄せ */
void drawCenteredTextNarrow(int16_t y, const __FlashStringHelper* txt, uint8_t size=1){
  display.setTextSize(size);
  display.setTextWrap(false);
  const char* p = (const char*)txt;
  uint16_t len = strlen_P(p);
  int16_t w = (int16_t)(len * 6 * size);
  int16_t x = (JUMPING_SCREEN_W - w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(txt);
}

struct JumpingPlatform { float x, y; uint8_t w; };

JumpingPlatform jumpingPlatforms[JUMPING_PLATFORM_COUNT];
float jumpingPlayerX = 0;
float jumpingPlayerY = 0;
float jumpingVelocityY = 0;
uint32_t jumpingScore = 0;
uint8_t jumpingLives = JUMPING_LIVES_MAX;
bool jumpingGameOver = false;
bool jumpingHardOver = false;
bool jumpingGameOverSoundPlayed = false;
uint32_t jumpingInputLockUntil = 0;
bool jumpingFacingLeft = false;
uint8_t jumpingEdgeStreak = 0;  // 画面端の連続回数
uint8_t jumpingLevel = 1;       // 現在のレベル（スコアから計算）
bool jumpingAirJumpAvailable = false; // 空中ジャンプ使用可能フラグ
uint8_t jumpingAirGauge = 0;          // 空中ジャンプゲージ (0〜5)

// ランタイム難易度パラメータ（レベルに応じて更新）
uint8_t jumpingCurWMin   = JUMPING_LV1_W_MIN;
uint8_t jumpingCurWMax   = JUMPING_LV1_W_MAX;
uint8_t jumpingCurGapMin = JUMPING_LV1_GAP_MIN;
uint8_t jumpingCurGapMax = JUMPING_LV1_GAP_MAX;
uint8_t jumpingCurSpread = JUMPING_LV1_SPREAD;
float   jumpingCurGravity = JUMPING_LV1_GRAVITY;
float   jumpingCurJumpV   = JUMPING_LV1_JUMP_V;

// 画面端とみなすX位置のしきい値（左端: <N  右端: >=W-N）
#define JUMPING_EDGE_MARGIN 12

Beep SND_JUMPING_BOUNCE = {1400, 40};

/* PROGMEMビットマップを左右反転して描画 */
static void drawBitmapFlipH(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color){
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; j++){
    for (int16_t i = 0; i < w; i++){
      int16_t src = w - 1 - i;
      if (pgm_read_byte(bitmap + j * byteWidth + src / 8) & (0x80 >> (src % 8))){
        display.drawPixel(x + i, y + j, color);
      }
    }
  }
}

void computeJumpingLevelParams(){
  uint8_t lv = jumpingLevel;
  if (lv < 1) lv = 1;
  if (lv > JUMPING_MAX_LEVEL) lv = JUMPING_MAX_LEVEL;
  uint8_t t = lv - 1;
  const uint8_t d = JUMPING_MAX_LEVEL - 1;
  jumpingCurWMin   = (uint8_t)((int16_t)JUMPING_LV1_W_MIN   + ((int16_t)JUMPING_LVMAX_W_MIN   - JUMPING_LV1_W_MIN)   * t / d);
  jumpingCurWMax   = (uint8_t)((int16_t)JUMPING_LV1_W_MAX   + ((int16_t)JUMPING_LVMAX_W_MAX   - JUMPING_LV1_W_MAX)   * t / d);
  jumpingCurGapMin = (uint8_t)((int16_t)JUMPING_LV1_GAP_MIN + ((int16_t)JUMPING_LVMAX_GAP_MIN - JUMPING_LV1_GAP_MIN) * t / d);
  jumpingCurGapMax = (uint8_t)((int16_t)JUMPING_LV1_GAP_MAX + ((int16_t)JUMPING_LVMAX_GAP_MAX - JUMPING_LV1_GAP_MAX) * t / d);
  jumpingCurSpread = (uint8_t)((int16_t)JUMPING_LV1_SPREAD  + ((int16_t)JUMPING_LVMAX_SPREAD  - JUMPING_LV1_SPREAD)  * t / d);
  jumpingCurGravity = JUMPING_LV1_GRAVITY + (JUMPING_LVMAX_GRAVITY - JUMPING_LV1_GRAVITY) * t / d;
  jumpingCurJumpV   = JUMPING_LV1_JUMP_V  + (JUMPING_LVMAX_JUMP_V  - JUMPING_LV1_JUMP_V)  * t / d;
}

uint8_t randomJumpingPlatformWidth(){
  return (uint8_t)random(jumpingCurWMin, jumpingCurWMax + 1);
}

int findHighestJumpingPlatformIndex(){
  uint8_t highestIndex = 0;
  for (uint8_t i = 1; i < JUMPING_PLATFORM_COUNT; ++i) {
    if (jumpingPlatforms[i].y < jumpingPlatforms[highestIndex].y) highestIndex = i;
  }
  return highestIndex;
}

void respawnJumpingPlatformAbove(uint8_t index){
  int highestIndex = findHighestJumpingPlatformIndex();
  uint8_t width = randomJumpingPlatformWidth();
  float anchorX = jumpingPlatforms[highestIndex].x;

  // 画面端に張り付いている場合、連続回数をカウント
  bool isEdge = (anchorX < JUMPING_EDGE_MARGIN) || (anchorX >= JUMPING_SCREEN_W - JUMPING_EDGE_MARGIN - width);
  if (isEdge) { jumpingEdgeStreak++; } else { jumpingEdgeStreak = 0; }

  float nextX;
  if (jumpingEdgeStreak >= 3) {
    // 3回連続したら中央からランダムに散らばす
    float center = (JUMPING_SCREEN_W - width) / 2.0f;
    nextX = center + (float)random(-(int)jumpingCurSpread, (int)jumpingCurSpread + 1);
    jumpingEdgeStreak = 0;
  } else {
    nextX = anchorX + (float)random(-(int)jumpingCurSpread, (int)jumpingCurSpread + 1);
  }
  if (nextX < 0) nextX = 0;
  if (nextX > JUMPING_SCREEN_W - width) nextX = JUMPING_SCREEN_W - width;

  jumpingPlatforms[index].w = width;
  jumpingPlatforms[index].x = nextX;
  jumpingPlatforms[index].y = jumpingPlatforms[highestIndex].y - (float)random(jumpingCurGapMin, jumpingCurGapMax + 1);
}

void resetJumpingGame(){
  jumpingScore = 0;
  jumpingLives = JUMPING_LIVES_MAX;
  jumpingGameOver = false;
  jumpingHardOver = false;
  jumpingGameOverSoundPlayed = false;
  jumpingInputLockUntil = 0;
  jumpingLevel = 1;
  computeJumpingLevelParams();

  jumpingPlatforms[0] = {12.0f, 108.0f, 40};
  for (uint8_t i = 1; i < JUMPING_PLATFORM_COUNT; ++i) {
    uint8_t width = randomJumpingPlatformWidth();
    float nextX = jumpingPlatforms[i - 1].x + (float)random(-(int)jumpingCurSpread, (int)jumpingCurSpread + 1);
    if (nextX < 0) nextX = 0;
    if (nextX > JUMPING_SCREEN_W - width) nextX = JUMPING_SCREEN_W - width;
    jumpingPlatforms[i] = {
      nextX,
      jumpingPlatforms[i - 1].y - (float)random(jumpingCurGapMin, jumpingCurGapMax + 1),
      width
    };
  }

  jumpingPlayerX = jumpingPlatforms[0].x + (jumpingPlatforms[0].w - JUMPING_PLAYER_W) / 2.0f;
  jumpingPlayerY = jumpingPlatforms[0].y - JUMPING_PLAYER_H;
  jumpingVelocityY = jumpingCurJumpV;
  jumpingAirJumpAvailable = false;
  jumpingAirGauge = 0;
  display.setRotation(1);
  setInvert(true);
}

void restartJumpingRun(){
  uint32_t currentScore = jumpingScore;
  jumpingGameOver = false;
  jumpingHardOver = false;
  jumpingGameOverSoundPlayed = false;
  jumpingInputLockUntil = 0;
  jumpingLevel = (uint8_t)(currentScore / JUMPING_SCORE_PER_LEVEL) + 1;
  if (jumpingLevel > JUMPING_MAX_LEVEL) jumpingLevel = JUMPING_MAX_LEVEL;
  computeJumpingLevelParams();

  jumpingPlatforms[0] = {12.0f, 108.0f, 40};
  for (uint8_t i = 1; i < JUMPING_PLATFORM_COUNT; ++i) {
    uint8_t width = randomJumpingPlatformWidth();
    float nextX = jumpingPlatforms[i - 1].x + (float)random(-(int)jumpingCurSpread, (int)jumpingCurSpread + 1);
    if (nextX < 0) nextX = 0;
    if (nextX > JUMPING_SCREEN_W - width) nextX = JUMPING_SCREEN_W - width;
    jumpingPlatforms[i] = {
      nextX,
      jumpingPlatforms[i - 1].y - (float)random(jumpingCurGapMin, jumpingCurGapMax + 1),
      width
    };
  }

  jumpingPlayerX = jumpingPlatforms[0].x + (jumpingPlatforms[0].w - JUMPING_PLAYER_W) / 2.0f;
  jumpingPlayerY = jumpingPlatforms[0].y - JUMPING_PLAYER_H;
  jumpingVelocityY = jumpingCurJumpV;
  jumpingAirJumpAvailable = false;
  jumpingAirGauge = 0;
  jumpingScore = (uint32_t)currentScore;
  display.setRotation(1);
  setInvert(true);
}

void commitJumpingDeath(){
  if (!jumpingGameOver) {
    jumpingGameOver = true;
    if (!jumpingGameOverSoundPlayed) { playBeep(SND_GAMEOVER); jumpingGameOverSoundPlayed = true; }
    if (jumpingLives > 0) jumpingLives--;
    if (jumpingLives == 0) {
      jumpingHardOver = true;
      setInvert(false);
    } else {
      jumpingHardOver = false;
    }
    jumpingInputLockUntil = millis() + 500;
    jumpingBtnLeftPrev  = false;
    jumpingBtnRightPrev = false;
  }
}

void updateJumpingGame(){
  if (jumpingGameOver) {
    if (millis() < jumpingInputLockUntil) return;

    bool leftEdge  = btnEdge(PIN_JUMPING_LEFT,  jumpingBtnLeftPrev);
    bool rightEdge = btnEdge(PIN_JUMPING_RIGHT, jumpingBtnRightPrev);
    if (jumpingHardOver) {
      if (leftEdge)  { playBeep(SND_RESTART); resetJumpingGame(); }
      if (rightEdge) { playBeep(SND_CANCEL); setInvert(true); resetTitle(); }
    } else {
      if (leftEdge)  { playBeep(SND_RESTART); restartJumpingRun(); }
      if (rightEdge) { playBeep(SND_CANCEL); setInvert(true); resetTitle(); }
    }
    return;
  }

  float horizontalMove = 0.0f;
  if (digitalRead(PIN_JUMPING_LEFT)  == LOW) horizontalMove -= JUMPING_MOVE_SPEED;
  if (digitalRead(PIN_JUMPING_RIGHT) == LOW) horizontalMove += JUMPING_MOVE_SPEED;
  bool airJumpEdge = btnEdge(PIN_BTN_DASH, btnDashPrev);

  if      (horizontalMove < 0) jumpingFacingLeft = true;
  else if (horizontalMove > 0) jumpingFacingLeft = false;

  jumpingPlayerX += horizontalMove;
  if (jumpingPlayerX < 0) jumpingPlayerX = 0;
  if (jumpingPlayerX > JUMPING_SCREEN_W - JUMPING_PLAYER_W) jumpingPlayerX = JUMPING_SCREEN_W - JUMPING_PLAYER_W;

  float previousBottom = jumpingPlayerY + JUMPING_PLAYER_H;
  jumpingVelocityY += jumpingCurGravity;
  jumpingPlayerY += jumpingVelocityY;

  if (jumpingVelocityY > 0) {
    for (uint8_t i = 0; i < JUMPING_PLATFORM_COUNT; ++i) {
      float topY = jumpingPlatforms[i].y;
      float leftX = jumpingPlatforms[i].x;
      float rightX = jumpingPlatforms[i].x + jumpingPlatforms[i].w;
      float playerLeft = jumpingPlayerX + 3;
      float playerRight = jumpingPlayerX + JUMPING_PLAYER_W - 3;
      // 着地有効範囲：プレイヤーと足場の重なりが3px以上必要
      float overlapLeft  = (playerLeft  > leftX)  ? playerLeft  : leftX;
      float overlapRight = (playerRight < rightX)  ? playerRight : rightX;
      bool overlapsX = (overlapRight - overlapLeft) >= 3.0f;
      bool crossesY = previousBottom <= topY && (jumpingPlayerY + JUMPING_PLAYER_H) >= topY;
      if (overlapsX && crossesY) {
        jumpingPlayerY = topY - JUMPING_PLAYER_H;
        jumpingVelocityY = jumpingCurJumpV;
        jumpingAirJumpAvailable = true;
        if (jumpingAirGauge < 5) jumpingAirGauge++; // 着地でゲージ増加
        playBeep(SND_JUMPING_BOUNCE);
        break;
      }
    }
  }

  // 空中ジャンプ（ゲージに応じた高さ: 1段=0.5倍〜5段=1.5倍）
  if (airJumpEdge && jumpingAirJumpAvailable) {
    // ゲージ1段=0.9倍 〜 5段=2.5倍
    float ratio = 0.9f + jumpingAirGauge * 0.4f;
    jumpingVelocityY = jumpingCurJumpV * ratio;
    jumpingAirJumpAvailable = false;
    // ゲージに応じて音を派手に（高くなるほど高音・長く）
    uint16_t freq = 800 + jumpingAirGauge * 300;  // 1100〜2300Hz
    uint16_t dur  = 40  + jumpingAirGauge * 20;   // 60〜140ms
    if (BUZZER_ENABLED) buzzerPlayRaw(freq, dur);
    jumpingAirGauge = 0;
  }

  if (jumpingPlayerY < JUMPING_SCROLL_TRIGGER_Y) {
    float scrollDelta = JUMPING_SCROLL_TRIGGER_Y - jumpingPlayerY;
    jumpingPlayerY = JUMPING_SCROLL_TRIGGER_Y;
    jumpingScore += (uint32_t)scrollDelta;
    { uint8_t newLv = (uint8_t)(jumpingScore / JUMPING_SCORE_PER_LEVEL) + 1; if (newLv > JUMPING_MAX_LEVEL) newLv = JUMPING_MAX_LEVEL; if (newLv != jumpingLevel) { jumpingLevel = newLv; computeJumpingLevelParams(); playBeep(SND_JUMPING_LEVELUP); } }
    for (uint8_t i = 0; i < JUMPING_PLATFORM_COUNT; ++i) {
      jumpingPlatforms[i].y += scrollDelta;
      if (jumpingPlatforms[i].y > JUMPING_SCREEN_H + 6) respawnJumpingPlatformAbove(i);
    }
  }

  if (jumpingPlayerY > JUMPING_SCREEN_H + 8) {
    commitJumpingDeath();
  }
}

void drawJumpingGame(){
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  for (int x = 4; x < JUMPING_SCREEN_W; x += 18) {
    int y = ((x * 9) + ((int)jumpingScore / 7)) % 28;
    display.drawPixel(x, y, SSD1306_WHITE);
  }

  for (uint8_t i = 0; i < JUMPING_PLATFORM_COUNT; ++i) {
    display.fillRect((int)jumpingPlatforms[i].x, (int)jumpingPlatforms[i].y, jumpingPlatforms[i].w, JUMPING_PLATFORM_H, SSD1306_WHITE);
  }

  if (jumpingFacingLeft)
    drawBitmapFlipH((int)jumpingPlayerX, (int)jumpingPlayerY, PLAYER_BMP, JUMPING_PLAYER_W, JUMPING_PLAYER_H, SSD1306_WHITE);
  else
    display.drawBitmap((int)jumpingPlayerX, (int)jumpingPlayerY, PLAYER_BMP, JUMPING_PLAYER_W, JUMPING_PLAYER_H, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("LV:")); display.print((int)jumpingLevel);
  display.setCursor(33, 0);
  display.print(F("LF:")); display.print((int)jumpingLives);
  display.setCursor(0, 9);
  display.print(F("UP:")); display.print(jumpingScore);

  // 空中ジャンプゲージ（右端に縦バー、5段階）
  {
    const int8_t bx = JUMPING_SCREEN_W - 5;
    const int8_t totalH = 40;
    const int8_t by = 20;
    display.drawRect(bx, by, 4, totalH, SSD1306_WHITE);
    if (jumpingAirGauge > 0){
      int8_t fillH = (int8_t)(totalH * jumpingAirGauge / 5) - 2;
      if (fillH < 1) fillH = 1;
      display.fillRect(bx+1, by + totalH - 1 - fillH, 2, fillH, SSD1306_WHITE);
    }
  }

  if (jumpingGameOver && jumpingHardOver) {
    display.setTextSize(2); drawCenteredTextNarrow(50,  F("GAME OVER"), 1);
    display.setTextSize(1); drawCenteredTextNarrow(64,  F("\x1b:Retry"),  1);
    display.setTextSize(1); drawCenteredTextNarrow(74,  F("Title:\x1a"),  1);
  } else if (jumpingGameOver) {
    display.setTextSize(2); drawCenteredTextNarrow(50,  F("Miss!"),     1);
    display.setTextSize(1); drawCenteredTextNarrow(64,  F("\x1b:Retry"),  1);
    display.setTextSize(1); drawCenteredTextNarrow(74,  F("Title:\x1a"),  1);
  }

  display.display();
}

void setup(){
  pinMode(PIN_BTN_JUMP, INPUT_PULLUP);
  pinMode(PIN_BTN_DASH, INPUT_PULLUP);
  pinMode(PIN_JUMPING_LEFT, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { while(1) delay(200); }
  display.ssd1306_command(0xD5); // ディスプレイクロック設定
  display.ssd1306_command(0x10); // オシレータ最大・分周比1 → リフレッシュレート向上
  display.ssd1306_command(0x81); // コントラスト設定
  display.ssd1306_command(0x70); // 0x00(最暗)〜0xFF(最明)、デフォルト0x7F

  display.stopscroll();
  display.setRotation(0);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.invertDisplay(true);
  g_displayInverted = true;

  randomSeed(analogRead(A0));

  resetTitle();
  resetShootingGame();
}

void drawTitle(){
  display.clearDisplay();

  drawCenteredText(5,  F("BM GamePod"), 2);

  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 28); display.print(F("1: Shooting Game"));
  display.setCursor(10, 40); display.print(F("2: Jumping Game"));

  int selectorY = (titleSelection == 0) ? 28 : 40;
  display.fillTriangle(2, selectorY + 2, 8, selectorY, 8, selectorY + 8, SSD1306_WHITE);

  display.fillRect(0, 52, OLED_WIDTH, 12, SSD1306_BLACK);

  // 左下: ▲(D5=Start) と ▼(D6=Change) を横並び
  // ▲ 上向き三角
  display.fillTriangle(4, 54, 1, 61, 7, 61, SSD1306_WHITE);
  // ▼ 下向き三角
  display.drawTriangle(9, 54, 15, 54, 12, 61, SSD1306_WHITE);

  // 中央: 音アイコン（小、矢印行と同じy）
  {
    const int8_t sx = 57, sy = 55;
    display.fillRect(sx, sy+1, 2, 5, SSD1306_WHITE);      // ボディ
    display.drawLine(sx+1, sy,   sx+4, sy-1, SSD1306_WHITE); // ホーン上
    display.drawLine(sx+1, sy+6, sx+4, sy+8, SSD1306_WHITE); // ホーン下
    if (BUZZER_ENABLED){
      display.drawFastVLine(sx+6, sy+1, 5, SSD1306_WHITE);  // 音波
    } else {
      display.drawLine(sx+6, sy+1, sx+9, sy+6, SSD1306_WHITE); // X
      display.drawLine(sx+9, sy+1, sx+6, sy+6, SSD1306_WHITE);
    }
  }

  // 右下: ● (A3ボタン = サウンドON/OFF)
  display.fillCircle(120, 58, 3, SSD1306_WHITE);

  display.display();
}

void loop(){
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (now - lastFrame < 16) return;
  lastFrame = now;

  bool ledOn = (digitalRead(PIN_BTN_JUMP)==LOW) || (digitalRead(PIN_BTN_DASH)==LOW);
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  switch(mode){
    case MODE_TITLE: {
      bool titleChange = btnEdge(PIN_BTN_JUMP, btnJumpPrev);
      bool soundToggle = btnEdge(PIN_JUMPING_LEFT, jumpingBtnLeftPrev);
      if (titleChange){
        titleSelection ^= 1;
        playBeep(SND_SELECT);
      }
      if (soundToggle){
        BUZZER_ENABLED = !BUZZER_ENABLED;
        if (BUZZER_ENABLED) playBeep(SND_SELECT);
      }
      if (btnEdge(PIN_BTN_DASH, btnDashPrev)){
        playBeep(SND_START);
        setInvert(true);
        if (titleSelection == 0) {
          resetShootingGame();
          mode = MODE_SHOOTING;
        } else {
          resetJumpingGame();
          mode = MODE_JUMPING;
        }
      }
      drawTitle();
    } break;

    case MODE_SHOOTING: {
      updateShootingGame();
      drawShootingGame();
      if (!(shootingGameOver || shootingClear)) {
        shootingScore += 1;
      }
    } break;

    case MODE_JUMPING: {
      updateJumpingGame();
      drawJumpingGame();
    } break;
  }
}
