#include <cstdint>

namespace FIDB {
	struct Item {
		uint64_t itemsize;
		char* item;
	};

	struct Index {
		uint64_t id;
		uint64_t blockoffset;
	};

	struct _IndexArr {
		uint64_t count;
		Index* indexitems;
	};

	class Database {
		private:
			Item _ReadBlock(uint64_t , uint64_t);
			void _WriteBlock(Item, uint64_t);
			_IndexArr* _ReadIndex(char*);
			void _GetLock(uint64_t, bool);
			void _ReleaseLock(uint64_t, bool);
		public:
			Database(char*);
			~Database();
			Item* operator[] (const uint64_t id);
			uint64_t AddItem(Item* item);
	};
}
