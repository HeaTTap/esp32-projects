#include <TFT_eSPI.h>
#include "sprites.h"
#include "sounds.h"
#include "map.h"

// Define Button Pins
#define PIN_LEFT   25
#define PIN_RIGHT  26
#define PIN_UP     27  // Jump
#define PIN_DOWN   33  // Crouch

// Buzzer Pin
#define PIN_BUZZER 32

// Game States
#define STATE_TITLE      0
#define STATE_PLAYING    1
#define STATE_DYING      2
#define STATE_WIN        3
#define STATE_GAMEOVER   4

// Physics Constants
#define RUN_ACCEL        0.12f
#define RUN_DECEL        0.18f
#define MAX_RUN_SPEED    2.2f
#define JUMP_FORCE       -5.5f
#define MIN_JUMP_FORCE   -2.5f
#define MAX_FALL_SPEED   4.5f


// Entity Struct
struct Enemy {
  bool active;
  uint8_t type; // 1 = Goomba, 2 = Koopa
  float x;
  float y;
  float vx;
  float vy;
  bool alive;
  bool shell_moving;
  unsigned long flat_timer;
  bool facing_left;
};

// Global Objects
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite buffer = TFT_eSprite(&tft);
SoundEngine sound;
bool fb_is_16bit = true; // High-speed direct memory access tracker
bool mario_is_big = false; // Player size state tracker
unsigned long invincibility_timer = 0; // Hit damage invincibility clock

// Custom High-Fidelity Retro Super Mushroom Sprite (Color Indexed)
const uint8_t sprite_mushroom[256] PROGMEM = {
  0,0,0,0,0,8,8,8,8,8,8,0,0,0,0,0,
  0,0,0,8,8,3,3,3,3,3,3,8,8,0,0,0,
  0,0,8,3,3,3,7,7,7,7,3,3,3,8,0,0,
  0,8,3,3,7,7,7,7,7,7,7,7,3,3,8,0,
  0,8,3,7,7,7,7,3,3,7,7,7,7,3,8,0,
  8,3,3,7,7,7,3,3,3,3,7,7,7,3,3,8,
  8,3,3,3,3,3,3,3,3,3,3,3,3,3,3,8,
  8,8,3,3,3,3,3,3,3,3,3,3,3,3,8,8,
  0,8,8,8,8,7,7,7,7,7,7,8,8,8,8,0,
  0,0,8,7,7,7,8,7,7,8,7,7,7,8,0,0,
  0,0,8,7,7,7,8,7,7,8,7,7,7,8,0,0,
  0,0,8,7,7,7,7,7,7,7,7,7,7,8,0,0,
  0,0,8,7,7,7,7,7,7,7,7,7,7,8,0,0,
  0,0,0,8,7,7,7,7,7,7,7,7,8,0,0,0,
  0,0,0,0,8,8,7,7,7,7,8,8,0,0,0,0,
  0,0,0,0,0,0,8,8,8,8,0,0,0,0,0,0
};

// Game State Variables
uint8_t game_state = STATE_TITLE;
float mario_x = 40;
float mario_y = 150;
float mario_vx = 0;
float mario_vy = 0;
bool mario_on_ground = false;
bool mario_facing_left = false;
bool mario_crouching = false;
int mario_lives = 3;
uint32_t mario_score = 0;
uint8_t mario_coins = 0;
int game_timer = 400;
unsigned long last_sec_time = 0;

// Camera
float cam_x = 0;

// Inputs
bool key_left = false;
bool key_right = false;
bool key_up = false;
bool key_down = false;
bool last_key_up = false;

// Enemies
Enemy enemies[20];
uint16_t loaded_enemy_index = 0;

// Animation/Timer variables
unsigned long state_timer = 0;
int mario_anim_frame = 0;
unsigned long last_anim_tick = 0;

// Local Tile modification tracker (when question blocks are hit or bricks broken)
uint8_t live_map[MAP_HEIGHT][MAP_WIDTH];

// Bouncing Block Animation Structure
struct BounceBlock {
  uint16_t tx;
  uint16_t ty;
  float offset_y;
  float vy;
  bool active;
  uint8_t original_tile;
};
BounceBlock bounce_blocks[5];

// Visual Coin Particle
struct CoinParticle {
  float x;
  float y;
  float vy;
  bool active;
  uint8_t frame;
};
CoinParticle coin_particles[5];

// Prototypes
void reset_game();
void load_level_map();
void check_input();
void update_physics();
void update_enemies();
void check_collisions();
void draw_game();
void draw_hud();
void draw_indexed_sprite(const uint8_t* sprite_data, int x, int y, bool flip_h);
void draw_indexed_sprite_double_height(const uint8_t* sprite_data, int x, int y, bool flip_h);
void spawn_mushroom(float x, float y);
void spawn_enemies_in_view();
void trigger_block_bounce(int tx, int ty, uint8_t tile);
void spawn_coin_particle(float x, float y);
void update_effects();

