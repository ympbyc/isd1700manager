/*
 * ISD1700 Manager sketch
 * Author: Minori Yamashita
 * Contact: ympbyc@gmail.com
 * 
 * Thanks! to following library authors:
 * ISD1700 lib by neuron_upheaval https://forum.arduino.cc/index.php?topic=52509.0
 * Timer lib by Simon Monk http://playground.arduino.cc/Code/Timer
 * OneButton lib by Matthias Hertel http://www.mathertel.de/Arduino/OneButtonLibrary.aspx
 */

#include <ISD1700.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <Timer.h>

#define SS 10
#define FIRST_ADDR 0x010
#define LAST_ADDR 0x2df
#define APC 0xE1 //000011100001
#define SLOT_NUM 5

#define MAIN_BTN 3
#define SUB_BTN  2

#define LEDI 14
#define LEDN 5
#define LEDFREQ 100
#define LEDREP 5

ISD1700 chip(SS); // Initialize chipcorder with
                 // SS at Arduino's digital pin 10

OneButton btn(MAIN_BTN, true);
OneButton btn2(SUB_BTN, true);

Timer tim;

uint16_t apc = 0;

uint16_t slot_addrs[SLOT_NUM];
uint16_t end_addrs[SLOT_NUM];
int cur_slot = 0;
int tim_id;

void setup() {
  int i;
  Serial.begin(9600);
  for (i=LEDI; i<LEDI+LEDN; i++)
    pinMode(i, OUTPUT);
  pinMode(MAIN_BTN, INPUT_PULLUP);
  pinMode(SUB_BTN, INPUT_PULLUP);

  btn.setClickTicks(300);
  btn.setPressTicks(400);
  btn2.setClickTicks(300);
  btn2.setPressTicks(1500); //press longer for g_erase
  btn.attachClick(single_click);
  btn.attachDoubleClick(double_click);
  btn.attachLongPressStart(long_press);
  btn.attachLongPressStop(long_press_end);
  btn2.attachClick(erase_one);
  btn2.attachLongPressStart(erase_all);
  digitalWrite(SS, HIGH); // just to ensure
  while (!chip_wakeup())
    delay(10);
  apc |= APC;
  chip.wr_apc2(apc);

  //erase
  /*
  chip.g_erase();
  for (i=0; i<SLOT_NUM; i++)
    EEPROM[i] = 0;
  chip_cleanup();
  */
  led_update();

  //setup memory management
  for (i=0; i<SLOT_NUM; i++) {
    slot_addrs[i] = floor((LAST_ADDR - FIRST_ADDR) / SLOT_NUM) * i + FIRST_ADDR + i;
    end_addrs[i] = floor((LAST_ADDR - FIRST_ADDR) / SLOT_NUM) * (i+1) + FIRST_ADDR + i;
  }
}

bool chip_wakeup () {
  // Power Up
  chip.rd_status();
  if (! chip.PU()) chip.pu();
  delay(10);
  chip.clr_int();
  chip.stop(); //stop whatever it's doing
  if (chip.RDY()) return true;
  return false;
}

bool chip_cleanup () {
  Serial.print("Status---> ");
  Serial.print(chip.CMD_ERR()? "CMD_ERR ": " ");
  Serial.print(chip.PU()? "PU ": " ");
  Serial.print(chip.RDY()? "RDY ": "Not_RDY");
  Serial.println();
  //delay(1);
  //chip.pd();
}

void single_click () {
  if ( ! chip_wakeup()) return;
  chip.set_play(slot_addrs[cur_slot], end_addrs[cur_slot]);
  tim.oscillate(LEDI+cur_slot, LEDFREQ, HIGH, LEDREP);
  chip_cleanup();
}

void double_click () {
  if ( ! chip_wakeup()) return;
  chip.stop();
  //chip.fwd();
  cur_slot = next_occupied_slot(cur_slot);
  chip.set_play(slot_addrs[cur_slot], end_addrs[cur_slot]);
  tim.oscillate(LEDI+cur_slot, LEDFREQ, HIGH, LEDREP);
  chip_cleanup();
}

void long_press () {
  int next_slot;
  if ( ! chip_wakeup()) return;
  next_slot = next_available_slot(cur_slot);
  if (next_slot < 0) return;
  chip.set_rec(slot_addrs[next_slot], end_addrs[next_slot]);
  tim_id = tim.oscillate(LEDI+next_slot, LEDFREQ, HIGH);
  EEPROM.write(next_slot, 1);
  cur_slot = next_slot;
}

void long_press_end () {
  led_update();
  chip_cleanup();
}

void erase_one () {
  if ( ! chip_wakeup()) return;
  chip.set_erase(slot_addrs[cur_slot], end_addrs[cur_slot]);
  EEPROM.write(cur_slot, 0);
  led_update();
  cur_slot = next_occupied_slot(cur_slot);
  chip_cleanup();
}

void erase_all () {
  int i;
  if ( ! chip_wakeup()) return;
  chip.g_erase();
  for (i=0; i<SLOT_NUM; i++)
    EEPROM[i] = 0;
  led_update();
  cur_slot = 0;
  chip_cleanup();
}

void led_update () {
  int i;
  tim.stop(tim_id);
  //led display update
  for (i=0; i<SLOT_NUM; i++) {
    if (EEPROM[i])
      digitalWrite(LEDI+i, HIGH);
    else
      digitalWrite(LEDI+i, LOW);
  }
}

int next_available_slot (int cur) {
  for (int i=0; i<SLOT_NUM; i++) {
    if ( ! EEPROM[i]) return i;
  }
  return -1;
}

int next_occupied_slot (int cur) {
  int nxt = (cur + 1) % SLOT_NUM;
  for (int i=nxt; i<SLOT_NUM+nxt; i++) {
    if (EEPROM[i % SLOT_NUM]) return i % SLOT_NUM;
  }
  return 0;
}

void loop() {
  btn.tick();
  btn2.tick();
  tim.update();
  delay(10);
}

