#include "database.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

namespace FIDB {
	std::fstream backing;

	Database::Database(char* file) {
		backing.open(file, std::ios::in|std::ios::out|std::ios::binary);
	}
	Database::~Database() {
		backing.close();
	}
	
	void Database::_GetLock(unsigned long blockpos, bool readonly) {
		char blocklock = 1;
		backing.seekg(blockpos);
		backing.read(&blocklock, 1);
		while (blocklock == 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); //Thanks StackOverflow!
		}
		if (!readonly) {
			blocklock = 1;
			backing.seekg(blockpos);
			backing.write(&blocklock, 1);
		}
	}

	void Database::_ReleaseLock(unsigned long blockpos, bool readonly) {
		if (!readonly) {
			char blocklock = 0;
			backing.seekg(blockpos);
			backing.write(&blocklock, 1);
		}
	}

	Item Database::_ReadBlock(unsigned long blockpos, unsigned long jmpfrom) {
		_GetLock(blockpos, true);
		unsigned long blkhdrs[3];
		backing.read((char*)&blkhdrs, sizeof(unsigned long)*3);
		
		if (blkhdrs[1] != blkhdrs[2] && blkhdrs[0] == 0 && jmpfrom == NULL)
			std::cerr << "Corrupt header in block @ " << blockpos << "(sizes differ, but no JMP)" << std::endl;
		
		char content[blkhdrs[2]];
		if (blkhdrs[0] != 0) {
			Item tomerge = _ReadBlock(blkhdrs[0], blockpos);
			//for each item in tomerge.content, add to content array, after all that construct item and return
		}
		_ReleaseLock(blockpos, true);
	}
	
	_IndexArr* Database::_ReadIndex(char* indexpos) {
	}
	
	Item* Database::operator[] (const unsigned long id) {
	}

	unsigned long Database::AddItem(Item* item) {
	}
}