void setup() {
  Serial.begin(115200);

  // Drive display backlight HIGH (GPIO 15) in case it is wired there
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  // De-assert SD Card CS (GPIO 13) to avoid SPI bus conflicts
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Initialize Buzzer
  sound.init(PIN_BUZZER);

  // Configure Button Inputs with Internal Pullups
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);

  // Initialize Display
  tft.init();
  tft.setRotation(1); // Landscape horizontal view
  tft.fillScreen(TFT_BLACK);

  // Create Double Buffer Framebuffer (240x240, 16-bit color)
  if (buffer.createSprite(240, 240) == nullptr) {
    Serial.println("Error: Failed to allocate 240x240 Framebuffer! Retrying at 8-bit depth...");
    // Fallback: 8-bit index color is supported by TFT_eSPI and saves half the RAM!
    buffer.setColorDepth(8);
    buffer.createSprite(240, 240);
    fb_is_16bit = false; // Disable direct 16-bit memory writes
  } else {
    fb_is_16bit = true;
  }

  reset_game();
  sound.play_bgm();
}

void loop() {
  uint32_t frameStart = millis();
  // Tick BGM and SFX player
  sound.tick();

  // Read hardware inputs
  check_input();

  // Game State Machine
  switch (game_state) {
    case STATE_TITLE:
      buffer.fillSprite(0x5CBF); // Sky Blue Background

      // Draw Title HUD
      draw_hud();

      // Draw Title Logo Text
      buffer.setTextColor(TFT_RED, 0x5CBF);
      buffer.setTextSize(3);
      buffer.drawString("SUPER MARIO", 25, 45);
      buffer.setTextColor(TFT_YELLOW, 0x5CBF);
      buffer.drawString("ESP32 CLONE", 25, 75);

      buffer.setTextColor(TFT_WHITE, 0x5CBF);
      buffer.setTextSize(1);
      buffer.drawString("PRESS JUMP TO PLAY", 65, 120);
      buffer.drawString("LEFT: D25  RIGHT: D26", 60, 150);
      buffer.drawString("JUMP: D27  CROUCH: D33", 58, 165);

      // Draw decorative Ground & Mario
      for (int i = 0; i < 15; i++) {
        draw_indexed_sprite(tile_ground, i * 16, 208, false);
        draw_indexed_sprite(tile_ground, i * 16, 224, false);
      }
      draw_indexed_sprite(mario_idle, 40, 192, false);
      draw_indexed_sprite(tile_pipe_tl, 160, 176, false);
      draw_indexed_sprite(tile_pipe_tr, 176, 176, false);
      draw_indexed_sprite(tile_pipe_bl, 160, 192, false);
      draw_indexed_sprite(tile_pipe_br, 176, 192, false);

      // Transition to game on Up/Jump key press
      if (key_up && !last_key_up) {
        reset_game();
        game_state = STATE_PLAYING;
        sound.play_sfx(SFX_COIN);
      }
      break;

    case STATE_PLAYING:
      update_physics();
      update_enemies();
      update_effects();
      spawn_enemies_in_view();
      draw_game();
      
      // Secondary logic: Tick timer down
      if (millis() - last_sec_time >= 1000) {
        last_sec_time = millis();
        if (game_timer > 0) {
          game_timer--;
          if (game_timer == 0) {
            // Time out death
            game_state = STATE_DYING;
            mario_vy = -4.5f;
            state_timer = millis();
            sound.play_sfx(SFX_DEATH);
          }
        }
      }
      break;

    case STATE_DYING:
      buffer.fillSprite(0x5CBF);
      draw_game();

      // Dying animation: Mario flies off screen vertically
      mario_vy += 0.2f; // Gravity pull during death
      mario_y += mario_vy;

      if (millis() - state_timer > 3000) {
        // After 3 seconds, check remaining lives
        mario_lives--;
        if (mario_lives > 0) {
          // Restart Level
          mario_x = 40;
          mario_y = 150;
          mario_vx = 0;
          mario_vy = 0;
          cam_x = 0;
          game_timer = 400;
          loaded_enemy_index = 0;
          load_level_map();
          game_state = STATE_PLAYING;
          sound.play_bgm();
        } else {
          // Game Over
          game_state = STATE_GAMEOVER;
          state_timer = millis();
          sound.stop();
        }
      }
      break;

    case STATE_WIN:
      // Slide flagpole and walk to castle animation
      if (mario_y < 192) {
        mario_y += 1.0f; // Slide down pole
      } else {
        // Move towards castle
        if (mario_x < 205 * 16) {
          mario_x += 1.0f;
          mario_facing_left = false;
          // Walk animation
          if (millis() - last_anim_tick >= 100) {
            last_anim_tick = millis();
            mario_anim_frame = (mario_anim_frame == 1) ? 2 : 1;
          }
        } else {
          // Inside Castle - Level Complete
          buffer.setTextColor(TFT_YELLOW, TFT_BLACK);
          buffer.setTextSize(2);
          buffer.drawString("STAGE CLEAR!", 50, 100);
        }
      }

      draw_game();

      if (millis() - state_timer > 6000) {
        game_state = STATE_TITLE;
        sound.play_bgm();
      }
      break;

    case STATE_GAMEOVER:
      buffer.fillSprite(TFT_BLACK);
      buffer.setTextColor(TFT_RED, TFT_BLACK);
      buffer.setTextSize(3);
      buffer.drawString("GAME OVER", 40, 100);
      
      buffer.setTextColor(TFT_WHITE, TFT_BLACK);
      buffer.setTextSize(1);
      buffer.drawString("PRESS JUMP TO RESTART", 60, 160);

      if (key_up && !last_key_up) {
        mario_lives = 3;
        mario_score = 0;
        mario_coins = 0;
        game_state = STATE_TITLE;
        sound.play_bgm();
      }
      break;
  }

  // Push double buffer to screen (fast DMA)
  buffer.pushSprite(0, 0);

  // Store last key states
  last_key_up = key_up;

  // Maintain standard 60 FPS rate (~16.6ms per frame)
  int delayTime = 16 - (millis() - frameStart);
  if (delayTime > 0) {
    delay(delayTime);
  }
}

