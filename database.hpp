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
		private:
			Item _ReadBlock(unsigned long, unsigned long);
			_IndexArr* _ReadIndex(char*);
			void _GetLock(unsigned long, bool);
			void _ReleaseLock(unsigned long, bool);
		public:
			Database(char*);
			~Database();
			Item* operator[] (const unsigned long id);
			unsigned long AddItem(Item* item);
	};
}
