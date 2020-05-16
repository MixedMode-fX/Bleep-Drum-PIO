/*
 * The Bleep Drum
 * By John-Mike Reed aka Dr. Bleep
 * https://bleeplabs.com/product/the-bleep-drum/
 *  
 * Updated version for April 2020 rerelease
 * 
 * 
 * Adapted for Platform.IO by Adrien Fauconnet
 *  
 */

#include <Arduino.h>

// SAMPLES DATA
#include "sine.h"
#ifdef DAM
#include "samples_dam.h"
#elif DAM2
#include "samples_dam2.h"
#elif DAM3
#include "samples_dam3.h"
#else
#include "samples_bleep.h"
#endif

// DEPENDENCIES

#include <avr/pgmspace.h>
#include <SPI.h>
#include <Bounce2.h>
#define BOUNCE_LOCK_OUT

// use ENABLE_MIDI in build flags to enable MIDI
#ifdef ENABLE_MIDI 
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#endif


// PINOUT
#define red_pin 2
#define blue_pin 7
#define green_pin 3
#define yellow_pin 4
#define PLAY 8    // D8
#define REC 19    // A5
#define TAP 18    // A4
#define SHIFT 17  // A3
#define LED_RED 6
#define LED_GREEN 5
#define LED_BLUE 9
#define POT_LEFT 0
#define POT_RIGHT 1
#define AUDIO_CLOCK 12
#define DAC_CS 10

// INIT
Bounce debouncerRed = Bounce();
Bounce debouncerGreen = Bounce();
Bounce debouncerBlue = Bounce();
Bounce debouncerYellow = Bounce();

uint32_t cm, pm;
const char noise_table[] PROGMEM = {};
const unsigned long dds_tune = 4294967296 / 9800; // 2^32/measured dds freq but this takes too long
int sample_holder1, sample_holder2;
int eee;
byte erase_latch;
byte shift, bankpg, bankpr, bout, rout, gout, prevpot2;
byte banko = 0;
byte n1, n2;
int n3;
byte bankpb = 4;
byte beat;
uint16_t pot1, pot2;
long pot3 = 200, pot4 = 200;
int kick_sample, snare_sample, sample, hat_sample, noise_sample, bass_sample, B2_freq_sample, B1_freq_sample;
uint16_t increment, increment2, increment3, increment4, increment5, increment2v, increment4v;
uint32_t accumulator, accumulator2, accumulator3, accumulator4, accumulator5, accu_freq_1, accu_freq_2;
int rando;
byte B2_sequence[64] = {};
byte B3_sequence[64] = {};
byte B1_sequence[64] = {};
byte B4_sequence[64] = {};
int B2_freq_sequence[64] = {};
int B1_freq_sequence[64] = {};
int sample_sum;
int j, k, freq3, cc;
int kf, pf, holdkf, kfe;
int shiftcount = 0;
int t1, c1, count1, dd;
byte noise_type;
uint16_t index, index2, index3, index4, index5, index4b, index_freq_1, index_freq_2, index4bv;
uint16_t indexr, index2r, index3r, index4r, index4br, index2vr, index4vr;
int osc, oscc;
byte ledstep;
byte noise_mode = 1;
unsigned long freq, freq2;
int wavepot, lfopot, arppot;
byte loopstep = 0;
byte loopstepf = 0;
byte recordbutton, prevrecordbutton, record, looptrigger, prevloopstep, revbutton, prevrevbutton, preva, prevb;
int looprate;
long prev, prev2, prev3;
byte playmode = 1;
byte play = 0;
byte playbutton, pplaybutton;
byte prevbanktrigger, banktrigger;
byte pkbutton, kbutton, B4_trigger, B4_latch, cbutton, pcbutton, B4_loop_trigger, B1_trigger, kick, B1_latch, clap, B1_loop_trigger, B4_seq_trigger, B3_seq_trigger;
byte ptbutton, tbutton, ttriger, B1_seq_trigger, B3_latch, B2_trigger, bc, B2_loop_trigger, B3_loop_trigger;
byte B2_latch, B3_trigger, B2_seq_trigger, pbutton, ppbutton;
byte kicktriggerv, B2_seq_latch, kickseqtriggerv,  B1_seq_latch, pewseqtriggerv, precordbutton;
byte measure, half;
byte recordmode = 1;
byte tap, tapbutton, ptapbutton, eigth;
long tapholder, prevtap;
unsigned long taptempo = 1000;
unsigned long ratepot;
byte r, g, b, erase, e, preveigth;
byte trigger_input, trigger_output,   trigger_out_latch, tl;
byte button1, button2, button3, button4, tapb;
byte pbutton1, pbutton2, pbutton3, pbutton4, ptapb;
byte prev_trigger_in_read, trigger_in_read, tiggertempo, trigger_step, triggerled, ptrigger_step;
byte bf1, bf2, bf3, bf4, bft;
uint16_t midicc3 = 128;
uint16_t midicc4 = 157;
uint16_t midicc1, midicc2, midicc5, midicc6, midicc7, midicc8;
byte midi_note_check;
byte prevshift, shift_latch;
byte tick;
byte t;
long tapbank[4];
byte  mnote, mvelocity, miditap, pmiditap, miditap2, midistep, pmidistep, miditempo, midinoise;
unsigned long recordoffsettimer, offsetamount, taptempof;
int potX;
int click_pitch;
byte click_amp;
int click_wait;
int sample_out_temp;
byte sample_out;
uint32_t dds_time;
byte bft_latch;
unsigned long raw1, raw2;
unsigned long log1, log2;
byte click_play, click_en;
int shift_time;
int shift_time_latch;
byte printer = 0;
uint32_t erase_led;