void check_input() {
  // Read active-low hardware buttons
  key_left  = (digitalRead(PIN_LEFT) == LOW);
  key_right = (digitalRead(PIN_RIGHT) == LOW);
  key_up    = (digitalRead(PIN_UP) == LOW);
  key_down  = (digitalRead(PIN_DOWN) == LOW);
}

void reset_game() {
  mario_x = 40;
  mario_y = 150;
  mario_vx = 0;
  mario_vy = 0;
  cam_x = 0;
  game_timer = 400;
  loaded_enemy_index = 0;
  mario_is_big = false;
  invincibility_timer = 0;

  // Clear bounce blocks & particles
  for (int i = 0; i < 5; i++) {
    bounce_blocks[i].active = false;
    coin_particles[i].active = false;
  }

  load_level_map();
}

void load_level_map() {
  // Load original level map structure into live modified buffer
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      live_map[y][x] = pgm_read_byte(&level_map[y][x]);
    }
  }

  // Reset enemies list
  for (int i = 0; i < 20; i++) {
    enemies[i].active = false;
  }
}

// Bounding Box Collision Check for solid tiles
bool is_solid(int tx, int ty) {
  if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;
  uint8_t tile = live_map[ty][tx];
  
  // Solid tile IDs
  return (tile == TILE_GROUND || 
          tile == TILE_BRICK || 
          tile == TILE_QBOX || 
          tile == TILE_QBOX_EMPTY || 
          tile == TILE_SOLID || 
          tile == TILE_PIPE_TL || 
          tile == TILE_PIPE_TR || 
          tile == TILE_PIPE_BL || 
          tile == TILE_PIPE_BR || 
          tile == TILE_CASTLE_BRICK);
}

