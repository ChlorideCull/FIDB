#include <cstdint>
#include <vector>

namespace FIDB {
	struct Item {
		char* name;
		uint64_t itemsize;
		char* item;
	};

	struct Index {
		uint64_t id;
		char* name;
		uint64_t blockoffset;
	};

	struct IndexBlock {
		std::vector<Index> Indexes;
		uint64_t HighestID;
	};

	class Database {
		private:
			Item _ReadBlock(uint64_t , uint64_t);
			void _WriteBlock(Item, uint64_t);
			IndexBlock _ReadIndex(const uint64_t);
			void _GetLock(uint64_t, bool);
			void _ReleaseLock(uint64_t, bool);
		public:
			Database(char*);
			~Database();
			Item* operator[] (const uint64_t id);
			Item* operator[] (const char*);
			uint64_t AddItem(Item* item);
	};
}
