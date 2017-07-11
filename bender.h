#pragma once
#include <stdint.h>
#include "LegacyMidiType.h"

class BendNoteSender
{
public:
	 BendNoteSender(MicroTuningLegacyMidi& midi) : midi(midi) {
	 }
	 
	 void setTuningData(TuningData* _tuning) {
		  tuning = _tuning;
	 }
	 
	 void noteOn(const uint8_t noteNumber, const uint8_t velocity) {
		  uint8_t actualNote = noteNumber;
		  
		  // send pitch bend if "tuned"
		  if (tuning[noteNumber].altNote != 0xff) {
			   midi.send(midi::PitchBend,  tuning[noteNumber].pitchLsb, tuning[noteNumber].pitchMsb, 1);
			   actualNote = tuning[noteNumber].altNote;
		  }
		  
		  midi.sendNoteOn(actualNote, velocity, 1);
	 }


	 void noteOff(const uint8_t noteNumber, const uint8_t /* velocity */) {
		  uint8_t actualNote = noteNumber;
		  
		  if (tuning[noteNumber].altNote != 0xff) {
			   actualNote = tuning[noteNumber].altNote;
		  }
		  midi.sendNoteOff(actualNote,(byte)64, 1);
	 }

	 void clear() {
		  midi.send(midi::PitchBend, 0x40, 0x00, 1);
	 }
	 
private:
	 TuningData* tuning;
	 MicroTuningLegacyMidi& midi;
	 
};