void update_physics() {
  // Mario Horizontal Movement
  mario_crouching = key_down && mario_on_ground;

  if (mario_crouching) {
    // Decelerate quickly if crouching
    mario_vx *= 0.8f;
  } else {
    if (key_left) {
      mario_vx -= RUN_ACCEL;
      if (mario_vx < -MAX_RUN_SPEED) mario_vx = -MAX_RUN_SPEED;
      mario_facing_left = true;
    } else if (key_right) {
      mario_vx += RUN_ACCEL;
      if (mario_vx > MAX_RUN_SPEED) mario_vx = MAX_RUN_SPEED;
      mario_facing_left = false;
    } else {
      // Apply friction
      if (mario_vx > 0) {
        mario_vx -= RUN_DECEL;
        if (mario_vx < 0) mario_vx = 0;
      } else if (mario_vx < 0) {
        mario_vx += RUN_DECEL;
        if (mario_vx > 0) mario_vx = 0;
      }
    }
  }

  // Jump Input Physics
  if (key_up && mario_on_ground) {
    mario_vy = mario_is_big ? -6.2f : JUMP_FORCE; // Tall Mario jumps higher!
    mario_on_ground = false;
    sound.play_sfx(SFX_JUMP);
  } else if (!key_up && mario_vy < (mario_is_big ? -3.2f : MIN_JUMP_FORCE)) {
    // Variable Jump Height: release button early to cut jump height short
    mario_vy = mario_is_big ? -3.2f : MIN_JUMP_FORCE;
  }

  // Apply Gravity
  mario_vy += 0.25f; // Gravity value
  if (mario_vy > MAX_FALL_SPEED) mario_vy = MAX_FALL_SPEED;

  // Move Mario Horizontally and check collisions
  mario_x += mario_vx;
  
  // Left border clamp
  if (mario_x < cam_x) {
    mario_x = cam_x;
    mario_vx = 0;
  }

  int w = 12; 
  int h = mario_crouching ? (mario_is_big ? 15 : 10) : (mario_is_big ? 30 : 15);
  int offset_y = mario_crouching ? (mario_is_big ? 15 : 5) : 0;

  // Horizontal Collision Resolution
  for (int row = (int)(mario_y + offset_y) / 16; row <= (int)(mario_y + offset_y + h) / 16; row++) {
    // Left edge
    if (is_solid((int)(mario_x) / 16, row)) {
      mario_x = ((int)(mario_x) / 16 + 1) * 16;
      mario_vx = 0;
    }
    // Right edge
    if (is_solid((int)(mario_x + w) / 16, row)) {
      mario_x = ((int)(mario_x + w) / 16) * 16 - w - 1;
      mario_vx = 0;
    }
  }

  // Move Mario Vertically and check collisions
  mario_y += mario_vy;
  mario_on_ground = false;

  // Vertical Collision Resolution
  for (int col = (int)(mario_x) / 16; col <= (int)(mario_x + w) / 16; col++) {
    // Downwards collision (landing)
    if (is_solid(col, (int)(mario_y + offset_y + h) / 16)) {
      mario_y = ((int)(mario_y + offset_y + h) / 16) * 16 - h - offset_y - 1;
      mario_vy = 0;
      mario_on_ground = true;
    }
    // Upwards collision (ceiling bump)
    if (is_solid(col, (int)(mario_y + offset_y) / 16)) {
      mario_y = ((int)(mario_y + offset_y) / 16 + 1) * 16 - offset_y;
      mario_vy = 0.5f; // Bounce down

      // Determine ceiling tile collision coordinate
      int target_tx = col;
      int target_ty = (int)(mario_y + offset_y) / 16 - 1;
      if (target_ty >= 0) {
        uint8_t tile = live_map[target_ty][target_tx];
        if (tile == TILE_BRICK) {
          trigger_block_bounce(target_tx, target_ty, tile);
          live_map[target_ty][target_tx] = TILE_AIR; // Break/Bounce Brick
          mario_score += 50;
          sound.play_sfx(SFX_HITBLOCK);
        } else if (tile == TILE_QBOX) {
          trigger_block_bounce(target_tx, target_ty, tile);
          live_map[target_ty][target_tx] = TILE_QBOX_EMPTY; // Block hit
          
          // Spawning: specific columns (e.g. 16 and 78) release a Super Mushroom, others give coins
          if (target_tx == 16 || target_tx == 78) {
            spawn_mushroom(target_tx * 16, (target_ty - 1) * 16);
          } else {
            mario_coins++;
            mario_score += 100;
            spawn_coin_particle(target_tx * 16, (target_ty - 1) * 16);
            sound.play_sfx(SFX_COIN);
          }
        }
      }
    }
  }

  // Check Pit Deaths
  if (mario_y > 240) {
    game_state = STATE_DYING;
    mario_vy = -4.5f;
    state_timer = millis();
    sound.play_sfx(SFX_DEATH);
  }

  // Scroll Camera
  if (mario_x - cam_x > 120) {
    cam_x = mario_x - 120;
    if (cam_x > (MAP_WIDTH - 15) * 16) {
      cam_x = (MAP_WIDTH - 15) * 16;
    }
  }

  // Animation ticks
  if (!mario_on_ground) {
    mario_anim_frame = 3; // Jump Sprite
  } else if (mario_crouching) {
    mario_anim_frame = 4; // Crouch Sprite
  } else if (abs(mario_vx) < 0.1f) {
    mario_anim_frame = 0; // Idle Sprite
  } else {
    // Walking animation cycle
    if (millis() - last_anim_tick >= (100 - abs(mario_vx) * 20)) {
      last_anim_tick = millis();
      mario_anim_frame = (mario_anim_frame == 1) ? 2 : 1;
    }
  }

  // Check Flagpole Collision (Level Finish!)
  int pole_col = (int)(mario_x + 6) / 16;
  int pole_row = (int)(mario_y + 8) / 16;
  if (pole_col >= 0 && pole_col < MAP_WIDTH) {
    if (live_map[pole_row][pole_col] == TILE_FLAGPOLE) {
      game_state = STATE_WIN;
      mario_vx = 0;
      mario_vy = 0;
      state_timer = millis();
      sound.play_sfx(SFX_WIN);
    }
  }
}

