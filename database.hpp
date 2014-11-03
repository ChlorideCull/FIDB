#include <iostream>
#include <fstream>

namespace FIDB {
	struct Item {
		unsigned long itemsize;
		char* item;
	};

	struct Index {
		unsigned long id;
		unsigned long blockoffset;
	};

	struct _IndexArr {
		unsigned long count;
		Index* indexitems;
	};

	class Database {
		std::fstream backing;
		private:
			Item _ReadBlock(char* blockpos);
			_IndexArr* _ReadIndex(char* indexpos);
		public:
			Database(char*);
			~Database();
			Item* operator[] (const unsigned long id);
			unsigned long AddItem(Item* item);
	};
}
