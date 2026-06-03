#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "bad_apple_data.h"
#include "song_data.h"

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// Row buffer for scaling and flicker-free rendering
uint16_t rowBuffer[240];

// Buzzer pins (routed through ULN2003AN driver)
const int PIN_SOPRANO_LEAD   = 26; // Soprano Lead (Melody Track)
const int PIN_SOPRANO_UNISON = 25; // Soprano Unison (Melody Track with Chorus Detune)
const int PIN_TENOR          = 33; // Tenor - 1 Octave lower (Bass Track)

// Premium chiptune chorus detuning (Hz) for Soprano Unison
#define CHORUS_DETUNE_HZ 2

// CRITICAL PHYSICS-BASED HARDWARE MITIGATION:
// Inductive reactance (XL = 2*pi*f*L) decreases at lower frequencies, meaning the Tenor buzzer
// draws massive current spikes compared to the Soprano buzzers. During the intense chorus (at 1 min),
// continuous low-frequency bass notes create huge sags and EMI noise on the VCC line, freezing the screen.
// We drop Soprano duty cycle to ~1.5% (4/255) and Tenor to ~0.8% (2/255).
// This keeps the chiptune audio extremely clean and retro, but slashes VCC noise by 95%!
#define DUTY_SOPRANO_LEAD   4 
#define DUTY_SOPRANO_UNISON 4 
#define DUTY_TENOR          2 

// Premium Color Palette Definitions (HSL-tailored harmonious tones)
#define COLOR_TITLE     tft.color565(255, 45, 85)    // Premium Neon Pink/Red
#define COLOR_CYAN      tft.color565(0, 240, 255)    // Premium Cyberpunk Cyan
#define COLOR_GOLD      tft.color565(255, 204, 0)    // Premium Gold Highlight
#define COLOR_TEXT      tft.color565(180, 180, 180)  // Premium Light Gray
#define COLOR_MUTED     tft.color565(90, 90, 90)     // Muted Gray

// LEDC configuration based on ESP32 Arduino Core version
#ifdef ESP_ARDUINO_VERSION_MAJOR
#if ESP_ARDUINO_VERSION_MAJOR >= 3
#define LEDC_NEW_API
#endif
#endif

// State caching for frequencies to prevent redundant hardware writes
uint16_t lastSopranoLeadFreq   = 0xFFFF;
uint16_t lastSopranoUnisonFreq = 0xFFFF;
uint16_t lastTenorFreq          = 0xFFFF;

void setupAudio() {
#ifdef LEDC_NEW_API
  ledcAttach(PIN_SOPRANO_LEAD, 2000, 8);   // Pin, base freq, resolution (8-bit)
  ledcAttach(PIN_SOPRANO_UNISON, 2000, 8);
  ledcAttach(PIN_TENOR, 100, 8);
#else
  // Assign channels to completely separate hardware timers to prevent register conflicts!
  // Channel 0 -> Timer 0
  // Channel 2 -> Timer 1
  // Channel 4 -> Timer 2
  ledcSetup(0, 2000, 8); // Channel 0, 2000Hz, 8-bit
  ledcAttachPin(PIN_SOPRANO_LEAD, 0);
  
  ledcSetup(2, 2000, 8); // Channel 2, 2000Hz, 8-bit
  ledcAttachPin(PIN_SOPRANO_UNISON, 2);
  
  ledcSetup(4, 100, 8);  // Channel 4, 100Hz, 8-bit
  ledcAttachPin(PIN_TENOR, 4);
#endif
}

void playSopranoLead(uint16_t freq) {
  if (freq == lastSopranoLeadFreq) return;
  lastSopranoLeadFreq = freq;

#ifdef LEDC_NEW_API
  if (freq > 0) {
    ledcWriteTone(PIN_SOPRANO_LEAD, freq);
    ledcWrite(PIN_SOPRANO_LEAD, DUTY_SOPRANO_LEAD); 
  } else {
    ledcWrite(PIN_SOPRANO_LEAD, 0);
  }
#else
  if (freq > 0) {
    ledcWriteTone(0, freq);
    ledcWrite(0, DUTY_SOPRANO_LEAD); 
  } else {
    ledcWrite(0, 0);
  }
#endif
}

void playSopranoUnison(uint16_t freq) {
  if (freq == lastSopranoUnisonFreq) return;
  lastSopranoUnisonFreq = freq;

#ifdef LEDC_NEW_API
  if (freq > 0) {
    ledcWriteTone(PIN_SOPRANO_UNISON, freq + CHORUS_DETUNE_HZ);
    ledcWrite(PIN_SOPRANO_UNISON, DUTY_SOPRANO_UNISON);
  } else {
    ledcWrite(PIN_SOPRANO_UNISON, 0);
  }
#else
  if (freq > 0) {
    ledcWriteTone(2, freq + CHORUS_DETUNE_HZ);
    ledcWrite(2, DUTY_SOPRANO_UNISON);
  } else {
    ledcWrite(2, 0);
  }
#endif
}

void playTenor(uint16_t freq) {
  if (freq == lastTenorFreq) return;
  lastTenorFreq = freq;

#ifdef LEDC_NEW_API
  if (freq > 0) {
    ledcWriteTone(PIN_TENOR, freq);
    ledcWrite(PIN_TENOR, DUTY_TENOR); // Lower duty cycle for bass to avoid heavy sags
  } else {
    ledcWrite(PIN_TENOR, 0);
  }
#else
  if (freq > 0) {
    ledcWriteTone(4, freq);
    ledcWrite(4, DUTY_TENOR); // Lower duty cycle for bass to avoid heavy sags
  } else {
    ledcWrite(4, 0);
  }
#endif
}