void trigger_block_bounce(int tx, int ty, uint8_t tile) {
  for (int i = 0; i < 5; i++) {
    if (!bounce_blocks[i].active) {
      bounce_blocks[i].tx = tx;
      bounce_blocks[i].ty = ty;
      bounce_blocks[i].offset_y = 0;
      bounce_blocks[i].vy = -2.5f;
      bounce_blocks[i].active = true;
      bounce_blocks[i].original_tile = tile;
      break;
    }
  }
}

void spawn_coin_particle(float x, float y) {
  for (int i = 0; i < 5; i++) {
    if (!coin_particles[i].active) {
      coin_particles[i].x = x + 4;
      coin_particles[i].y = y;
      coin_particles[i].vy = -3.5f;
      coin_particles[i].frame = 0;
      coin_particles[i].active = true;
      break;
    }
  }
}

void spawn_mushroom(float x, float y) {
  for (int i = 0; i < 20; i++) {
    if (!enemies[i].active) {
      enemies[i].active = true;
      enemies[i].type = 3; // 3 = Super Mushroom
      enemies[i].x = x;
      enemies[i].y = y;
      enemies[i].vx = 1.0f; // Slide right
      enemies[i].vy = -2.0f; // Pop upwards out of Question Box
      enemies[i].alive = true;
      enemies[i].shell_moving = false;
      enemies[i].flat_timer = 0;
      enemies[i].facing_left = false;
      sound.play_sfx(SFX_HITBLOCK); // Play pop sound!
      break;
    }
  }
}

void update_effects() {
  // Update Block Bounces
  for (int i = 0; i < 5; i++) {
    if (bounce_blocks[i].active) {
      bounce_blocks[i].offset_y += bounce_blocks[i].vy;
      bounce_blocks[i].vy += 0.4f; // Block gravity return
      if (bounce_blocks[i].offset_y >= 0) {
        bounce_blocks[i].offset_y = 0;
        bounce_blocks[i].active = false;
      }
    }
  }

  // Update Coins rising
  for (int i = 0; i < 5; i++) {
    if (coin_particles[i].active) {
      coin_particles[i].y += coin_particles[i].vy;
      coin_particles[i].vy += 0.3f;
      coin_particles[i].frame++;
      if (coin_particles[i].vy > 3.0f || coin_particles[i].frame > 20) {
        coin_particles[i].active = false;
      }
    }
  }
}

void spawn_enemies_in_view() {
  // Spawns enemies as the camera advances
  unsigned long now = millis();
  
  // Read spawn list from PROGMEM
  for (int i = loaded_enemy_index; i < ENEMY_COUNT; i++) {
    EnemySpawn spawn;
    memcpy_P(&spawn, &enemy_spawns[i], sizeof(EnemySpawn));

    float spawn_x = spawn.x * 16;
    if (spawn_x < cam_x + 240 + 32) {
      // Enemy entered viewport + pad, activate in first available slot
      for (int slot = 0; slot < 20; slot++) {
        if (!enemies[slot].active) {
          enemies[slot].active = true;
          enemies[slot].type = spawn.type;
          enemies[slot].x = spawn_x;
          enemies[slot].y = spawn.y * 16;
          enemies[slot].vx = (spawn.type == 1) ? -0.5f : -0.6f; // Initial walk speed
          enemies[slot].vy = 0;
          enemies[slot].alive = true;
          enemies[slot].shell_moving = false;
          enemies[slot].flat_timer = 0;
          enemies[slot].facing_left = true;
          break;
        }
      }
      loaded_enemy_index = i + 1; // Advanced spawned pointer
    } else {
      break; // Sorted by X axis, safe to stop
    }
  }
}

