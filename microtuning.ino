#include <AltSoftSerial.h>

#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

/**
F0 7F <device ID> 08 02 tt ll (kk xx yy zz) F7

F0 7F Universal Real Time SysEx header
<device ID> ID of target device !! DONT CARE
08 Sub-ID#1 (MIDI Tuning Standard)
02 Sub-ID#2 (02H, note change)
tt tuning program number (0-127) ! DONT CARE
ll number of changes (1 change = 1 set of (kk xx yy zz)) !! 1 fixed
kk = MIDI key number
xx yy zz = frequency data for that key (repeated „ll“ number of times)
F7 EOX
**/

#include <stdint.h>
#include <EEPROM.h>
#include <MsTimer2.h>
#include <SoftwareSerial.h>

// #define DEBUG

struct TuningData
{
	 uint8_t altNote;	 
	 uint8_t pitchMsb;
	 uint8_t pitchLsb;
	 
	 TuningData() : altNote(0xff), pitchMsb(0), pitchLsb(0) {}
};

MIDI_CREATE_DEFAULT_INSTANCE();

//SoftwareSerial debugMonitor(10, 11);
AltSoftSerial legacyMidiSerial(8, 9);

MIDI_CREATE_INSTANCE(AltSoftSerial, legacyMidiSerial, legacyMidi);


#include "bender.h"
#include "assigner.h"


enum class Sw1_Mode
{
	Play,
	Write,
	Unknown,
};

enum class Sw2_Mode
{
	 ThruOff,
	 ThruOn,
	 Unknown,
};

Sw1_Mode sw1Mode = Sw1_Mode::Unknown;
Sw2_Mode sw2Mode = Sw2_Mode::Unknown;


enum
{
	 EmptySlotAltNote = 0xff,
	 NumberOfNotes = 128,
	 FlashWritingTimeoutMilliSec = 500,
	 TuningMessageLength = 12,
	 NoteNumberIndex = 7,
	 AltNoteNumberIndex = 8,
	 
	 TuneMsbIndex = 9,
	 TuneLsbIndex = 10,

	 
	 PinkLed = 6, // power on
	 YellowLed = 7, // busy
	 
	 WritePlaySwitch = 12,
	 ThruSwitch = 13,
};



TuningData tuning[NumberOfNotes];

bool needWriteEepromOnLoop = false;

void timer()
{
	 needWriteEepromOnLoop = true;
}

void resetTimer()
{
	 MsTimer2::stop();
	 MsTimer2::start();
}

void stopTimer()
{
	 MsTimer2::stop();
}

BendNoteSender bendNoteSender(legacyMidi);
Assigner assigner(bendNoteSender);



void setup() {
#if DEBUG
	 debugMonitor.begin(115200);
	 debugMonitor.println("program start.");
#endif

	 legacyMidi.begin();
	 legacyMidi.turnThruOff();

	 
	 MIDI.begin(1);                // Launch MIDI with default options
	 MIDI.turnThruOff();

	 
	 // configure pins
	 pinMode(PinkLed, OUTPUT); // for power on
	 pinMode(YellowLed, OUTPUT); // for busy
	 pinMode(WritePlaySwitch, INPUT_PULLUP);
	 pinMode(ThruSwitch, INPUT_PULLUP); 

	 digitalWrite(PinkLed, HIGH);

   // read saved data
	 readEepRom();
	 bendNoteSender.setTuningData(tuning);
	 readSwitches();	 
	 MsTimer2::set(500, timer);

     delay(100);
}

uint8_t buffer[TuningMessageLength];
uint8_t bufferCounter = 0;
bool tuneMessageReceived;

void processUsbSysEx()
{
     digitalWrite(YellowLed, HIGH);

	 auto* const arr = MIDI.getSysExArray();
	 for (auto i = 0; i < MIDI.getSysExArrayLength(); ++i) {
		  buffer[bufferCounter++] = arr[i];
		  
		  if (arr[i] == 0xF7) {
			   tuneMessageReceived = true;
			   break;
		  }

		  if (bufferCounter == TuningMessageLength) {
			   tuneMessageReceived = true;
			   break;
		  }
	 }

	 if (tuneMessageReceived) {
		  tuneMessageReceived = false;
		  bufferCounter = 0;

		  const auto note = buffer[NoteNumberIndex];
		  const auto altNote = buffer[AltNoteNumberIndex];
		  const auto tuneMsb = (buffer[TuneMsbIndex]>> 1) + 0x40;
		  const auto tuneMsbLsb = buffer[TuneMsbIndex] & 1;
		  
		  const auto tuneLsb = ((buffer[TuneLsbIndex]) >> 1) + (tuneMsbLsb << 6); // append 1 bit from msb
		  
		  tuning[note].altNote = altNote;
		  tuning[note].pitchMsb = tuneMsb;
		  tuning[note].pitchLsb = tuneLsb;
#if DEBUG
		  debugMonitor.print("Note ");
		  debugMonitor.print(note);
		  debugMonitor.print("->");
		  debugMonitor.print(altNote);
		  debugMonitor.print("Pitch");
		  debugMonitor.print((tuneMsb << 7) + tuneLsb);
		  debugMonitor.println("");
#endif
		  resetTimer();
	 }

     digitalWrite(YellowLed, LOW);
}

void noteOn(const uint8_t note, const uint8_t velocity)
{
	 assigner.noteOn(note, velocity);
}
void noteOff(const uint8_t note, const uint8_t /* velocity */)
{
	 assigner.noteOff(note);
}

