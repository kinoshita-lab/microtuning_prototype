#pragma once


#include <stdint.h>
#include "List.h"

class Assigner
{
	 enum {
		  NUM_OF_LIST_ITEMS = 4,
	 };
	 
public:
	 struct Item
	 {	  
		  enum {
			   INVALID_VALUE = 0xff,
		  };
		  uint8_t noteNumber;
		  uint8_t velocity;
		  
		  Item() : noteNumber(INVALID_VALUE), velocity(INVALID_VALUE) {}
		  Item(const uint8_t noteNumber, const uint8_t velocity) : noteNumber(noteNumber), velocity(velocity) {
		  }

		  bool operator==(const Item& rhs) {
			   return noteNumber == rhs.noteNumber;
		  }
	 };
	 
	 
public:

	 Assigner(BendNoteSender& bendNoteSender) : lastNoteOnNumber(0xff), bendNoteSender(bendNoteSender) {
	 }
	 
	 ~Assigner() {
	 }


	 void noteOn(const uint8_t noteNumber, const uint8_t velocity) {
#if DEBUG
		  debugMonitor.print("note on ");
		  debugMonitor.println(noteNumber);
#endif
		  
		  
		  if (items.size()) {			   
			   // note off for last item
			   auto it = items.find(Item(lastNoteOnNumber, 0));
			   bendNoteSender.noteOff(it.noteNumber, it.velocity);
		  }
		  
		  auto newItem = Item(noteNumber, velocity);
		  items.push_back(newItem);
		  
		  bendNoteSender.noteOn(newItem.noteNumber, newItem.velocity);
		  lastNoteOnNumber = newItem.noteNumber;

		  printItems();
	 }

	 void noteOff(const uint8_t noteNumber) {
#if DEBUG
		  debugMonitor.print("note off ");
		  debugMonitor.println(noteNumber);
#endif

		  auto it = items.find(Item(noteNumber, 0));
		  
		  if (it == items.end()) {
			   return;
		  }
		  
		  if (it.noteNumber == lastNoteOnNumber) {
			  bendNoteSender.noteOff(it.noteNumber, it.velocity);
		  }
		  
		  items.erase(it);

		  
		  if (items.size()) {
			   const auto& item = items.back();
			   if (lastNoteOnNumber != item.noteNumber) {
					bendNoteSender.noteOn(item.noteNumber, item.velocity);
					lastNoteOnNumber = item. noteNumber;
			   }
		  } else {
			   lastNoteOnNumber = 0xff;
		  }
		  printItems();
	 }

	 void allNoteOff() {
		  items.clear();
		  if (lastNoteOnNumber != 0xff) {
			  bendNoteSender.noteOff(lastNoteOnNumber, 64);
		  }
		  lastNoteOnNumber = 0xff;
		  printItems();
	 }

	 void printItems() {
#if DEBUG
		  // for debug
		  debugMonitor.print("Assigner status: ");
		  
		  for (auto i = 0; i < NUM_OF_LIST_ITEMS; ++i) {
			   debugMonitor.print(items[i].noteNumber);
			   debugMonitor.print(", ");
			   
		  }
		  debugMonitor.println("");
#endif
	 }

private:

 	 uint8_t lastNoteOnNumber;
	 BendNoteSender& bendNoteSender;
	 ListLike< Item, NUM_OF_LIST_ITEMS > items;
};