void update_enemies() {
  unsigned long now = millis();
  int mw = 12;
  int mh = 15;

  for (int i = 0; i < 20; i++) {
    if (!enemies[i].active) continue;

    // Handle dead flat goomba animation timeout
    if (!enemies[i].alive && enemies[i].type == 1) {
      if (now - enemies[i].flat_timer > 500) {
        enemies[i].active = false; // Despawn
      }
      continue;
    }

    // Apply gravity to enemies
    enemies[i].vy += 0.25f;
    if (enemies[i].vy > MAX_FALL_SPEED) enemies[i].vy = MAX_FALL_SPEED;

    // Horizontal Movement
    enemies[i].x += enemies[i].vx;

    // Despawn if fell off camera left
    if (enemies[i].x < cam_x - 32) {
      enemies[i].active = false;
      continue;
    }

    // Solid Map Collisions
    for (int row = (int)(enemies[i].y) / 16; row <= (int)(enemies[i].y + 15) / 16; row++) {
      if (is_solid((int)(enemies[i].x) / 16, row)) {
        enemies[i].x = ((int)(enemies[i].x) / 16 + 1) * 16;
        enemies[i].vx = -enemies[i].vx; // Reverse direction
        enemies[i].facing_left = !enemies[i].facing_left;
      }
      if (is_solid((int)(enemies[i].x + 15) / 16, row)) {
        enemies[i].x = ((int)(enemies[i].x + 15) / 16) * 16 - 16;
        enemies[i].vx = -enemies[i].vx; // Reverse direction
        enemies[i].facing_left = !enemies[i].facing_left;
      }
    }

    // Vertical Movement
    enemies[i].y += enemies[i].vy;
    for (int col = (int)(enemies[i].x) / 16; col <= (int)(enemies[i].x + 15) / 16; col++) {
      if (is_solid(col, (int)(enemies[i].y + 15) / 16)) {
        enemies[i].y = ((int)(enemies[i].y + 15) / 16) * 16 - 16;
        enemies[i].vy = 0;
      }
    }

    // Bounding Box Collision with Mario
    float ex = enemies[i].x;
    float ey = enemies[i].y;
    int mario_h = mario_crouching ? (mario_is_big ? 15 : 10) : (mario_is_big ? 30 : 15);
    int mario_offset_y = mario_crouching ? (mario_is_big ? 15 : 5) : 0;

    if (mario_x + mw > ex && mario_x < ex + 15 &&
        mario_y + mario_offset_y + mario_h > ey && mario_y + mario_offset_y < ey + 15) {
      
      // Hit detection
      if (enemies[i].type == 3) {
        // Collect Super Mushroom!
        enemies[i].active = false;
        mario_score += 1000;
        if (!mario_is_big) {
          mario_is_big = true;
          sound.play_sfx(SFX_COIN); // Power-up arpeggio!
        }
      } else if (mario_vy > 0.5f && (mario_y + mario_offset_y + mario_h - ey) < 8) {
        // Stomp Enemy!
        mario_score += 200;
        mario_vy = -3.2f; // Mario bounce bounce

        if (enemies[i].type == 1) {
          // Goomba Stomp
          enemies[i].alive = false;
          enemies[i].vx = 0;
          enemies[i].flat_timer = now;
          sound.play_sfx(SFX_STOMP);
        } else if (enemies[i].type == 2) {
          // Koopa shell transition
          if (!enemies[i].shell_moving) {
            enemies[i].shell_moving = true;
            enemies[i].vx = (mario_vx >= 0) ? 3.0f : -3.0f; // Kick shell!
            sound.play_sfx(SFX_STOMP);
          } else {
            enemies[i].shell_moving = false;
            enemies[i].vx = 0;
            sound.play_sfx(SFX_STOMP);
          }
        }
      } else {
        // Hurt Mario!
        if (enemies[i].type == 2 && !enemies[i].shell_moving) {
          // Kick stationary shell instead of dying
          enemies[i].shell_moving = true;
          enemies[i].vx = (mario_x < ex) ? 3.0f : -3.0f;
          sound.play_sfx(SFX_STOMP);
        } else {
          // Apply Power-down Invincibility frames
          if (now - invincibility_timer > 2000) {
            if (mario_is_big) {
              mario_is_big = false;
              invincibility_timer = now;
              sound.play_sfx(SFX_HITBLOCK); // Play power-down shrink tone!
            } else {
              // Mario dies
              game_state = STATE_DYING;
              mario_vy = -4.5f; // Death vertical jump velocity
              state_timer = now;
              sound.play_sfx(SFX_DEATH);
            }
          }
        }
      }
    }
  }
}

