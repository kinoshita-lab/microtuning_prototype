#pragma once

#include <inttypes.h>

template <typename Type, size_t N>
class ListLike
{
public:
	 ListLike() {
		  clear();
	 }

	 void setEndItem(const Type& _end) {
		  endItem = _end;
	 }
	 
	 size_t size() {
		  size_t size = 0;
		  
		  for (auto&& o : order) {
			   if (o != Empty) {
					size++;
			   }
		  }
		  return size;
		  
	 }

	 Type& back() {
		  int newest = -1;
		  int newestIndex = -1;
		  
		  for (auto i = 0; i < N; ++i) {
			   if (order[i] > newest) {
					newest = order[i];
					newestIndex = i;
			   }
		  }

		  if (newestIndex != -1) {
			   return listItems[newestIndex];
		  }

		  return end();
	 }

	 Type& find(const Type& item) {
		  for (auto i = 0; i < N; ++i) {
			   if (listItems[i] == item) {
					return listItems[i];
			   }
		  }
		  
		  return end();
	 }
	 
	 Type& end() {
		  return endItem;
	 }

	 void clear() {
		  for (auto i = 0; i < N; ++i) {
			   order[i] = Empty;
			   listItems[i] = endItem;
		  }
	 }

	 void push_back(const Type& item) {
		  int newest = -1;
		  
		  // find new order number
		  for (auto i = 0; i < N; ++i) {
			   if (order[i] > newest) {
					newest = order[i];
			   }
		  }

		  const int pushItemOrder = newest + 1;

		  // search oldest or empty slot
		  int oldest = INT16_MAX;
		  int oldestIndex = -1;
		  
		  for (auto i = 0; i < N; ++i) {
			   if (order[i] < oldest) {
					oldest = order[i];
					oldestIndex = i;
					if (oldestIndex == Empty) {
						 break;
					}
			   }
		  }

		  if (oldest != Empty) {
			   erase(listItems[oldestIndex]);
		  }
		  
		  order[oldestIndex] = pushItemOrder;
		  listItems[oldestIndex] = item;
	 }

	 void erase(const Type& item) {
		  auto index = -1;
		  
		  for (auto i = 0; i < N; ++i) {
			   if (listItems[i] == item) {
					index = i;
					break;
			   }
		  }

		  if (index != -1) {
			   order[index] = Empty;
		  }
	 }
	 
	 Type& operator[](const int i) {
		  if (order[i] == Empty) {
			   return endItem;
		  }
		  return listItems[i];
	 }
private:
	 Type endItem;
	 Type listItems[N];

	 enum {
		  Empty = -1,
	 };	 
	 int order[N];
};