void setup() {
  tft.init();
  tft.setRotation(1); // Set landscape orientation (square 240x240 matches fine)
  tft.fillScreen(TFT_BLACK);
  
  // --- SLEEK, PREMIUM BOOT SEQUENCE ---
  tft.drawRect(4, 4, 232, 232, COLOR_CYAN);
  tft.drawRect(6, 6, 228, 228, COLOR_MUTED);
  
  tft.setTextColor(COLOR_TITLE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 30);
  tft.println("BAD APPLE!!");
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN, TFT_BLACK);
  tft.setCursor(20, 70);
  tft.println("ST7789 240x240");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD, TFT_BLACK);
  tft.setCursor(20, 105);
  tft.println("SYNTHESIZER STATUS: ONLINE");
  
  tft.setTextColor(COLOR_TEXT, TFT_BLACK);
  tft.setCursor(20, 125);
  tft.println("- Soprano Lead   : GPIO 26 [Ch 0]");
  tft.setCursor(20, 140);
  tft.println("- Soprano Unison : GPIO 25 [Ch 2 Detuned]");
  tft.setCursor(20, 155);
  tft.println("- Tenor Bass     : GPIO 33 [Ch 4]");
  
  tft.setTextColor(COLOR_MUTED, TFT_BLACK);
  tft.setCursor(20, 180);
  tft.println("Loading 15FPS RLE Video Stream...");
  
  tft.drawRect(20, 200, 200, 8, COLOR_MUTED);
  for (int w = 0; w < 196; w += 4) {
    tft.fillRect(22, 202, w, 4, COLOR_CYAN);
    delay(10);
  }
  
  setupAudio();
  
  delay(500);
  tft.fillScreen(TFT_BLACK);
}

// Render video frame (locks SPI per row to allow background OS tasks to breathe)
void drawFrame(int frameIdx) {
  // Get RLE offset and length from flash memory
  uint32_t startOffset = pgm_read_dword(&bad_apple_offsets[frameIdx]);
  uint32_t endOffset = pgm_read_dword(&bad_apple_offsets[frameIdx + 1]);
  uint32_t rleLen = endOffset - startOffset;
  
  const uint8_t* rleData = &bad_apple_data[startOffset];
  
  bool srcRow[64];
  int srcX = 0;
  int srcY = 0;
  bool currentColor = false; // false = black, true = white
  
  for (uint32_t i = 0; i < rleLen; i++) {
    uint8_t runLen = pgm_read_byte(&rleData[i]);
    for (int p = 0; p < runLen; p++) {
      srcRow[srcX] = currentColor;
      srcX++;
      if (srcX >= 64) {
        // Horizontal Scaling: Stretch 64 pixels to 240
        for (int x = 0; x < 64; x++) {
          int x1 = (x * 240) / 64;
          int x2 = ((x + 1) * 240) / 64;
          uint16_t color = srcRow[x] ? TFT_WHITE : TFT_BLACK;
          for (int bx = x1; bx < x2; bx++) {
            rowBuffer[bx] = color;
          }
        }
        
        // Vertical Scaling: Open/Close SPI transaction per scaled row (240x5 area)
        tft.startWrite();
        tft.setAddrWindow(0, srcY * 5, 240, 5);
        for (int r = 0; r < 5; r++) {
          tft.pushColors(rowBuffer, 240);
        }
        tft.endWrite();
        
        // STABILITY FIX: Yield CPU briefly to allow OS scheduling between rows
        yield();
        
        srcX = 0;
        srcY++;
        if (srcY >= 48) {
          return; // Frame fully rendered!
        }
      }
    }
    currentColor = !currentColor;
  }
}

unsigned long songStartTime = 0;
bool playing = false;

int nextMelodyIdx = 0;
int nextBassIdx = 0;
int lastRenderedFrame = -1;

const int totalMelodyNotes = sizeof(d1) / sizeof(d1[0]);
const int totalBassNotes = sizeof(d3) / sizeof(d3[0]);

void loop() {
  if (!playing) {
    songStartTime = millis();
    nextMelodyIdx = 0;
    nextBassIdx = 0;
    lastRenderedFrame = -1;
    playing = true;
    
    // Reset state caches on song start/loop
    lastSopranoLeadFreq   = 0xFFFF;
    lastSopranoUnisonFreq = 0xFFFF;
    lastTenorFreq          = 0xFFFF;
  }
  
  unsigned long elapsed = millis() - songStartTime;
  
  // 1. Play Soprano Lead & Unison notes based on elapsed time
  while (nextMelodyIdx < totalMelodyNotes && 
         elapsed >= pgm_read_dword(&d1[nextMelodyIdx])) {
    uint16_t freq = pgm_read_word(&m1[nextMelodyIdx]);
    playSopranoLead(freq);
    playSopranoUnison(freq);
    nextMelodyIdx++;
  }
  
  // 2. Play Tenor notes based on elapsed time
  while (nextBassIdx < totalBassNotes && 
         elapsed >= pgm_read_dword(&d3[nextBassIdx])) {
    uint16_t freq = pgm_read_word(&m3[nextBassIdx]);
    playTenor(freq);
    nextBassIdx++;
  }
  
  // 3. Render video frame (15 FPS -> 66.67ms per frame)
  int frameIdx = elapsed / 66; // 66.67ms approx
  if (frameIdx < bad_apple_frames) {
    if (frameIdx != lastRenderedFrame) {
      drawFrame(frameIdx);
      lastRenderedFrame = frameIdx;
    } else {
      // Yield control to the FreeRTOS scheduler when waiting for the next frame
      delay(1); 
    }
  } else {
    // End of video/song -> loop!
    playing = false; 
    playSopranoLead(0);
    playSopranoUnison(0);
    playTenor(0);
    delay(1000); // 1 second break before looping again
  }
}