int midi_note_on();
void LEDS();
void BUTTONS();
void RECORD();

void setup() {
  cli();

  pinMode (AUDIO_CLOCK, OUTPUT); pinMode (DAC_CS, OUTPUT);
  pinMode (LED_BLUE, OUTPUT); pinMode (LED_RED, OUTPUT);  pinMode (LED_GREEN, OUTPUT);

  pinMode (PLAY, INPUT);     digitalWrite(PLAY, HIGH);  //play
  pinMode (REC, INPUT);     digitalWrite (REC, HIGH); //rec
  pinMode (TAP, INPUT);     digitalWrite (TAP, HIGH); //tap
  pinMode (SHIFT, INPUT);     digitalWrite(SHIFT, HIGH);   //shift

  pinMode (green_pin, INPUT_PULLUP);   //low left clap green
  pinMode (yellow_pin, INPUT_PULLUP);   // low right kick yellow
  pinMode (blue_pin, INPUT_PULLUP);    //Up Right tom Blue
  pinMode (red_pin, INPUT_PULLUP);   // Up right pew red

  debouncerGreen.attach(green_pin);
  debouncerGreen.interval(2); // interval in ms
  debouncerYellow.attach(yellow_pin);
  debouncerYellow.interval(2); // interval in ms
  debouncerBlue.attach(blue_pin);
  debouncerBlue.interval(2); // interval in ms
  debouncerRed.attach(red_pin);
  debouncerRed.interval(2); // interval in ms

  delay(100);
  #ifdef ENABLE_MIDI
    if (digitalRead(green_pin) == LOW) {
      analogWrite(LED_GREEN, 64); //green
      MIDI.begin(3);
      delay(20000);

    }
    else if (digitalRead(red_pin) == LOW) {
      analogWrite(LED_RED, 64); //RED
      MIDI.begin(1);
      delay(20000); // we're messing with the timers so this isn't actually 20000 Millis

    }
    else if (digitalRead(blue_pin) == LOW) {
      analogWrite(LED_BLUE, 64); //Blue
      MIDI.begin(2);
      delay(20000);

    }
    else if (digitalRead(yellow_pin) == LOW) {
      analogWrite(LED_RED, 48); //yellow
      analogWrite(LED_GREEN, 16);

      MIDI.begin(4);
      delay(20000);

    }

    else {
      MIDI.begin(0);
    }

    MIDI.turnThruOff();

  #endif

  //pinMode (16, INPUT); digitalWrite (16, HIGH);
  digitalWrite(16, HIGH); //
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);

  /* Enable interrupt on timer2 == 127, with clk/8 prescaler. At 16MHz,
     this gives a timer interrupt at 15625Hz. */


  TIMSK2 = (1 << OCIE2A);
  OCR2A = 50; // sets the compare. measured at 9813Hz

  TCCR2A = 1 << WGM21 | 0 << WGM20; /* CTC mode, reset on match */
  TCCR2B = 0 << CS22 | 1 << CS21 | 1 << CS20; /* clk, /8 prescaler */


  TCCR0B = B0000001;
  TCCR1B = B0000001;



  sei();
  if (digitalRead(SHIFT) == 0) {
    noise_mode = 1;
  }
  else {
    noise_mode = 0;
  }


}

