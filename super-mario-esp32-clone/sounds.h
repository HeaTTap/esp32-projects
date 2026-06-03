#ifndef SOUNDS_H
#define SOUNDS_H

#include <Arduino.h>

// Note Frequencies (Hz)
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_REST 0

// SFX IDs
#define SFX_NONE       0
#define SFX_JUMP       1
#define SFX_COIN       2
#define SFX_STOMP      3
#define SFX_HITBLOCK   4
#define SFX_DEATH      5
#define SFX_WIN        6

struct SoundNote {
  uint16_t freq;
  uint16_t duration;
};

// ----------------------------------------------------
// Melodies (Stored in PROGMEM to save RAM)
// ----------------------------------------------------

// Mario Overworld Theme (BGM)
const SoundNote bgm_mario[] PROGMEM = {
  {NOTE_E5, 125}, {NOTE_E5, 125}, {NOTE_REST, 125}, {NOTE_E5, 125}, {NOTE_REST, 125}, {NOTE_C5, 125}, {NOTE_E5, 125}, {NOTE_REST, 125},
  {NOTE_G5, 125}, {NOTE_REST, 375}, {NOTE_G4, 125}, {NOTE_REST, 375},
  
  {NOTE_C5, 187}, {NOTE_REST, 125}, {NOTE_G4, 125}, {NOTE_REST, 250}, {NOTE_E4, 187}, {NOTE_REST, 125},
  {NOTE_A4, 125}, {NOTE_REST, 125}, {NOTE_B4, 125}, {NOTE_REST, 125}, {NOTE_AS4, 125}, {NOTE_A4, 125},
  
  {NOTE_G4, 83}, {NOTE_E5, 83}, {NOTE_G5, 83}, {NOTE_A5, 125}, {NOTE_REST, 125}, {NOTE_F5, 125}, {NOTE_G5, 125},
  {NOTE_REST, 125}, {NOTE_E5, 125}, {NOTE_REST, 125}, {NOTE_C5, 125}, {NOTE_D5, 125}, {NOTE_B4, 125}, {NOTE_REST, 250},
  
  {NOTE_C5, 187}, {NOTE_REST, 125}, {NOTE_G4, 125}, {NOTE_REST, 250}, {NOTE_E4, 187}, {NOTE_REST, 125},
  {NOTE_A4, 125}, {NOTE_REST, 125}, {NOTE_B4, 125}, {NOTE_REST, 125}, {NOTE_AS4, 125}, {NOTE_A4, 125},
  
  {NOTE_G4, 83}, {NOTE_E5, 83}, {NOTE_G5, 83}, {NOTE_A5, 125}, {NOTE_REST, 125}, {NOTE_F5, 125}, {NOTE_G5, 125},
  {NOTE_REST, 125}, {NOTE_E5, 125}, {NOTE_REST, 125}, {NOTE_C5, 125}, {NOTE_D5, 125}, {NOTE_B4, 125}, {NOTE_REST, 250}
};
const uint16_t BGM_LEN = sizeof(bgm_mario) / sizeof(SoundNote);

// SFX melodies
const SoundNote sfx_jump[] PROGMEM = {
  {NOTE_C4, 50}, {NOTE_D4, 50}, {NOTE_E4, 50}, {NOTE_F4, 50}, {NOTE_G4, 50}, {NOTE_A4, 50}, {NOTE_B4, 50}, {NOTE_C5, 80}
};
const uint16_t JUMP_LEN = sizeof(sfx_jump) / sizeof(SoundNote);

const SoundNote sfx_coin[] PROGMEM = {
  {NOTE_B5, 80}, {NOTE_E6, 250}
};
const uint16_t COIN_LEN = sizeof(sfx_coin) / sizeof(SoundNote);

const SoundNote sfx_stomp[] PROGMEM = {
  {NOTE_A3, 60}, {NOTE_E3, 60}, {NOTE_C3, 100}
};
const uint16_t STOMP_LEN = sizeof(sfx_stomp) / sizeof(SoundNote);

const SoundNote sfx_hitblock[] PROGMEM = {
  {NOTE_C3, 60}, {NOTE_REST, 30}, {NOTE_C3, 80}
};
const uint16_t HITBLOCK_LEN = sizeof(sfx_hitblock) / sizeof(SoundNote);

const SoundNote sfx_death[] PROGMEM = {
  {NOTE_C5, 100}, {NOTE_CS5, 100}, {NOTE_D5, 100}, {NOTE_REST, 100},
  {NOTE_B4, 100}, {NOTE_F5, 100}, {NOTE_REST, 100}, {NOTE_F5, 100},
  {NOTE_F5, 100}, {NOTE_E5, 100}, {NOTE_D5, 100}, {NOTE_C5, 150}
};
const uint16_t DEATH_LEN = sizeof(sfx_death) / sizeof(SoundNote);