void usbThru(const midi::MidiType type, const uint8_t data1, const uint8_t data2, const uint8_t channel)
{
	 legacyMidi.send(type, data1, data2, channel);
	 
}


void processUsbMidi()
{	 
	 if (MIDI.read()) {
		  const auto type = MIDI.getType();
          const auto data1 = MIDI.getData1();
          const auto data2 = MIDI.getData2();
          const auto channel = MIDI.getChannel();
		  
		  switch(type) {
		  case midi::SystemExclusive:
			   processUsbSysEx();
			   break;
          case midi::NoteOn:
			   if (sw2Mode == Sw2_Mode::ThruOn) {
					usbThru(type, data1, data2, channel);
			   } else {
					noteOn(data1, data2);
			   }
               break;
          case midi::NoteOff:
               if (sw2Mode == Sw2_Mode::ThruOn) {
					usbThru(type, data1, data2, channel);
			   } else {
					noteOff (data1, data2);
			   }
               break;
		  default:
               legacyMidi.send(type, data1, data2, 1);
			   break;
		  }
	 }	 
}


uint8_t receiveCounter = 0;
uint8_t midiBuf[3] = {};



void thru()
{
	 legacyMidi.send(legacyMidi.getType(),
					 legacyMidi.getData1(),
					 legacyMidi.getData2(),
					 legacyMidi.getChannel());
}

void processLegacyMidi()
{
	 if (!legacyMidi.read()) {
		  return;
	 }
  
	 // thru
	 if (sw2Mode == Sw2_Mode::ThruOn) {
		  thru();
		  return;
	 }

	 const auto type = legacyMidi.getType();
	 const auto data1 = legacyMidi.getData1();
	 const auto data2 = legacyMidi.getData2();
	 
	 switch (type) {
	 case midi::NoteOn:
		  noteOn(data1, data2);
		  break;
	 case midi::NoteOff:
		  noteOff(data1, data2);
		  break;
	 default:
		  thru();
		  break;		  
	 }
}

void readEepRom()
{
#if DEBUG
	 debugMonitor.println("Reading EEPROM...");
#endif

	 digitalWrite(YellowLed, HIGH);
	 for (auto i = 0; i < NumberOfNotes; ++i) {
		  const auto baseAddress = i * 3;
		  tuning[i].altNote = EEPROM.read(baseAddress);
		  tuning[i].pitchMsb = EEPROM.read(baseAddress + 1);
		  tuning[i].pitchLsb = EEPROM.read(baseAddress + 2);
	 }
	 digitalWrite(YellowLed, LOW);
	 
#if DEBUG
	 for (auto i = 0; i < NumberOfNotes; ++i) {
		  debugMonitor.print(tuning[i].altNote);
		  debugMonitor.print(", ");
		  
		  debugMonitor.print(tuning[i].pitchLsb);
		  debugMonitor.print(", ");

		  debugMonitor.print(tuning[i].pitchMsb);
		  debugMonitor.println(", ");
	 }
#endif
}

void writeEepRom()
{
#if DEBUG
	 debugMonitor.println("writing EEPROM..");
	 debugMonitor.print("EEPROM Size =");
	 debugMonitor.println(EEPROM.length());
#endif
	 
	 for (auto i = 0; i < NumberOfNotes; ++i) {
		  digitalWrite(YellowLed, HIGH);

		  const auto baseAddress = i * 3;		  
		  EEPROM.write(baseAddress    , EmptySlotAltNote); 
		  EEPROM.write(baseAddress    , tuning[i].altNote);

		  EEPROM.write(baseAddress + 1, 0);
		  EEPROM.write(baseAddress + 1, tuning[i].pitchMsb);

		  EEPROM.write(baseAddress + 2, 0);
		  EEPROM.write(baseAddress + 2, tuning[i].pitchLsb);
					   
		  digitalWrite(YellowLed, LOW);
	 }
#if DEBUG
	 debugMonitor.println("done");
#endif
}


void modeTransition()
{
	 // rough implementation
	 bufferCounter = 0;
	 receiveCounter = 0;
	 
	 assigner.allNoteOff();
	 bendNoteSender.clear();
	 
}

void sw1Changed(const Sw1_Mode newMode)
{
#if DEBUG
	 debugMonitor.print("Sw1 Mode = ");
	 debugMonitor.println((int)newMode);
#endif

//modeTransition();
	 

	 sw1Mode = newMode;
}

void sw2Changed(const Sw2_Mode newMode)
{
#if DEBUG
	 debugMonitor.print("Sw2 Mode = ");
	 debugMonitor.println((int)newMode);
#endif

	 modeTransition();

	 sw2Mode = newMode;
}
void readSwitches()
{	 
	 const auto sw1 = digitalRead(WritePlaySwitch);
	 const auto newSw1Mode = sw1 ? Sw1_Mode::Write : Sw1_Mode::Play;
	 
	 if (sw1Mode != newSw1Mode) {
		  sw1Changed(newSw1Mode);
	 }
	 
	 const auto sw2 = digitalRead(ThruSwitch);
	 const auto newSw2Mode = sw2 ? Sw2_Mode::ThruOn : Sw2_Mode::ThruOff;

	 if (sw2Mode != newSw2Mode) {
		  sw2Changed(newSw2Mode);
	 }
}

void loop()
{
	 processLegacyMidi();
	 processUsbMidi();

	 readSwitches();

	 if (needWriteEepromOnLoop) {
		  needWriteEepromOnLoop = false;
		  stopTimer();
		  writeEepRom();
	 }
}