byte out_test, out_tick = 1;
int sine_sample;
byte index_sine;
uint32_t acc_sine;
uint32_t trig_out_time, trig_out_latch;
ISR(TIMER2_COMPA_vect) {
  dds_time++;
  OCR2A = 50;

  prevloopstep = loopstep;
  preva = eigth;
  if (recordmode == 1 && miditempo == 0 && tiggertempo == 0) {
    if (dds_time - prev > (taptempof) ) {
      prev = dds_time;
      loopstep++;
      if (loopstep > 31)
      {
        loopstep = 0;
      }
    }
  }

  if (shift == 1) {
    if (bft == 1) {
      bft = 0;
      bft_latch = 0;
      tiggertempo = 0;
      t++;
      t %= 2;
      tapbank[t] = (dds_time - prevtap) >> 2;
      taptempo = ((tapbank[0] + tapbank[1]) >> 1);
      prevtap = dds_time;
    }
  }

  taptempof = taptempo;

  recordoffsettimer = dds_time - prev ;
  offsetamount = taptempof - (taptempof >> 2 );

  if ((recordoffsettimer) > (offsetamount))
  {
    loopstepf = loopstep + 1;
    if (loopstepf > 31) {
      loopstepf = 0;
    }
  }

  if (loopstep % 4 == 0) {
    click_pitch = 220;
  }
  if (loopstep % 8 == 0) {
    click_pitch = 293;
  }

  if (loopstep == 0) {
    click_pitch = 440;
  }

  if (play == 1) {
    click_play = 1;
    B4_loop_trigger = B4_sequence[loopstep + banko];
    B1_loop_trigger = B1_sequence[loopstep + banko];
    B2_loop_trigger = B2_sequence[loopstep + banko];
    B3_loop_trigger = B3_sequence[loopstep + banko];
  }

  if (play == 0) {
    loopstep = 31;
    prev = 0;
    click_play = 0;
    B4_loop_trigger = 0;
    B1_loop_trigger = 0;
    B2_loop_trigger = 0;
    B3_loop_trigger = 0;
  }

  if (loopstep != prevloopstep) {
    digitalWrite(12, 1);
    trig_out_time = dds_time;
    trig_out_latch = 1;
  }
  if (dds_time - trig_out_time > 80 && trig_out_latch == 1) {
    trig_out_latch = 0;
    digitalWrite(12, 0);
  }

  if (loopstep != prevloopstep && B3_loop_trigger == 1) {
    B3_seq_trigger = 1;
  }
  else {
    B3_seq_trigger = 0;
  }

  if (loopstep != prevloopstep && B2_loop_trigger == 1) {
    B2_seq_trigger = 1;
  }
  else {
    B2_seq_trigger = 0;
  }

  if (loopstep != prevloopstep && B4_loop_trigger == 1) {
    B4_seq_trigger = 1;
  }
  else {
    B4_seq_trigger = 0;
  }

  if (loopstep != prevloopstep && B1_loop_trigger == 1) {
    B1_seq_trigger = 1;
  }
  else {
    B1_seq_trigger = 0;
  }

  if (B3_trigger == 1 || B3_seq_trigger == 1) {
    index3 = 0;
    accumulator3 = 0;
    B3_latch = 1;
  }

  if (B4_trigger == 1 || B4_seq_trigger == 1) {
    index4 = 0;
    accumulator4 = 0;
    B4_latch = 1;
  }
  if (B1_trigger == 1) {
    index = 0;
    accumulator = 0;
    B1_latch = 1;
  }
  if (B1_seq_trigger == 1) {
    index_freq_1 = 0;
    accu_freq_1 = 0;
    B1_seq_latch = 1;
  }
  if (B2_seq_trigger == 1) {
    index_freq_2 = 0;
    accu_freq_2 = 0;
    B2_seq_latch = 1;
  }
  if (B2_trigger == 1) {
    index2 = 0;
    accumulator2 = 0;
    B2_latch = 1;
  }

  if (loopstep % 4 == 0 && prevloopstep % 4 != 0) {
    click_amp = 64;
  }

  sine_sample = (((pgm_read_byte(&sine_table[index_sine]) - 127) * click_amp) >> 8) * click_play * click_en;
  //sine_sample = ((sine_table[index_sine]));

  if (playmode == 0) { //reverse
    snare_sample = (pgm_read_byte(&table2[(indexr)])) - 127;
    kick_sample = (pgm_read_byte(&table3[(index2r)])) - 127;
    hat_sample = (pgm_read_byte(&table0[(index3r)])) - 127;
    bass_sample = (((pgm_read_byte(&table1[(index4r)])))) - 127;

    B1_freq_sample = pgm_read_byte(&table0[(index2vr)]) - 127;
    B2_freq_sample = (pgm_read_byte(&table1[(index4vr)])) - 127;
  }

  if (playmode == 1) {
    snare_sample = (pgm_read_byte(&table2[(index3)])) - 127;
    kick_sample = (pgm_read_byte(&table3[(index4)])) - 127;
    hat_sample = (pgm_read_byte(&table0[(index)])) - 127;
    bass_sample = (((pgm_read_byte(&table1[(index2)])))) - 127;
    B1_freq_sample = pgm_read_byte(&table0[(index_freq_1)]) - 127;
    B2_freq_sample = (pgm_read_byte(&table1[(index_freq_2)])) - 127;

    noise_sample = (((pgm_read_byte(&sine_table[(index5)])))) - 127;

  }

  if (noise_mode == 1) {
    sample_sum = (snare_sample + kick_sample + hat_sample + bass_sample + B1_freq_sample + B2_freq_sample);
    byte sine_noise = ((sine_sample) | (noise_sample >> 2) * click_amp >> 2) >> 3;

    if (click_play == 0 | click_en == 0 | click_amp < 4) {
      sine_noise = 0;
    }

    sample_holder1 = ((sample_sum ^ (noise_sample >> 1)) >> 1) + 127;
    if (B1_latch == 0 && B2_latch == 0  && B3_latch == 0  && B4_latch == 0 ) {
      sample_out_temp = 127  + sine_noise ;
    }
    else {
      sample_out_temp = sample_holder1   + sine_noise ;
    }
  }

  if (noise_mode == 0) {
    sample_out_temp = ((snare_sample + kick_sample + hat_sample + bass_sample + B1_freq_sample + B2_freq_sample + sine_sample) >> 1) + 127;
  }
  if (sample_out_temp > 255) {
    sample_out_temp -= (sample_out_temp - 255) << 1; //fold don't clip!
  }
  if (sample_out_temp < 0) {
    sample_out_temp += sample_out_temp * -2;
  }
  sample_out = sample_out_temp;

  uint16_t dac_out = (0 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | ( sample_out << 4 );
  digitalWrite(DAC_CS, LOW);
  SPI.transfer(dac_out >> 8);
  SPI.transfer(dac_out & 255);
  digitalWrite(DAC_CS, HIGH);

  ///////////////////////////////////////////////////////////////////////////////

  acc_sine += click_pitch << 2 ;
  index_sine = (dds_tune * acc_sine) >> (32 - 8);
  //  index_sine = (acc_sine << 15) >> (32 - 8); //fast and the pitches arent critical
  click_wait++;
  if (click_wait > 4) {
    click_wait = 0;

    if (click_amp >= 4) {
      click_amp -= 1;
    }
    if (click_amp < 4) {
      click_amp = 0;
    }
  }

  if (playmode == 0) {
    indexr = (index3 - length2) * -1;
    index2r = (index4 - length3) * -1;
    index3r = (index - length0) * -1;
    index4r = (index2 - length1) * -1;
    index2vr = (index_freq_1 - length0) * -1;
    index4vr = (index_freq_2 - length1) * -1;

  }

  if (B1_latch == 1) {
    if (midicc1 > 4) {
      accumulator += midicc1;
    }
    else {
      accumulator += pot1;
    }
    index = (accumulator >> (6));
    if (index > length0) {
      index = 0;
      accumulator = 0;
      B1_latch = 0;
    }
  }

  if (B2_latch == 1) {
    if (midicc2 > 4) {
      accumulator2 += midicc2;
    }
    else {
      accumulator2 += pot2;
    }
    index2 = (accumulator2 >> (6));
    if (index2 > length1) {
      index2 = 0;
      accumulator2 = 0;
      B2_latch = 0;
    }
  }

  if (B3_latch == 1) {
    accumulator3 += (midicc3);
    index3 = (accumulator3 >> 6);
    if (index3 > length2) {

      index3 = 0;
      accumulator3 = 0;
      B3_latch = 0;
    }
  }

  if (B4_latch == 1) {
    accumulator4 += (midicc4);
    // index4b=(accumulator4 >> (6));
    index4 = (accumulator4 >> (6));
    if (index4 > length3) {

      index4 = 0;
      accumulator4 = 0;
      B4_latch = 0;
    }
  }

  accu_freq_1 += kf;
  index_freq_1 = (accu_freq_1 >> (6));
  if (B1_seq_trigger == 1) {
    kf = B1_freq_sequence[loopstep + banko];
    kfe = kf;
  }

  if (index_freq_1 > length0) {
    kf = 0;
    index_freq_1 = 0;
    accu_freq_1 = 0;
    B1_seq_latch = 0;
  }


  accu_freq_2 += pf;
  index_freq_2 = (accu_freq_2 >> (6));

  if (B2_seq_trigger == 1 && tiggertempo == 0) {
    pf = B2_freq_sequence[loopstepf + banko];
  }

  if (B2_seq_trigger == 1 && tiggertempo == 1) {
    pf = B2_freq_sequence[loopstep + banko];
  }
  if (index_freq_2 > length1) {
    pf = 0;
    index_freq_2 = 0;
    accu_freq_2 = 0;
    B2_seq_latch = 0;
  }

  if (noise_mode == 1) {
    accumulator5 += (pot3);
    index5 = (accumulator5 >> (6));
    if (index5 > pot4) {
      index5 = 0;
      accumulator5 = 0;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////loop

void loop() {

  cm = millis();

  midi_note_check = midi_note_on();

  pbutton1 = button1;
  pbutton2 = button2;
  pbutton3 = button3;
  pbutton4 = button4;

  debouncerRed.update();
  debouncerBlue.update();
  debouncerGreen.update();
  debouncerYellow.update();

  button1 = debouncerRed.read();
  button2 = debouncerBlue.read();
  button3 = debouncerGreen.read();
  button4 = debouncerYellow.read();

  tapb = digitalRead(TAP);

  if (button1 == 0 && pbutton1 == 1) {
    bf1 = 1;
  }
  else {
    bf1 = 0;
  }
  if (button2 == 0 && pbutton2 == 1) {
    bf2 = 1;
  }
  else {
    bf2 = 0;
  }

  if (button3 == 0 && pbutton3 == 1) {
    bf3 = 1;
  }
  else {
    bf3 = 0;
  }
  if (button4 == 0 && pbutton4 == 1) {
    bf4 = 1;
  }
  else {
    bf4 = 0;
  }
  if (tapb == 0 && ptapb == 1 && bft_latch == 0) {
    bft = 1;
    bft_latch = 1;
  }
  else {
    bft = 0;
    bft_latch = 0;
  }

  if (midi_note_check == 67) {
    play++;
    play %= 2;
  }

  if (midi_note_check == 69) {
    playmode++;
    playmode %= 2;
  }

  if (midi_note_check == 70) {
    midinoise = 1;
    shift_latch = 1;
    noise_mode++;
    noise_mode %= 2;
  }

  if (midi_note_check == 72) {
    banko = 0; //blue
  }
  if (midi_note_check == 74) {
    banko = 31; // yellow
  }
  if (midi_note_check == 76) {
    banko = 63; //red
  }
  if (midi_note_check == 77) {
    banko = 95; //green
  }

  ptapb = tapb;
  pmiditap = miditap;
  pmidistep = midistep;

  LEDS();
  BUTTONS();
  RECORD();

  raw1 = (analogRead(POT_LEFT) - 1024) * -1;
  log1 = raw1 * raw1;
  raw2 = (analogRead(POT_RIGHT) - 1024) * -1;
  log2 = raw2 * raw2;

  if (noise_mode == 0) {
    pot1 = (log1 >> 11) + 2;
    pot2 = (log2 >> 11) + 42;
  }

  if (noise_mode == 1) {

    if (shift_latch == 0) {
      pot1 = (log1 >> 11) + 1;
      pot2 = (log2 >> 12) + 1;
    }
    if (shift_latch == 1) {
      pot3 = (log1 >> 6) + 1;
      pot4 = (log2 >> 8) + 1;
    }
  }

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RECORD() {
  pplaybutton = playbutton;
  playbutton = digitalRead(PLAY);
  if (pplaybutton == 1 && playbutton == 0 && shift == 1 && recordbutton == 1 ) {
    play = !play;
  }


  precordbutton = recordbutton;
  recordbutton = digitalRead(REC);
  if (recordbutton == 0 && precordbutton == 1) {
    record = !record;
    play = 1;
    erase_latch = 1;
    eee = 0;
  }
  if (recordbutton == 1 && precordbutton == 0) {
    erase_latch = 0;
  }


  if (play == 0) {
    record = 0;
  }


  ////////////////////////////////////////////////////////////////////erase

  if (recordbutton == 0) {
    if (playbutton == 0 &&  erase_latch == 1) {
      eee++;
      if (eee >= 800) {
        eee = 0;
        erase_latch = 0;
        erase = 1;
        erase_led = millis();
        play = 1;
        record = 0;
        for (byte j; j < 32; j++) {
          B1_sequence[j + banko] = 0;
          B2_sequence[j + banko] = 0;
          B4_sequence[j + banko] = 0;
          B3_sequence[j + banko] = 0;
        }
      }
    }
  }

  if (millis() - erase_led > 10000) {
    erase = 0;
  }


  if (record == 1)
  {
    if (B1_trigger == 1) {
      B1_sequence[loopstepf + banko] = 1;
      B1_freq_sequence[loopstepf + banko] = pot1;

    }

    if (B2_trigger == 1) {
      B2_sequence[loopstepf + banko] = 1;
      B2_freq_sequence[loopstepf + banko] = (pot2);
    }

    if (B4_trigger == 1) {
      B4_sequence[loopstepf + banko] = 1;
    }

    if (B3_trigger == 1) {
      B3_sequence[loopstepf + banko] = 1;
    }


  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LEDS() {

  analogWrite(LED_BLUE, bout >> 1); //Blue
  analogWrite(LED_GREEN, (gout >> 1) + triggerled); //green
  analogWrite(LED_RED, rout >> 1);

  if (noise_mode == 1) {
    rout = r;
    gout = g;
    bout = b;
    if (shift_latch == 1) {
      if (record == 0 && play == 0 ) {
        r = sample_out >> 4;
        b = sample_out >> 4;
      }
    }
    if (shift_latch == 0) {
      if (record == 0 && play == 0 ) {
        g = sample_out >> 4;
        b = sample_out >> 5;
      }
    }
  }

  preveigth = eigth;

  if (g > 1) {
    g--;
  }
  if (g <= 1) {
    g = 0;
  }

  if (r > 1) {
    r--;
  }
  if (r <= 1) {
    r = 0;
  }

  if (b > 1) {
    b--;
  }
  if (b <= 1) {
    b = 0;
  }
  if (noise_mode == 0) {
    if (record == 0 && play == 0 ) {

      rout = 16;
      gout = 16;
      bout = 16;
    }

  }


  if (play == 1 && record == 0) {
    bout = b * !erase;
    rout = r * !erase;
    gout = g * !erase;

    if ( loopstep == 0 ) {
      r = 12;
      g = 15;
      b = 12;
    }
    else if ( loopstep % 4 == 0) {
      r = 5;
      g = 5;
      b = 5;
    }
    else {
      b = bankpb;
      r = bankpr;
      g = bankpg;
    }

  }

  if (play == 1 && record == 1 ) {
    bout = b;
    rout = r;
    gout = g;

    if ( loopstep == 0 ) {
      r = 30;
      g = 6;
      b = 6;
    }
    else if ( loopstep % 4 == 0) {
      r = 20;
      g = 2;
      b = 2;
    }
    else {
      b = bankpb;
      r = bankpr;
      g = bankpg;
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void BUTTONS() {
  prevshift = shift;

  shift = digitalRead(SHIFT);

  if (shift == 0 && prevshift == 1) {
    shift_latch++;
    shift_latch %= 2;
    shift_time = 0;
    shift_time_latch = 1;
  }

  if (shift == 0 && tapb == 1 && shift_time_latch == 1) {
    shift_time++;
    if (shift_time > 800 ) {
      click_en = !click_en;
      shift_time = 0;
      shift_time_latch = 0;
    }
  }
  ///////////////////////////////////////////////////sequence select

  if (shift == 0 && recordbutton == 1) {
    prevpot2 = pot2;
    if (button1 == 0 ) { //red
      banko = 63;

    }
    if (button4 == 0) { //yellow
      banko = 31;
      bankpr = 4;
      bankpg = 4;
      bankpb = 0;
    }
    if (button2 == 0 || banko == 0) { //blue
      banko = 0;
      bankpr = 0;
      bankpg = 0;
      bankpb = 8;
    }
    if (button3 == 0) { //green
      banko = 95;
      bankpr = 0;
      bankpg = 3;
      bankpb = 0;

    }


    if (tapb == 0) {
      play = 1;
      ratepot = (analogRead(POT_LEFT));
      taptempo = ratepot << 2;
    }
    revbutton = digitalRead(PLAY);
    if (revbutton == 0 && prevrevbutton == 1) {
      playmode++;
      playmode %= 2;

    }
    prevrevbutton = revbutton;
  }

  if ( banko == 63) {

    bankpr = 4;
    bankpg = 0;
    bankpb = 0;
  }
  if ( banko == 31) {
    bankpr = 4;
    bankpg = 4;
    bankpb = 0;
  }
  if ( banko == 0) {
    bankpr = 0;
    bankpg = 0;
    bankpb = 8;
  }

  if ( banko == 95) {
    bankpr = 0;
    bankpg = 3;
    bankpb = 0;

  }


  if (shift == 1) {
    //       if (bf1==1){

    if (bf1 == 1 || midi_note_check == 60) {
      B1_trigger = 1;
    }
    else {
      B1_trigger = 0;
    }
    //    if (bf4==1){

    if (bf4 == 1 || midi_note_check == 65) {
      B4_trigger = 1;
    }
    else {
      B4_trigger = 0;
    }
    //    if (bf2==1){

    if (bf2 == 1 || midi_note_check == 62) {
      B2_trigger = 1;
    }
    else {
      B2_trigger = 0;
    }
    //   if (bf3==1){
    if (bf3 == 1 || midi_note_check == 64) {
      B3_trigger = 1;
    }
    else {
      B3_trigger = 0;
    }

  }



  ////////////////////////////////////////////

}

int midi_note_on() {
  #ifdef ENABLE_MIDI
    int type, note, velocity, channel, d1, d2;
    byte r = MIDI.read();
    if (r == 1) {                  // Is there a MIDI message incoming ?
      byte type = MIDI.getType();
      switch (type) {
        case 0x90: //note on. For some reasong "NoteOn" won't work enven though it's declared in midi_Defs.h
          note = MIDI.getData1();
          velocity = MIDI.getData2();
          if (velocity == 0) {
            note = 0;
          }

          break;
        case 0x80: //note off
          note = 0;
          break;

        case 0xB0: //control change
          if (MIDI.getData1() == 70) {
            midicc1 = (MIDI.getData2() << 2) + 3;
          }

          if (MIDI.getData1() == 71) {
            midicc2 = (MIDI.getData2() << 2) + 3;
          }

          if (MIDI.getData1() == 72) {
            midicc3 = (MIDI.getData2() << 2);
          }

          if (MIDI.getData1() == 73) {
            midicc4 = (MIDI.getData2() << 2);
          }

        default:
          note = 0;
      }
    }
    return note;
  #else 
  return 0;
  #endif
}