void draw_game() {
  buffer.fillSprite(0x5CBF); // Clear Screen with Sky Blue

  int tile_offset_x = (int)cam_x % 16;
  int start_tx = (int)cam_x / 16;

  // 1. Draw level tile blocks
  for (int ty = 0; ty < MAP_HEIGHT; ty++) {
    for (int tx = 0; tx < 16; tx++) {
      int map_col = start_tx + tx;
      if (map_col >= MAP_WIDTH) break;

      uint8_t tile = live_map[ty][map_col];

      // Check if block is currently animated bouncing
      float offset_y = 0;
      for (int b = 0; b < 5; b++) {
        if (bounce_blocks[b].active && bounce_blocks[b].tx == map_col && bounce_blocks[b].ty == ty) {
          offset_y = bounce_blocks[b].offset_y;
          // Temporarily drawEmpty Question Block during bounce
          if (bounce_blocks[b].original_tile == TILE_QBOX) {
            tile = TILE_QBOX_EMPTY;
          }
        }
      }

      int render_x = tx * 16 - tile_offset_x;
      int render_y = ty * 16 + (int)offset_y;

      switch (tile) {
        case TILE_GROUND:
          draw_indexed_sprite(tile_ground, render_x, render_y, false);
          break;
        case TILE_BRICK:
          draw_indexed_sprite(tile_brick, render_x, render_y, false);
          break;
        case TILE_QBOX:
          draw_indexed_sprite(tile_qbox, render_x, render_y, false);
          break;
        case TILE_QBOX_EMPTY:
          draw_indexed_sprite(tile_qbox_empty, render_x, render_y, false);
          break;
        case TILE_SOLID:
          draw_indexed_sprite(tile_solid, render_x, render_y, false);
          break;
        case TILE_PIPE_TL:
          draw_indexed_sprite(tile_pipe_tl, render_x, render_y, false);
          break;
        case TILE_PIPE_TR:
          draw_indexed_sprite(tile_pipe_tr, render_x, render_y, false);
          break;
        case TILE_PIPE_BL:
          draw_indexed_sprite(tile_pipe_bl, render_x, render_y, false);
          break;
        case TILE_PIPE_BR:
          draw_indexed_sprite(tile_pipe_br, render_x, render_y, false);
          break;
        case TILE_FLAGPOLE:
          draw_indexed_sprite(tile_flagpole, render_x, render_y, false);
          break;
        case TILE_FLAG:
          draw_indexed_sprite(tile_flag, render_x, render_y, false);
          break;
        case TILE_CASTLE_BRICK:
          draw_indexed_sprite(tile_castle_brick, render_x, render_y, false);
          break;
        case TILE_CASTLE_BATTLE:
          draw_indexed_sprite(tile_castle_battle, render_x, render_y, false);
          break;
      }
    }
  }

  // 2. Draw Coin Particle Effects
  for (int i = 0; i < 5; i++) {
    if (coin_particles[i].active) {
      // Draw small golden spinning rectangular block
      int cx = coin_particles[i].x - cam_x;
      int cy = coin_particles[i].y;
      buffer.fillRect(cx, cy, 6, 10, 0xFDE0);
      buffer.drawRect(cx, cy, 6, 10, 0x0000);
    }
  }

  // 3. Draw Active Enemies
  for (int i = 0; i < 20; i++) {
    if (!enemies[i].active) continue;

    int render_ex = enemies[i].x - cam_x;
    int render_ey = enemies[i].y;

    if (enemies[i].type == 1) { // Goomba
      if (!enemies[i].alive) {
        draw_indexed_sprite(goomba_flat, render_ex, render_ey, false);
      } else {
        // Alternate walk frame sprites every 150ms
        const uint8_t* goomba_sprite = ((millis() / 150) % 2 == 0) ? goomba_walk1 : goomba_walk2;
        draw_indexed_sprite(goomba_sprite, render_ex, render_ey, false);
      }
    } else if (enemies[i].type == 2) { // Koopa
      if (enemies[i].shell_moving || !enemies[i].alive) {
        draw_indexed_sprite(koopa_shell, render_ex, render_ey, false);
      } else {
        const uint8_t* koopa_sprite = ((millis() / 150) % 2 == 0) ? koopa_walk1 : koopa_walk2;
        draw_indexed_sprite(koopa_sprite, render_ex, render_ey, enemies[i].facing_left);
      }
    } else if (enemies[i].type == 3) {
      // Draw Super Mushroom
      draw_indexed_sprite(sprite_mushroom, render_ex, render_ey, false);
    }
  }

  // 4. Draw Mario
  unsigned long now = millis();
  int render_mx = mario_x - cam_x;
  int render_my = mario_is_big ? (mario_y - 2) : mario_y;

  if (game_state == STATE_DYING) {
    draw_indexed_sprite(mario_dead, render_mx, render_my, false);
  } else {
    // Blinking effect when invincible after taking damage
    if (now - invincibility_timer > 2000 || (now / 100) % 2 == 0) {
      const uint8_t* current_mario_sprite = mario_idle;
      if (mario_is_big) {
        switch (mario_anim_frame) {
          case 0: current_mario_sprite = big_mario_idle; break;
          case 1: current_mario_sprite = big_mario_walk1; break;
          case 2: current_mario_sprite = big_mario_walk2; break;
          case 3: current_mario_sprite = big_mario_jump; break;
          case 4: current_mario_sprite = big_mario_crouch; break;
        }
        draw_indexed_sprite_double_height(current_mario_sprite, render_mx, render_my, mario_facing_left);
      } else {
        switch (mario_anim_frame) {
          case 0: current_mario_sprite = mario_idle; break;
          case 1: current_mario_sprite = mario_walk1; break;
          case 2: current_mario_sprite = mario_walk2; break;
          case 3: current_mario_sprite = mario_jump; break;
          case 4: current_mario_sprite = mario_crouch; break;
        }
        draw_indexed_sprite(current_mario_sprite, render_mx, render_my, mario_facing_left);
      }
    }
  }

  // 5. Draw HUD Overlay
  draw_hud();
}

