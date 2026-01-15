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
const int PIN_BTN_JUMP  = 5;  // D5: タイトルで切替 / G1 前進 / G2 上昇
const int PIN_BTN_DASH  = 6;  // D6: タイトルで開始 / G1 ジャンプ / G2 射撃

/* =========================
 *  プレイヤー（20×20ビットマップ）
 * ========================= */
 /*Default
  0x1f,0xff,0x80,0x20,0x00,0x40,0x2a,0x8a,0xc0,0x20,0x00,0x40,0x28,0x00,0xc0,0x23,
  0xcf,0x40,0x21,0x4a,0x40,0x20,0x84,0x40,0x2c,0x01,0xc0,0x2c,0x39,0xc0,0x60,0x00,
  0x70,0x60,0x00,0x50,0xae,0x92,0xd0,0xaa,0x10,0x50,0xee,0x92,0xf0,0xde,0x10,0x70,
  0x1f,0xff,0x80,0x01,0x02,0x00,0x03,0x03,0x00,0x07,0x03,0x80
*/
const uint8_t PROGMEM PLAYER_BMP[60] = {
  0x1f,0xff,0x80,0x20,0x00,0x40,0x2a,0x8a,0xc0,0x20,0x00,0x40,0x28,0x00,0xc0,0x23,
  0xcf,0x40,0x21,0x4a,0x40,0x20,0x84,0x40,0x2c,0x01,0xc0,0x2c,0x39,0xc0,0x60,0x00,
  0x70,0x60,0x00,0x50,0xae,0x92,0xd0,0xaa,0x10,0x50,0xee,0x92,0xf0,0xde,0x10,0x70,
  0x1f,0xff,0x80,0x01,0x02,0x00,0x03,0x03,0x00,0x07,0x03,0x80
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
Beep SND_JUMP     = {1200, 80};
Beep SND_AIRJUMP  = {1050, 60};
Beep SND_STAGEUP  = {1500,100};

/* =========================
 *  入力エッジ検出
 * ========================= */
bool btnJumpPrev=false, btnDashPrev=false;
static inline bool btnEdge(int pin, bool &prev){
  bool now=(digitalRead(pin)==LOW); bool edge=(now && !prev); prev=now; return edge;
}

/* ★★★ 入力ロック時刻 ★★★ */
uint32_t g1_inputLockUntil = 0;  // G1
uint32_t g2_inputLockUntil = 0;  // G2

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
enum Mode : uint8_t { MODE_TITLE=0, MODE_GAME1, MODE_GAME2 };
Mode mode = MODE_TITLE;
uint8_t titleSel = 0; // 0=Game1, 1=Game2
void resetTitle(){ mode = MODE_TITLE; titleSel = 0; }

/* =========================
 *  G1: ジャンプ
 * ========================= */
const int   G1_GROUND_Y       = 52;
const int   G1_PLAYER_X       = 20;
const int   G1_PLAYER_W       = 20;
const int   G1_PLAYER_H       = 20;
const float G1_GRAVITY        = 0.65f;
const float G1_JUMP_V         = -7.0f;
const float G1_AIR_JUMP_V     = -4.0f;
const uint16_t FRAME_MS       = 16;
const float   G1_MOVE_SPEED   = 3.0f;
const uint8_t STAGE_MAX       = 3;
const uint8_t STAGE_DELTA_H   = 2;

const int8_t   G1_BOSS_BOB_AMPL     = 4;
const uint16_t G1_BOSS_BOB_STEP_MS  = 60;

// ★ G1 ライフ
const uint8_t G1_LIVES_MAX = 3;
uint8_t g1_lives = G1_LIVES_MAX;
bool    g1_hardOver = false;   // ライフ0の白黒反転中

enum ObjType : uint8_t { BLOCK=0, PIT=1, PLATFORM=2, BOSS_G1=3 };
struct Obj { ObjType type; uint16_t x; uint8_t w; uint8_t h; };
const uint16_t MAP_LENGTH = 900;
const Obj MAP[] PROGMEM = {
  {BLOCK,120,12,18},{PIT,160,14,0},
  {BLOCK,200,10,12},{PLATFORM,230,18,20},
  {BLOCK,280,10,22},{BLOCK,320,20,10},
  {PIT,360,26,0},
  {PLATFORM,400,22,24},{BLOCK,440,12,14},
  {BLOCK,480,10,12},{BLOCK,520,10,12},{BLOCK,560,10,12},
  {BLOCK,620,16,22},{BLOCK,660,16,24},
  {PIT,700,18,0},
  {PLATFORM,728,16,26},
  {BLOCK,760,12,20},{BLOCK,800,20,12},
  {PIT,840,20,0},{PLATFORM,864,18,24},
  {BOSS_G1,900,BOSS_W,BOSS_H},
};
const uint8_t MAP_LEN = sizeof(MAP)/sizeof(MAP[0]);

float   g1_playerY, g1_vy;
bool    g1_onGround, g1_gameOver, g1_fastFall;
bool    g1_airJumpAvailable;
bool    g1_gameOverSoundPlayed1=false;
uint8_t g1_currentStage;
uint32_t g1_lastFrameMs, g1_startMs;
uint32_t g1_score_dist = 0;
float   g1_camX;
int     g1_bgOffset;

int8_t    g1_bossBob = 0;
int8_t    g1_bossBobDir = 1;
uint32_t  g1_bossBobLastMs = 0;

// ステージ開始位置（camXは各ステージ0始まり）
const float G1_STAGE_START_CAMX = 0.0f;

/* ステージに応じた高さ補正 */
inline uint8_t g1_stageAdjustedH(uint8_t baseH){
  const uint8_t H_MAX = (G1_GROUND_Y - 6);
  uint16_t h = baseH + (uint16_t)g1_currentStage * STAGE_DELTA_H;
  if (h > H_MAX) h = H_MAX;
  return (uint8_t)h;
}
inline int g1_screenX(uint16_t objX, float cam){
  int16_t raw = (int16_t)objX - (int16_t)((uint16_t)cam);
  int sx = raw; if (sx < -128) sx += MAP_LENGTH; return sx;
}

static inline int g1_floorY_at_screen_x(int sx, float cam){
  bool pitHere = false;
  int best = OLED_HEIGHT + 100;

  for(uint8_t loop=0; loop<2; ++loop){
    for(uint8_t i=0; i<MAP_LEN; ++i){
      Obj o; memcpy_P(&o, &MAP[i], sizeof(Obj));
      int ox = g1_screenX(o.x, cam);
      int L = ox, R = ox + o.w - 1;
      if (sx < L || sx > R) continue;

      if (o.type == PIT){
        pitHere = true;
      } else if (o.type == PLATFORM){
        uint8_t hAdj = g1_stageAdjustedH(o.h);
        int topY = G1_GROUND_Y - hAdj;
        if (topY < best) best = topY;
      }
    }
  }
  if (!pitHere){
    if (G1_GROUND_Y < best) best = G1_GROUND_Y;
  }
  return best;
}

int g1_floorYAtPlayerX(){
  const int inset = 2;
  int leftX   = G1_PLAYER_X + inset;
  int rightX  = G1_PLAYER_X + G1_PLAYER_W - 1 - inset;
  int centerX = (leftX + rightX) / 2;

  int fyL = g1_floorY_at_screen_x(leftX,   g1_camX);
  int fyC = g1_floorY_at_screen_x(centerX, g1_camX);
  int fyR = g1_floorY_at_screen_x(rightX,  g1_camX);

  int floorY = fyL;
  if (fyC < floorY) floorY = fyC;
  if (fyR < floorY) floorY = fyR;

  return floorY;
}

void g1_stageStartReset(){  // 同ステージ先頭から再開
  g1_playerY = G1_GROUND_Y - G1_PLAYER_H;
  g1_vy = 0; g1_onGround = true; g1_fastFall=false;
  g1_airJumpAvailable = true;
  g1_camX = G1_STAGE_START_CAMX;
  g1_bgOffset = 0;
  g1_gameOver = false; g1_gameOverSoundPlayed1=false;
  g1_inputLockUntil = 0;
  // ボスの上下ゆらぎ初期化
  g1_bossBob = 0; g1_bossBobDir = 1; g1_bossBobLastMs = millis();
}

void g1_reset(){ // ステージ1開始
  g1_currentStage = 0;
  g1_score_dist   = 0;
  g1_startMs = millis();
  g1_stageStartReset();
  g1_lives = G1_LIVES_MAX;
  g1_hardOver = false;
  setInvert(true); // 通常表示へ
}

void g1_advanceStageIfNeeded(){
  if (g1_camX>=MAP_LENGTH){
    g1_camX -= MAP_LENGTH;
    if (g1_currentStage < STAGE_MAX){ g1_currentStage++; playBeep(SND_STAGEUP); }
    // 新しいステージの先頭にカメラを置く（camXは既に[0,MAP_LENGTH)内）
    // ステージの「スタート位置」はcamX=0扱いなので、ここでbgだけ揃える
    g1_bgOffset = 0;
  }
}

/* ========== G1 ライフ減処理（死亡時即時適用） ========== */
void g1_commitDeath(){
  if (!g1_gameOver){
    g1_gameOver = true;
    if (!g1_gameOverSoundPlayed1){ playBeep(SND_GAMEOVER); g1_gameOverSoundPlayed1=true; }
    if (g1_lives > 0) g1_lives--;
    if (g1_lives == 0){
      g1_hardOver = true;
      setInvert(false); // 反転表示
    }else{
      g1_hardOver = false;
    }
    g1_inputLockUntil = millis() + 500; btnJumpPrev = false; btnDashPrev = false;
  }
}

void g1_update(){
  if (g1_gameOver) return;

  uint32_t now = millis();
  if (now - g1_bossBobLastMs >= G1_BOSS_BOB_STEP_MS) {
    g1_bossBobLastMs = now;
    g1_bossBob += g1_bossBobDir;
    if (g1_bossBob >=  G1_BOSS_BOB_AMPL)  { g1_bossBob =  G1_BOSS_BOB_AMPL;  g1_bossBobDir = -1; }
    if (g1_bossBob <= -G1_BOSS_BOB_AMPL)  { g1_bossBob = -G1_BOSS_BOB_AMPL;  g1_bossBobDir =  1; }
  }

  bool moveHeld = (digitalRead(PIN_BTN_JUMP)==LOW);
  bool jumpEdge = btnEdge(PIN_BTN_DASH, btnDashPrev);

  if (jumpEdge){
    if (g1_onGround){
      g1_vy = G1_JUMP_V; g1_onGround=false; g1_fastFall=false;
      g1_airJumpAvailable = true;
      playBeep(SND_JUMP);
    }else if (g1_airJumpAvailable){
      g1_vy = G1_AIR_JUMP_V; g1_airJumpAvailable=false; g1_fastFall=false;
      playBeep(SND_AIRJUMP);
    }
  }
  g1_vy += G1_GRAVITY;
  g1_playerY += g1_vy;

  int floorY = g1_floorYAtPlayerX();
  if (g1_vy>=0 && (int)(g1_playerY + G1_PLAYER_H) >= floorY){
    g1_playerY = floorY - G1_PLAYER_H; g1_vy=0; g1_onGround=(floorY<OLED_HEIGHT);
    if (g1_onGround){ g1_fastFall=false; g1_airJumpAvailable=true; }
  }else g1_onGround=false;

  float scroll = moveHeld ? G1_MOVE_SPEED : 0.0f;
  g1_camX += scroll;
  if (scroll > 0) g1_score_dist += (uint32_t)scroll;

  g1_advanceStageIfNeeded();

  // 落下（画面外）
  if (g1_playerY > OLED_HEIGHT+10){
    g1_commitDeath();
    return;
  }

  // 当たり判定（ブロック/ボス）
  const int G1_HITBOX_INSET = 2;
  int px1 = G1_PLAYER_X + G1_HITBOX_INSET;
  int py1 = (int)g1_playerY + G1_HITBOX_INSET;
  int px2 = G1_PLAYER_X + G1_PLAYER_W - 1 - G1_HITBOX_INSET;
  int py2 = (int)g1_playerY + G1_PLAYER_H - 1 - G1_HITBOX_INSET;

  for(uint8_t loop=0; loop<2; loop++){
    for(uint8_t i=0;i<MAP_LEN;i++){
      Obj o; memcpy_P(&o,&MAP[i],sizeof(Obj));
      if (o.type!=BLOCK && o.type!=BOSS_G1) continue;

      int sx = g1_screenX(o.x, g1_camX);
      if (sx>=OLED_WIDTH || sx+o.w<0) continue;

      int ox1, oy1, ox2, oy2;
      if (o.type==BLOCK){
        uint8_t hAdj = g1_stageAdjustedH(o.h);
        ox1 = sx; oy1 = G1_GROUND_Y - hAdj; ox2 = ox1 + o.w - 1; oy2 = G1_GROUND_Y - 1;
      }else{
        ox1 = sx;
        oy1 = (G1_GROUND_Y - BOSS_H) - g1_bossBob;
        ox2 = ox1 + BOSS_W - 1;
        oy2 = (G1_GROUND_Y - 1) - g1_bossBob;
      }

      bool hit = !(px2<ox1 || ox2<px1 || py2<oy1 || oy2<py1);
      if (hit){ g1_commitDeath(); return; }
    }
  }

  g1_bgOffset += (int)scroll; if (g1_bgOffset>=16) g1_bgOffset=0;
}

void g1_draw(){
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  // ★ハードゲームオーバー（ライフ0・反転中）：文字のみ + ST/DS 表示
  if (g1_gameOver && g1_hardOver){
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(F("ST:")); display.print((int)g1_currentStage);
    display.setCursor(64,0);
    display.print(F("DS:")); display.print(g1_score_dist);
    display.setTextSize(2); drawCenteredText(22, F("GAME OVER"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Stage1  R:Title"), 1);
    display.display();
    return;
  }

  // 背景・床
  for(int x=0;x<OLED_WIDTH;x+=16){ int y=(x*13 + (g1_bgOffset*3))%48; display.drawPixel(x, y/2, SSD1306_WHITE); }
  for(int x=0;x<OLED_WIDTH;x++){
    bool pit=false;
    for(uint8_t loop=0; loop<2 && !pit; loop++){
      for(uint8_t i=0;i<MAP_LEN;i++){
        Obj o; memcpy_P(&o,&MAP[i],sizeof(Obj));
        if (o.type!=PIT) continue;
        int sx=g1_screenX(o.x,g1_camX); if (x>=sx && x<=sx+o.w-1){ pit=true; break; }
      }
    }
    if(!pit) display.drawPixel(x, G1_GROUND_Y, SSD1306_WHITE);
  }

  // 地形/ボス
  for(uint8_t loop=0; loop<2; loop++){
    for(uint8_t i=0;i<MAP_LEN;i++){
      Obj o; memcpy_P(&o,&MAP[i],sizeof(Obj));
      int sx=g1_screenX(o.x,g1_camX); if (sx>=OLED_WIDTH || sx+o.w<0) continue;

      if (o.type==BLOCK){
        uint8_t h=g1_stageAdjustedH(o.h); int oy=G1_GROUND_Y - h; display.fillRect(sx,oy,o.w,h,SSD1306_WHITE);
      }else if (o.type==PLATFORM){
        uint8_t h=g1_stageAdjustedH(o.h); int top=G1_GROUND_Y - h; int ph=4;
        display.fillRect(sx,top,o.w,ph,SSD1306_WHITE);
        for(int px=sx+2; px<sx+o.w; px+=6) for(int py=top+ph; py<=G1_GROUND_Y; py+=3) display.drawPixel(px,py,SSD1306_WHITE);
      }else if (o.type==BOSS_G1){
        int oy = (G1_GROUND_Y - BOSS_H) - g1_bossBob;
        display.drawBitmap(sx, oy, BOSS_BMP, BOSS_W, BOSS_H, SSD1306_WHITE);
      }
    }
  }

  // プレイヤー
  display.drawBitmap(G1_PLAYER_X, (int)g1_playerY, PLAYER_BMP, G1_PLAYER_W, G1_PLAYER_H, SSD1306_WHITE);

  // HUD（1行目：DIST / STG / LF）
  display.setTextSize(1);
  display.setCursor(0,0);  display.print(F("DIST: ")); display.print(g1_score_dist);
  display.setCursor(78,0); display.print(F("STG:"));  display.print((int)g1_currentStage);
  display.setCursor(110,0); display.print(F("L")); display.print((int)g1_lives);

  if (g1_gameOver){
    display.setTextSize(2); drawCenteredText(22, F("Miss!"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Retry  R:Title"), 1);
  }
  display.display();
}

/* =========================
 *  G2: シューティング
 * ========================= */
// プレイヤー
const int   G2_PLAYER_X = 12;
const int   G2_PLAYER_W = 20;
const int   G2_PLAYER_H = 20;
const float G2_GRAVITY  = 0.7f;
const float G2_THRUST   = -2.6f;
const float G2_FRICTION = 0.98f;

// 敵・弾
const uint8_t G2_MAX_BULLETS     = 8;
const uint8_t G2_MAX_ENEMIES     = 6;
const uint8_t G2_MAX_BOSS_SHOTS  = 8;
const uint8_t G2_BULLET_W = 3, G2_BULLET_H = 3;
const float   G2_BULLET_VX = 3.2f;
const float   G2_ENEMY_BASE_SPEED = 1.6f;
const float   G2_ENEMY_SCORE_SPEED_COEF = 0.8f;

// ステージ係数
const uint16_t G2_SPAWN_BASE_INTERVAL_MS   = 700;
const int16_t  G2_SPAWN_DEC_PER_STAGE_MS   = 50;
const uint16_t G2_SPAWN_MIN_INTERVAL_MS    = 200;
const float    G2_SPEEDMUL_PER_STAGE       = 0.15f;

// 敵追加スポーン確率（%）
const int      G2_EXTRA_ENEMY_PER_STAGE_PC = 15;
const int      G2_EXTRA_ENEMY_MAX_PC       = 70;

// ボス関連
const uint16_t G2_BOSS_SPAWN_SCORE      = 3000;
const uint8_t  G2_BOSS_HP               = 30;
const float    G2_BOSS_VY               = 0.7f;
const int16_t  G2_BOSS_ENTRY_OFFSET_X   = 25;
const int16_t  G2_BOSS_HOLD_X           = 90;
const int16_t  G2_BOSS_MIN_Y            = 1;
const int16_t  G2_BOSS_MARGIN_BOTTOM    = 1;
// ボス弾
const uint8_t  G2_BOSSSHOT_W = 6, G2_BOSSSHOT_H = 6;
const float    G2_BOSSSHOT_VX_BASE      = 1.8f;
const float    G2_BOSSSHOT_VX_STAGE_COEF= 0.15f;
const uint16_t G2_BOSSSHOT_BASE_INTERVAL_MS = 1200;
const int16_t  G2_BOSSSHOT_DEC_PER_STAGE_MS = 40;
const uint16_t G2_BOSSSHOT_MIN_INTERVAL_MS  = 800;

// ライフ
const uint8_t G2_LIVES_MAX = 3;
uint8_t g2_lives = G2_LIVES_MAX;
// ハードゲームオーバー（ライフ0・反転中）
bool g2_hardOver = false;

Beep SND_SHOT     = {1300, 40};
Beep SND_HIT      = { 900, 70};
Beep SND_BOSS_HIT = {1000, 40};
Beep SND_BOSS_DIE = {1600,120};

struct Bullet   { float x,y,vx; uint8_t w,h; bool alive; };
struct Enemy    { float x,y; uint8_t w,h; bool alive; };
struct Boss     { float x,y; float vy; uint8_t w,h; uint8_t hp; bool alive; bool entering; };
struct BossShot { float x,y,vx; uint8_t w,h; bool alive; };

float   g2_py=OLED_HEIGHT/2, g2_vy=0;
Bullet  g2_bul[G2_MAX_BULLETS];
Enemy   g2_en [G2_MAX_ENEMIES];
Boss    g2_boss;
BossShot g2_bshot[G2_MAX_BOSS_SHOTS];
bool    g2_bossSpawned=false;
bool    g2_bossDefeated=false;
bool    g2_clear=false;

uint32_t g2_lastSpawn=0;
uint32_t g2_score=0;
bool     g2_gameOver=false;
bool     g2_gameOverSoundPlayed2=false;

// 進行管理
uint8_t  g2_stage=1;
uint16_t g2_spawnInterval=G2_SPAWN_BASE_INTERVAL_MS;
float    g2_speedMul=1.0f;
uint16_t g2_bossSpawnScore=G2_BOSS_SPAWN_SCORE;
uint32_t g2_nextBossSpawnAt=G2_BOSS_SPAWN_SCORE;
uint32_t g2_stageScoreBase = 0;

// ボス弾
uint32_t g2_bossLastShotMs=0;
uint16_t g2_bossShotInterval=G2_BOSSSHOT_BASE_INTERVAL_MS;

static inline uint8_t g2_countAlive(){ uint8_t c=0; for(uint8_t i=0;i<G2_MAX_ENEMIES;i++) if (g2_en[i].alive) c++; return c; }

void g2_applyStageParams(){
  g2_speedMul = 1.0f + G2_SPEEDMUL_PER_STAGE * g2_stage;

  int16_t cand = (int16_t)G2_SPAWN_BASE_INTERVAL_MS - G2_SPAWN_DEC_PER_STAGE_MS * (int16_t)g2_stage;
  g2_spawnInterval = (cand < (int16_t)G2_SPAWN_MIN_INTERVAL_MS) ? G2_SPAWN_MIN_INTERVAL_MS : (uint16_t)cand;

  int16_t bCand = (int16_t)G2_BOSSSHOT_BASE_INTERVAL_MS - G2_BOSSSHOT_DEC_PER_STAGE_MS * (int16_t)g2_stage;
  g2_bossShotInterval = (bCand < (int16_t)G2_BOSSSHOT_MIN_INTERVAL_MS) ? G2_BOSSSHOT_MIN_INTERVAL_MS : (uint16_t)bCand;
}

void g2_nextStage(){
  g2_stage += 1;
  g2_applyStageParams();

  g2_clear = false;
  g2_gameOver = false;
  g2_gameOverSoundPlayed2 = false;
  g2_hardOver = false;

  for(uint8_t i=0;i<G2_MAX_BULLETS;i++) g2_bul[i]={0,0,0,G2_BULLET_W,G2_BULLET_H,false};
  for(uint8_t i=0;i<G2_MAX_ENEMIES;i++) g2_en[i]={0,0,0,0,false};
  for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++) g2_bshot[i]={0,0,0,0,0,false};

  g2_nextBossSpawnAt = g2_score + g2_bossSpawnScore;
  g2_stageScoreBase  = g2_score;

  g2_boss = { (float)OLED_WIDTH + (float)G2_BOSS_ENTRY_OFFSET_X, 12.0f, G2_BOSS_VY, BOSS_W, BOSS_H, G2_BOSS_HP, false, false };
  g2_bossSpawned=false;
  g2_bossDefeated=false;

  g2_lastSpawn = millis();
  g2_bossLastShotMs = millis();
  g2_inputLockUntil = 0;

  setInvert(true);
  playBeep(SND_START);
}

void g2_reset(){
  g2_py = (OLED_HEIGHT - G2_PLAYER_H)/2; g2_vy = 0; g2_score=0;
  g2_gameOver=false; g2_gameOverSoundPlayed2=false; g2_clear=false; g2_hardOver=false;

  for(uint8_t i=0;i<G2_MAX_BULLETS;i++) g2_bul[i]={0,0,0,G2_BULLET_W,G2_BULLET_H,false};
  for(uint8_t i=0;i<G2_MAX_ENEMIES;i++) g2_en[i]={0,0,0,0,false};
  for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++) g2_bshot[i]={0,0,0,0,0,false};

  g2_stage = 1;
  g2_applyStageParams();

  g2_nextBossSpawnAt = g2_bossSpawnScore;
  g2_stageScoreBase  = 0;

  g2_boss = { (float)OLED_WIDTH + (float)G2_BOSS_ENTRY_OFFSET_X, 12.0f, G2_BOSS_VY, BOSS_W, BOSS_H, G2_BOSS_HP, false, false };
  g2_bossSpawned=false; g2_bossDefeated=false;

  g2_lastSpawn = millis();
  g2_bossLastShotMs = millis();

  g2_inputLockUntil = 0;

  g2_lives = G2_LIVES_MAX;   // ライフ全回復
  setInvert(true);           // 反転解除
}

/* ========== G2 ライフ減処理（死亡時即時適用） ========== */
void g2_commitDeath(){
  if (!g2_gameOver){
    g2_gameOver = true;
    if (!g2_gameOverSoundPlayed2){ playBeep(SND_GAMEOVER); g2_gameOverSoundPlayed2=true; }
    if (g2_lives > 0) g2_lives--;
    if (g2_lives == 0){
      g2_hardOver = true;
      setInvert(false); // 反転へ
    }else{
      g2_hardOver = false; // 通常ゲームオーバー
    }
    g2_inputLockUntil = millis() + 500; btnJumpPrev = false; btnDashPrev = false;
  }
}

void g2_spawnEnemy(){
  for(uint8_t i=0;i<G2_MAX_ENEMIES;i++){
    if (!g2_en[i].alive){
      g2_en[i].alive=true;
      g2_en[i].x = OLED_WIDTH + random(0,25);
      g2_en[i].y = 2 + random(0, OLED_HEIGHT - (2 + 8));
      g2_en[i].w = 8 + random(0,6);
      g2_en[i].h = 6 + random(0,8);
      return;
    }
  }
}
void g2_fire(){
  for(uint8_t i=0;i<G2_MAX_BULLETS;i++){
    if (!g2_bul[i].alive){
      g2_bul[i].alive=true;
      g2_bul[i].w = G2_BULLET_W; g2_bul[i].h = G2_BULLET_H;
      g2_bul[i].x = G2_PLAYER_X + G2_PLAYER_W;
      g2_bul[i].y = g2_py + (G2_PLAYER_H/2) - (g2_bul[i].h/2);
      g2_bul[i].vx= G2_BULLET_VX;
      playBeep(SND_SHOT);
      return;
    }
  }
}
void g2_bossFire(){
  for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++){
    if (!g2_bshot[i].alive){
      g2_bshot[i].alive = true;
      g2_bshot[i].w = G2_BOSSSHOT_W; g2_bshot[i].h = G2_BOSSSHOT_H;
      int8_t dy = random(-10, 11);
      g2_bshot[i].x = g2_boss.x - 2;
      g2_bshot[i].y = g2_boss.y + (BOSS_H/2) + dy - (g2_bshot[i].h/2);
      if (g2_bshot[i].y < 0) g2_bshot[i].y = 0;
      if (g2_bshot[i].y > OLED_HEIGHT - g2_bshot[i].h) g2_bshot[i].y = OLED_HEIGHT - g2_bshot[i].h;
      g2_bshot[i].vx = -(G2_BOSSSHOT_VX_BASE + G2_BOSSSHOT_VX_STAGE_COEF * g2_stage);
      return;
    }
  }
}

void g2_update(){
  bool thrust = (digitalRead(PIN_BTN_JUMP)==LOW);
  bool fireEdge = btnEdge(PIN_BTN_DASH, btnDashPrev);

  // クリア/ゲームオーバー中
  if (g2_clear || g2_gameOver) {
    if (millis() < g2_inputLockUntil) return;

    if (g2_clear){
      // クリア時：L=Next、R=Title
      if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) { g2_nextStage(); }
      if (fireEdge) { playBeep(SND_CANCEL); setInvert(true); resetTitle(); mode = MODE_TITLE; }
    }else{
      // ゲームオーバー
      if (g2_hardOver){
        // ライフ0：反転・文字だけ
        if (fireEdge) { // R: Title
          playBeep(SND_CANCEL);
          setInvert(true);
          resetTitle(); mode = MODE_TITLE;
        } else if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) { // L: Stage1
          playBeep(SND_RESTART);
          g2_reset();    // ステージ1/ライフ満タン/反転解除
        }
      }else{
        // ライフ残あり：L=Retry（同ステージ、ライフ消費済み）、R=Title
        if (fireEdge) { // R: Title
          playBeep(SND_CANCEL);
          setInvert(true);
          resetTitle(); mode = MODE_TITLE;
        }
        if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)) { // L: Retry（ライフは減らさない）
          playBeep(SND_RESTART);
          // 軽量リスタート（スコア/ステージ維持）
          g2_py = (OLED_HEIGHT - G2_PLAYER_H)/2; g2_vy = 0;
          for(uint8_t i=0;i<G2_MAX_BULLETS;i++) g2_bul[i]={0,0,0,G2_BULLET_W,G2_BULLET_H,false};
          for(uint8_t i=0;i<G2_MAX_ENEMIES;i++) g2_en[i]={0,0,0,0,false};
          for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++) g2_bshot[i]={0,0,0,0,0,false};
          g2_bossSpawned=false; g2_bossDefeated=false;
          g2_boss = { (float)OLED_WIDTH + (float)G2_BOSS_ENTRY_OFFSET_X, 12.0f, G2_BOSS_VY, BOSS_W, BOSS_H, G2_BOSS_HP, false, false };
          g2_lastSpawn = millis();
          g2_bossLastShotMs = millis();
          g2_gameOver = false;
          g2_gameOverSoundPlayed2 = false;
          g2_inputLockUntil = 0;
          setInvert(true);
        }
      }
    }
    return;
  }

  // 通常更新
  if (thrust) g2_vy += G2_THRUST;
  g2_vy += G2_GRAVITY;
  g2_vy *= G2_FRICTION;
  g2_py += g2_vy;
  if (g2_py < 0){ g2_py=0; g2_vy=0; }
  if (g2_py > OLED_HEIGHT - G2_PLAYER_H){ g2_py = OLED_HEIGHT - G2_PLAYER_H; g2_vy=0; }

  if (fireEdge) g2_fire();

  // 弾
  for(uint8_t i=0;i<G2_MAX_BULLETS;i++){
    if (!g2_bul[i].alive) continue;
    g2_bul[i].x += g2_bul[i].vx;
    if (g2_bul[i].x > OLED_WIDTH) g2_bul[i].alive=false;
  }

  // ボス出現
  if (!g2_bossSpawned && !g2_bossDefeated && g2_score >= g2_nextBossSpawnAt){
    g2_bossSpawned = true;
    g2_boss.alive = true;
    g2_boss.entering = true;
    g2_boss.x = OLED_WIDTH + G2_BOSS_ENTRY_OFFSET_X;
    g2_boss.y = 12;
    g2_boss.vy = G2_BOSS_VY;
    g2_boss.hp = G2_BOSS_HP;
  }

  // 敵スポーン（ボス中停止）
  uint32_t now = millis();
  if (!g2_bossSpawned || !g2_boss.alive){
    if (now - g2_lastSpawn >= g2_spawnInterval){
      g2_lastSpawn = now;
      if (g2_countAlive() < G2_MAX_ENEMIES){
        g2_spawnEnemy();
        int extraChance = G2_EXTRA_ENEMY_PER_STAGE_PC * (int)g2_stage;
        if (extraChance > G2_EXTRA_ENEMY_MAX_PC) extraChance = G2_EXTRA_ENEMY_MAX_PC;
        if (random(0,100) < extraChance && g2_countAlive() < G2_MAX_ENEMIES) g2_spawnEnemy();
      }
    }
  }

  // 敵更新＆当たり
  for(uint8_t i=0;i<G2_MAX_ENEMIES;i++){
    if (!g2_en[i].alive) continue;

    float scorePhase = 0.0f;
    if (g2_score > g2_stageScoreBase) {
      scorePhase = (float)(g2_score - g2_stageScoreBase) / (float)g2_bossSpawnScore;
      if (scorePhase > 1.0f) scorePhase = 1.0f;
    }
    float vx = G2_ENEMY_BASE_SPEED * g2_speedMul + G2_ENEMY_SCORE_SPEED_COEF * scorePhase;

    g2_en[i].x -= vx;
    if (g2_en[i].x + g2_en[i].w < 0) g2_en[i].alive=false;

    int px1=G2_PLAYER_X, py1=(int)g2_py;
    int px2=px1+G2_PLAYER_W-1, py2=py1+G2_PLAYER_H-1;
    int ex1=(int)g2_en[i].x, ey1=(int)g2_en[i].y;
    int ex2=ex1+g2_en[i].w-1, ey2=ey1+g2_en[i].h-1;
    bool hit = !(px2<ex1 || ex2<px1 || py2<ey1 || ey2<py1);
    if (hit){ g2_commitDeath(); return; }

    for(uint8_t b=0;b<G2_MAX_BULLETS;b++){
      if (!g2_bul[b].alive) continue;
      int bx1=(int)g2_bul[b].x, by1=(int)g2_bul[b].y;
      int bx2=bx1 + g2_bul[b].w - 1, by2=by1 + g2_bul[b].h - 1;
      bool bHit = !(bx2<ex1 || ex2<bx1 || by2<ey1 || ey2<by1);
      if (bHit){
        g2_bul[b].alive=false; g2_en[i].alive=false; g2_score += 100; playBeep(SND_HIT);
        break;
      }
    }
  }

  // ボス更新
  if (g2_bossSpawned && g2_boss.alive){
    if (g2_boss.entering){
      g2_boss.x -= 1.2f;
      if (g2_boss.x <= G2_BOSS_HOLD_X){ g2_boss.x = G2_BOSS_HOLD_X; g2_boss.entering = false; }
    }else{
      g2_boss.y += g2_boss.vy;
      if (g2_boss.y < G2_BOSS_MIN_Y){ g2_boss.y = G2_BOSS_MIN_Y; g2_boss.vy = fabs(g2_boss.vy); }
      if (g2_boss.y > (OLED_HEIGHT - g2_boss.h - G2_BOSS_MARGIN_BOTTOM)){ g2_boss.y = (OLED_HEIGHT - g2_boss.h - G2_BOSS_MARGIN_BOTTOM); g2_boss.vy = -fabs(g2_boss.vy); }

      if (now - g2_bossLastShotMs >= g2_bossShotInterval){
        g2_bossLastShotMs = now;
        g2_bossFire();
        if (g2_stage >= 3) g2_bossFire();
      }
    }

    int px1=G2_PLAYER_X, py1=(int)g2_py;
    int px2=px1+G2_PLAYER_W-1, py2=py1+G2_PLAYER_H-1;
    int bx1=(int)g2_boss.x, by1=(int)g2_boss.y;
    int bx2=bx1+g2_boss.w-1, by2=by1+g2_boss.h-1;
    bool bossHit = !(px2<bx1 || bx2<px1 || py2<by1 || by2<py1);
    if (bossHit){ g2_commitDeath(); return; }

    for(uint8_t b=0;b<G2_MAX_BULLETS;b++){
      if (!g2_bul[b].alive) continue;
      int ex1=(int)g2_bul[b].x, ey1=(int)g2_bul[b].y;
      int ex2=ex1 + g2_bul[b].w - 1, ey2=ey1 + g2_bul[b].h - 1;
      bool bHit = !(ex2<bx1 || bx2<ex1 || ey2<by1 || by2<ey1);
      if (bHit){
        g2_bul[b].alive=false;
        if (g2_boss.hp > 0) g2_boss.hp--;
        g2_score += 120;
        playBeep(SND_BOSS_HIT);
        if (g2_boss.hp == 0){
          g2_boss.alive=false;
          g2_bossDefeated=true;
          playBeep(SND_BOSS_DIE);
          g2_score += 2000;
          g2_clear = true;
          g2_inputLockUntil = millis() + 500; btnJumpPrev = false; btnDashPrev = false;
        }
        break;
      }
    }
  }

  // ボス弾
  for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++){
    if (!g2_bshot[i].alive) continue;
    g2_bshot[i].x += g2_bshot[i].vx;
    if (g2_bshot[i].x + g2_bshot[i].w < 0) { g2_bshot[i].alive=false; continue; }

    int px1=G2_PLAYER_X, py1=(int)g2_py;
    int px2=px1+G2_PLAYER_W-1, py2=py1+G2_PLAYER_H-1;
    int sx1=(int)g2_bshot[i].x, sy1=(int)g2_bshot[i].y;
    int sx2=sx1+g2_bshot[i].w-1, sy2=sy1+g2_bshot[i].h-1;
    bool shHit = !(px2<sx1 || sx2<px1 || py2<sy1 || sy2<py1);
    if (shHit){ g2_commitDeath(); return; }
  }
}

