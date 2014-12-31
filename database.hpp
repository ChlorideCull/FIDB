#include <cstdint>
#include <vector>

namespace FIDB {
	struct Item {
		uint64_t itemsize;
		char* item;
	};

	struct Index {
		uint64_t id;
		uint32_t namelen;
		char* name;
		uint64_t blockoffset;
	};

	class Database {
		private:
			Item _ReadBlock(uint64_t , uint64_t);
			void _WriteBlock(Item, uint64_t);
			std::vector<Index> _ReadIndex(const uint64_t);
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