const SoundNote sfx_win[] PROGMEM = {
  {NOTE_G4, 100}, {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_C6, 100}, {NOTE_E6, 100},
  {NOTE_G6, 250}, {NOTE_E6, 250}, {NOTE_REST, 50},
  {NOTE_GS4, 100}, {NOTE_C5, 100}, {NOTE_DS5, 100}, {NOTE_GS5, 100}, {NOTE_C6, 100}, {NOTE_DS6, 100},
  {NOTE_GS6, 250}, {NOTE_DS6, 250}, {NOTE_REST, 50},
  {NOTE_AS4, 100}, {NOTE_D5, 100}, {NOTE_F5, 100}, {NOTE_AS5, 100}, {NOTE_D6, 100}, {NOTE_F6, 100},
  {NOTE_AS6, 250}, {NOTE_AS6, 100}, {NOTE_AS6, 100}, {NOTE_AS6, 100}, {NOTE_C7, 400}
};
const uint16_t WIN_LEN = sizeof(sfx_win) / sizeof(SoundNote);

// ----------------------------------------------------
// Sound Player State Machine
// ----------------------------------------------------
class SoundEngine {
private:
  uint8_t buzzer_pin;
  bool is_muted;
  
  // Player state
  const SoundNote* current_track;
  uint16_t track_len;
  uint16_t track_index;
  unsigned long note_end_time;
  bool playing_sfx;
  bool loop_bgm;

public:
  SoundEngine() : buzzer_pin(26), is_muted(false), current_track(nullptr), track_len(0), track_index(0), note_end_time(0), playing_sfx(false), loop_bgm(false) {}

  void init(uint8_t pin) {
    buzzer_pin = pin;
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, LOW);

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#ifdef ESP_ARDUINO_VERSION_MAJOR
#if ESP_ARDUINO_VERSION_MAJOR >= 3
#define LEDC_NEW_API
#endif
#endif

#ifdef LEDC_NEW_API
    ledcAttach(buzzer_pin, 2000, 8);
#else
    ledcSetup(0, 2000, 8); // Channel 0, 2000Hz, 8-bit
    ledcAttachPin(buzzer_pin, 0);
#endif
  }

  void mute(bool enable) {
    is_muted = enable;
    if (is_muted) {
      stop_tone();
    }
  }

  void play_bgm() {
    if (playing_sfx) return; // Don't interrupt playing SFX
    
    current_track = bgm_mario;
    track_len = BGM_LEN;
    track_index = 0;
    note_end_time = 0;
    playing_sfx = false;
    loop_bgm = true;
  }

  void play_sfx(uint8_t sfx_id) {
    stop_tone();
    
    playing_sfx = true;
    loop_bgm = false;
    track_index = 0;
    note_end_time = 0;

    switch (sfx_id) {
      case SFX_JUMP:
        current_track = sfx_jump;
        track_len = JUMP_LEN;
        break;
      case SFX_COIN:
        current_track = sfx_coin;
        track_len = COIN_LEN;
        break;
      case SFX_STOMP:
        current_track = sfx_stomp;
        track_len = STOMP_LEN;
        break;
      case SFX_HITBLOCK:
        current_track = sfx_hitblock;
        track_len = HITBLOCK_LEN;
        break;
      case SFX_DEATH:
        current_track = sfx_death;
        track_len = DEATH_LEN;
        break;
      case SFX_WIN:
        current_track = sfx_win;
        track_len = WIN_LEN;
        break;
      default:
        playing_sfx = false;
        play_bgm();
        break;
    }
  }

  void stop() {
    current_track = nullptr;
    track_len = 0;
    track_index = 0;
    playing_sfx = false;
    loop_bgm = false;
    stop_tone();
  }

  void tick() {
    if (current_track == nullptr || is_muted) return;

    unsigned long now = millis();
    if (now >= note_end_time) {
      if (track_index >= track_len) {
        // Track finished
        if (playing_sfx) {
          // Finished playing SFX, resume BGM
          playing_sfx = false;
          play_bgm();
        } else if (loop_bgm) {
          // Loop BGM
          track_index = 0;
        } else {
          stop();
          return;
        }
      }

      // Read next note from PROGMEM
      SoundNote note;
      memcpy_P(&note, &current_track[track_index], sizeof(SoundNote));

      if (note.freq == NOTE_REST) {
        stop_tone();
      } else {
        play_tone(note.freq, note.duration);
      }

      note_end_time = now + note.duration + 20; // 20ms gap for crisp sound articulation
      track_index++;
    }
  }

private:
  // Inductive reactance physical mitigation: Keep duty cycle very low (approx 1.5% - 4/255)
  // to prevent massive current draws, VCC power sags, and EMI which slow down/freeze the display.
  #define BUZZER_DUTY_CYCLE 4

  void play_tone(uint32_t freq, uint32_t duration) {
    if (is_muted) return;
    
#ifdef LEDC_NEW_API
    if (freq > 0) {
      ledcWriteTone(buzzer_pin, freq);
      ledcWrite(buzzer_pin, BUZZER_DUTY_CYCLE);
    } else {
      ledcWrite(buzzer_pin, 0);
    }
#else
    if (freq > 0) {
      ledcWriteTone(0, freq);
      ledcWrite(0, BUZZER_DUTY_CYCLE);
    } else {
      ledcWrite(0, 0);
    }
#endif
  }

  void stop_tone() {
#ifdef LEDC_NEW_API
    ledcWrite(buzzer_pin, 0);
#else
    ledcWrite(0, 0);
#endif
    digitalWrite(buzzer_pin, LOW); // Force low
  }
};

extern SoundEngine sound;

#endif