void g2_draw(){
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  // ★ハードゲームオーバー（ライフ0・反転中）：文字のみ + ST/SC 表示
  if (g2_gameOver && g2_hardOver){
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(F("ST:")); display.print((int)g2_stage);
    display.setCursor(64,0);
    display.print(F("SC:")); display.print(g2_score);
    display.setTextSize(2); drawCenteredText(22, F("GAME OVER"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Stage1  R:Title"), 1);
    display.display();
    return;
  }

  // 自機
  int x=G2_PLAYER_X, y=(int)g2_py;
  display.drawBitmap(x, y, PLAYER_BMP, G2_PLAYER_W, G2_PLAYER_H, SSD1306_WHITE);

  // 弾
  for(uint8_t i=0;i<G2_MAX_BULLETS;i++){
    if (g2_bul[i].alive) display.fillRect((int)g2_bul[i].x, (int)g2_bul[i].y, g2_bul[i].w, g2_bul[i].h, SSD1306_WHITE);
  }

  // 敵
  for(uint8_t i=0;i<G2_MAX_ENEMIES;i++){
    if (!g2_en[i].alive) continue;
    display.drawRect((int)g2_en[i].x, (int)g2_en[i].y, g2_en[i].w, g2_en[i].h, SSD1306_WHITE);
  }

  // ボス
  if (g2_bossSpawned && g2_boss.alive){
    display.drawBitmap((int)g2_boss.x, (int)g2_boss.y, BOSS_BMP, BOSS_W, BOSS_H, SSD1306_WHITE);
  }

  // ボス弾
  for(uint8_t i=0;i<G2_MAX_BOSS_SHOTS;i++){
    if (g2_bshot[i].alive) display.fillRect((int)g2_bshot[i].x, (int)g2_bshot[i].y, g2_bshot[i].w, g2_bshot[i].h, SSD1306_WHITE);
  }

  // 上部HUD（1行目：ST/SC/LF）
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(F("ST:")); display.print((int)g2_stage);
  display.setCursor(44,0);
  display.print(F("SC:")); display.print(g2_score);
  display.setCursor(96,0);
  display.print(F("LF:")); display.print((int)g2_lives);

  // ボス戦中は2行目HPバー
  if (g2_bossSpawned && g2_boss.alive){
    const int barX = 0, barY = 8, barW = 120, barH = 6;
    display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
    uint8_t hp = g2_boss.hp; if (hp > G2_BOSS_HP) hp = G2_BOSS_HP;
    int fillW = (int)((barW - 2) * ((float)hp / (float)G2_BOSS_HP) + 0.5f);
    if (fillW < 0) fillW = 0;
    display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
  }

  if (g2_clear){
    display.setTextSize(2); drawCenteredText(22, F("CLEAR!"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Next  R:Title"), 1);
  } else if (g2_gameOver){
    display.setTextSize(2); drawCenteredText(22, F("Miss!"), 2);
    display.setTextSize(1); drawCenteredText(44, F("L:Retry  R:Title"), 1);
  }

  display.display();
}

/* =========================
 *  セットアップ
 * ========================= */
void setup(){
  pinMode(PIN_BTN_JUMP, INPUT_PULLUP);
  pinMode(PIN_BTN_DASH, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { while(1) delay(200); }

  display.stopscroll();
  display.setRotation(0);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.invertDisplay(true);
  g_displayInverted = true;   // このプロジェクトでは true=通常表示 として扱う

  resetTitle();
  g1_reset();
  g2_reset();

  randomSeed(analogRead(A0));
}

/* =========================
 *  タイトル描画
 * ========================= */
void drawTitle(){
  display.clearDisplay();

  drawCenteredText(5,  F("BM GamePod"), 2);

  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 28); display.print(F("1: Jumping Game"));
  display.setCursor(10, 40); display.print(F("2: Shooting Game"));

  int selY = (titleSel==0) ? 28 : 40;
  display.fillTriangle(2, selY+2, 8, selY, 8, selY+8, SSD1306_WHITE);

  display.fillRect(0, 52, OLED_WIDTH, 12, SSD1306_BLACK);
  drawCenteredText(52, F("L:Change  R:Start"), 1);

  display.display();
}

/* =========================
 *  メインループ
 * ========================= */
void loop(){
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (now - lastFrame < 16) return;
  lastFrame = now;

  bool ledOn = (digitalRead(PIN_BTN_JUMP)==LOW) || (digitalRead(PIN_BTN_DASH)==LOW);
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  switch(mode){
    case MODE_TITLE: {
      if (btnEdge(PIN_BTN_JUMP, btnJumpPrev)){ titleSel ^= 1; playBeep(SND_SELECT); }
      if (btnEdge(PIN_BTN_DASH, btnDashPrev)){
        playBeep(SND_START);
        setInvert(true); // 念のため通常表示に戻す
        if (titleSel==0){ g1_reset(); mode=MODE_GAME1; }
        else { g2_reset(); mode=MODE_GAME2; }
      }
      drawTitle();
    } break;

    case MODE_GAME1: {
      g1_update();
      g1_draw();
      if (g1_gameOver){
        bool jEdge = btnEdge(PIN_BTN_JUMP, btnJumpPrev);
        bool dEdge = btnEdge(PIN_BTN_DASH, btnDashPrev);
        if (millis() >= g1_inputLockUntil) {
          if (g1_hardOver){
            if (jEdge){ playBeep(SND_RESTART); g1_reset(); }         // L: Stage1
            if (dEdge){ playBeep(SND_CANCEL); setInvert(true); resetTitle(); mode=MODE_TITLE; } // R: Title
          }else{
            if (jEdge){ playBeep(SND_RESTART); g1_stageStartReset(); } // L: Retry 同ステージ先頭
            if (dEdge){ playBeep(SND_CANCEL); setInvert(true); resetTitle(); mode=MODE_TITLE; } // R: Title
          }
        }
      }
    } break;

    case MODE_GAME2: {
      g2_update();
      g2_draw();
      if (!(g2_gameOver || g2_clear)) {
        g2_score += 1;
      }
    } break;
  }
}