void draw_hud() {
  buffer.setTextColor(TFT_WHITE);
  buffer.setTextSize(1);

  // Score
  buffer.drawString("MARIO", 10, 5);
  char score_str[8];
  sprintf(score_str, "%06d", mario_score);
  buffer.drawString(score_str, 10, 15);

  // Coins count
  buffer.drawString("COINS", 75, 5);
  char coin_str[6];
  sprintf(coin_str, "o x%02d", mario_coins);
  buffer.drawString(coin_str, 75, 15);

  // World Stage
  buffer.drawString("WORLD", 135, 5);
  buffer.drawString("1-1", 140, 15);

  // Lives count
  buffer.drawString("LIVES", 180, 5);
  char live_str[4];
  sprintf(live_str, "x%d", mario_lives);
  buffer.drawString(live_str, 185, 15);

  // Timer
  buffer.drawString("TIME", 212, 5);
  char time_str[5];
  sprintf(time_str, "%03d", game_timer);
  buffer.drawString(time_str, 212, 15);
}

// Optimized Custom Render Function utilizing Direct Memory Access (DMA) and Pointer Arithmetic
void draw_indexed_sprite(const uint8_t* sprite_data, int x, int y, bool flip_h) {
  // Directly write to the raw 16-bit RAM buffer array to bypass slow drawPixel function overhead
  uint16_t* fb = (uint16_t*)buffer.getPointer();

  if (fb != nullptr && fb_is_16bit) {
    for (int row = 0; row < 16; row++) {
      int screen_y = y + row;
      if (screen_y < 0 || screen_y >= 240) continue; // Boundary Y clipping
      
      uint16_t* fb_row = &fb[screen_y * 240]; // Pre-calculate exact row memory address
      const uint8_t* sprite_row = &sprite_data[row * 16]; // Pre-calculate sprite row offset in flash

      for (int col = 0; col < 16; col++) {
        int screen_x = x + col;
        if (screen_x < 0 || screen_x >= 240) continue; // Boundary X clipping

        int src_col = flip_h ? (15 - col) : col;
        uint8_t color_idx = pgm_read_byte(&sprite_row[src_col]);
        
        if (color_idx != PAL_TRANS) {
          uint16_t color = pgm_read_word(&palette[color_idx]);
          fb_row[screen_x] = color; // Direct RAM write at pre-calculated offset (blazing fast!)
        }
      }
    }
  } else {
    // Robust slow path fallback if using 8-bit indexed buffer or if pointer is unavailable
    for (int row = 0; row < 16; row++) {
      int screen_y = y + row;
      if (screen_y < 0 || screen_y >= 240) continue;
      
      for (int col = 0; col < 16; col++) {
        int screen_x = x + col;
        if (screen_x < 0 || screen_x >= 240) continue;

        int src_col = flip_h ? (15 - col) : col;
        uint8_t color_idx = pgm_read_byte(&sprite_data[row * 16 + src_col]);
        
        if (color_idx != PAL_TRANS) {
          uint16_t color = pgm_read_word(&palette[color_idx]);
          buffer.drawPixel(screen_x, screen_y, color);
        }
      }
    }
  }
}

// Optimized Custom Render Function for Double-Height Mario (16x32)
void draw_indexed_sprite_double_height(const uint8_t* sprite_data, int x, int y, bool flip_h) {
  uint16_t* fb = (uint16_t*)buffer.getPointer();

  if (fb != nullptr && fb_is_16bit) {
    for (int row = 0; row < 32; row++) {
      int screen_y = y + row;
      if (screen_y < 0 || screen_y >= 240) continue; // Boundary Y clipping
      
      uint16_t* fb_row = &fb[screen_y * 240];
      const uint8_t* sprite_row_data = &sprite_data[row * 16];

      for (int col = 0; col < 16; col++) {
        int screen_x = x + col;
        if (screen_x < 0 || screen_x >= 240) continue; // Boundary X clipping

        int src_col = flip_h ? (15 - col) : col;
        uint8_t color_idx = pgm_read_byte(&sprite_row_data[src_col]);
        
        if (color_idx != PAL_TRANS) {
          uint16_t color = pgm_read_word(&palette[color_idx]);
          fb_row[screen_x] = color;
        }
      }
    }
  } else {
    // Robust slow path fallback if using 8-bit indexed buffer
    for (int row = 0; row < 32; row++) {
      int screen_y = y + row;
      if (screen_y < 0 || screen_y >= 240) continue;
      
      for (int col = 0; col < 16; col++) {
        int screen_x = x + col;
        if (screen_x < 0 || screen_x >= 240) continue;

        int src_col = flip_h ? (15 - col) : col;
        uint8_t color_idx = pgm_read_byte(&sprite_data[row * 16 + src_col]);
        
        if (color_idx != PAL_TRANS) {
          uint16_t color = pgm_read_word(&palette[color_idx]);
          buffer.drawPixel(screen_x, screen_y, color);
        }
      }
    }
  }
}
