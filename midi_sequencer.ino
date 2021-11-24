/*
  Arduino sequencer - Jake Bowden
  Circuit: USB power, 128x32 display on A4 and A5, multiplexer w. 10 potentiometers on 8, 9, 10, 11 and A0
  MIDI out: 5V-res(220) -> midi 4, TX(1) -> midi 5, gnd -> midi 2
*/

#include <stdio.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <MIDI.h>

Adafruit_SSD1306 display(128, 32, &Wire, -1);
MIDI_CREATE_DEFAULT_INSTANCE();

// Pinout:
const int muxOut_S0 = 8;
const int muxOut_S1 = 9;
const int muxOut_S2 = 10;
const int muxOut_S3 = 11;
const int muxIn_SIG = A0;
const int toneOut = 7;

// Mux init:
const int muxPinN = 10;
int muxState[10] = {0};

// Sequencer init:
int seqStep = 0;
int seqLen = 8;
int seqNotes[8] = {1};
int seqBpm = 120;
int stepDelay = 60000 / seqBpm;
unsigned long delayStart = 0;

// Settings init:
int settingsMode = 0;
String settingsTitles[5] = {"Length", "BPM", "Channel"};
String activeParam = "";
int paramPotVal = 0;
int oldPotVal = 0;

// MIDI Settings: 
int midiChan = 1;
int midiNote = 1;

void setup() {
  // Mux pin settings:
  pinMode(muxOut_S0, OUTPUT);
  pinMode(muxOut_S1, OUTPUT);
  pinMode(muxOut_S2, OUTPUT);
  pinMode(muxOut_S3, OUTPUT);
  pinMode(muxIn_SIG, INPUT);

  // Can output audio level on PWM~ digital pins:
  // pinMode(toneOut, OUTPUT);

  // Begin display:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Step timer delay sart:
  delayStart = millis();

  // Serial output for debugging:
//  Serial.begin(9600);

  // MIDI:
  MIDI.begin(midiChan);
}

void updateMux() {
  // Read analog in pins from multiplex chip:
  for (int i = 0; i < muxPinN; i++) {
    digitalWrite(muxOut_S0, HIGH && (i & B00000001));
    digitalWrite(muxOut_S1, HIGH && (i & B00000010));
    digitalWrite(muxOut_S2, HIGH && (i & B00000100));
    digitalWrite(muxOut_S3, HIGH && (i & B00001000));
    int potVal = analogRead(muxIn_SIG);
    if (abs(potVal - muxState[i]) >= 5) {
      muxState[i] = potVal;
    }
  }
}

void updateSettings() {
  // Change which settings are being displayed and changed, update relevant setting:
  settingsMode = map(muxState[8], 0, 1023, 0, 2);

  // Only change param if pot is turned:
  paramPotVal = muxState[9];
  if (abs(paramPotVal - oldPotVal) >= 5) {
    // Sequence length:
    if (settingsMode == 0) {
      seqLen = map(paramPotVal, 0, 1023, 1, 8);
      activeParam = String(seqLen);
      oldPotVal = paramPotVal;
    }
    // BPM:
    else if (settingsMode == 1) {
      seqBpm = map(paramPotVal, 0, 1023, 10, 600);
      stepDelay = 60000 / seqBpm;
      activeParam = String(seqBpm);
      oldPotVal = paramPotVal;
    }
    else if (settingsMode == 2) {
      midiChan = map(paramPotVal, 0, 1023, 1, 10);
      activeParam = String(midiChan);
      oldPotVal = paramPotVal;
      MIDI.begin(midiChan);
    }
  }

  // Select which parameter is drawn to the screen:
  if (settingsMode == 0) {
    activeParam = String(seqLen);
  }
  else if (settingsMode == 1) {
    activeParam = String(seqBpm);
  }
  else if (settingsMode == 2) {
    activeParam = String(midiChan);
  }
 }

void updateSeq() {
  // Using mux readings assign notes to seq steps:
  for (int i = 0; i < 8; i++) {
    seqNotes[i] = map(muxState[i], 0, 1023, 0, 100);
  }
}

void sendMidiSig(){
  // Send last note off:
  MIDI.sendNoteOff(midiNote, 0, midiChan);
  // Send note on:
  midiNote = seqNotes[seqStep];
  MIDI.sendNoteOn(midiNote, 127, midiChan);
 }

void drawDisplay() {
  // Clear display:
  display.fillScreen(SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Draw step sequencer boxes and notes:
  for (int i = 0; i < seqLen; i++) {
    display.drawRect(i * 16, 0, 16, 16, SSD1306_WHITE);
  }
  for (int i = 0; i < 8; i++) {
    display.setCursor(i * 16 + 3, 4);
    display.print(seqNotes[i]);
  }

  // Draw active step box:
  display.fillRect(seqStep * 16, 0, 16, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(seqStep * 16 + 3, 4);
  display.print(seqNotes[seqStep]);

  // Draw Setting and parameter:
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 24);
  display.print("< ");
  display.print(settingsTitles[settingsMode]);
  display.print(" >");

  display.setCursor(80, 24);
  display.print("< ");
  display.print(activeParam);
  display.print(" >");

  // Draw to the display:
  display.display();
}

void stepSequencer() {
  // See if the interval for the sequencer has passed, if it has, move to next step:
  if (millis() - delayStart >= stepDelay) {
    delayStart += stepDelay;
    seqStep += 1;
    if (seqStep >= seqLen) {
      seqStep = 0;
    }
    sendMidiSig();
  }
}

void loop() {
  updateMux();
  updateSeq();
  updateSettings();
  drawDisplay();
  stepSequencer();
}
