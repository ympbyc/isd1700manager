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

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <prescaler.h>
#include <EEPROM.h>
#include <ISD1700.h>
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

#define REC_TIMEUP ((90 * 1000) / SLOT_NUM)
#define AVR_PWR_DWN_TIME 3000

int interrupt_main_btn = digitalPinToInterrupt(MAIN_BTN);
int interrupt_sub_btn = digitalPinToInterrupt(SUB_BTN);

ISD1700 chip(SS); // Initialize chipcorder with
                 // SS at Arduino's digital pin 10

OneButton btn(MAIN_BTN, true);
OneButton btn2(SUB_BTN, true);

Timer tim;
int tim_id;
int c_w_t_id;
int r_tu_t_id;
int avr_pd_t_id;

uint16_t apc = 0;

uint16_t slot_addrs[SLOT_NUM];
uint16_t end_addrs[SLOT_NUM];
int cur_slot = 0;

void setup() {
  int i;
  Serial.begin(115200);
  /* lower power consumption */
  DDRD &= B00000011;       // set Arduino pins 2 to 7 as inputs
  DDRB = B00000000;        // set pins 8 to 13 as inputs
  PORTD |= B11111100;      // enable pullups on pins 2 to 7
  PORTB |= B11111111;      // enable pullups on pins 8 to 13
  setClockPrescaler(CLOCK_PRESCALER_1); //8MHz
  
  // re-set pin-mode for SPI after above setting
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(SS, OUTPUT);

  for (i=LEDI; i<LEDI+LEDN; i++)
    pinMode(i, OUTPUT);
  pinMode(MAIN_BTN, INPUT_PULLUP);
  pinMode(SUB_BTN, INPUT_PULLUP);

  btn.setClickTicks(300);
  btn.setPressTicks(400);
  btn2.setClickTicks(300);
  btn2.setPressTicks(1500); //press longer for g_erase
  btn.attachClick(playback);
  btn.attachDoubleClick(forward);
  btn.attachLongPressStart(rec);
  btn.attachLongPressStop(rec_end);
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
  tim.stop(avr_pd_t_id); //clear pwrdwn timeout
  Serial.println("ISD1700 wakeup");
  chip.rd_status();
  if (! chip.PU()) chip.pu();
  delay(10);
  chip.stop(); //stop whatever it's doing
  chip.clr_int();
  if (chip.RDY()) return true;
  return false;
}

bool chip_cleanup () {
  tim.stop(c_w_t_id);
  Serial.print("Status---> ");
  Serial.print(chip.CMD_ERR()? "CMD_ERR ": " ");
  Serial.print(chip.PU()? "PU ": " ");
  Serial.print(chip.RDY()? "RDY ": "Not_RDY");
  Serial.println();
  c_w_t_id = tim.every(1000, attempt_pd);
}

void attempt_pd () {
  chip.rd_status();
  if (chip.INT() || chip.EOM()) {
    tim.stop(c_w_t_id);
    chip.pd();
    Serial.println(chip.CMD_ERR() ? "pd CMD_ERR" : "ISD1700 powerdown");
    avr_pd_t_id = tim.after(AVR_PWR_DWN_TIME, avr_sleep);
    Serial.print("PWRDWN Timer id: "); Serial.println(avr_pd_t_id);
  }
}

void playback () {
  if ( ! EEPROM[cur_slot]) return;
  if ( ! chip_wakeup()) return;
  chip.set_play(slot_addrs[cur_slot], end_addrs[cur_slot]);
  tim.stop(tim_id);
  tim_id = tim.oscillate(LEDI+cur_slot, LEDFREQ, HIGH, LEDREP);
  chip_cleanup();
}

void forward () {
  if ( ! chip_wakeup()) return;
  chip.stop();
  //chip.fwd();
  cur_slot = next_occupied_slot(cur_slot);
  chip.set_play(slot_addrs[cur_slot], end_addrs[cur_slot]);
  tim.stop(tim_id);
  tim_id = tim.oscillate(LEDI+cur_slot, LEDFREQ, HIGH, LEDREP);
  chip_cleanup();
}

void rec () {
  int next_slot;
  if ( ! chip_wakeup()) return;
  next_slot = next_available_slot(cur_slot);
  if (next_slot < 0) return;
  chip.set_rec(slot_addrs[next_slot], end_addrs[next_slot]);
  tim.stop(tim_id);
  tim_id = tim.oscillate(LEDI+next_slot, LEDFREQ, HIGH);
  EEPROM.write(next_slot, 1);
  cur_slot = next_slot;
  r_tu_t_id = tim.after(REC_TIMEUP, rec_end); //ensure rec_end gets called
}

void rec_end () {
  Serial.println("rec end");
  tim.stop(tim_id);
  tim.stop(r_tu_t_id);
  chip.stop();
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

void led_off () {
  int i;
  for (i=0; i<SLOT_NUM; i++)
    digitalWrite(LEDI+i, LOW);
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

void pinInterrupt () {
  sleep_disable(); //Wait for an interrupt, disable, and continue
  detachInterrupt(interrupt_main_btn);
  detachInterrupt(interrupt_sub_btn);
}

void avr_sleep () {
  Serial.println("avr_sleep reached");
  if (digitalRead(MAIN_BTN) == LOW || digitalRead(SUB_BTN) == LOW) return;
  sleep_enable();  //Set sleep enable (SE) bit
  attachInterrupt(interrupt_main_btn, pinInterrupt, LOW);
  attachInterrupt(interrupt_sub_btn,  pinInterrupt, LOW);
  delay(100);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  led_off();
  Serial.println("Arduino PWR_DOWN");
  delay(10);
  sleep_mode();    //Put the device to sleep
  
  sleep_disable(); //Wait for an interrupt, disable, and continue
  Serial.println("Arduino Wakeup");
  led_update();
}

void loop() {
  btn.tick();
  btn2.tick();
  tim.update();
  delay(10);
}

