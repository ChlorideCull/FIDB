#include "database.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

//Allocate blocks at multiples of this size to limit fragmentation.
//(otherwise most changes to blocks would generate single byte or very small blocks)
#define OVERALLOC_SIZE 512

//Version, do not change without written consent from lead developer.
#define FIDBVER 3

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
			backing.seekg(blockpos);
			backing.read(&blocklock, 1);
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
		unsigned long blkhdrs[4];
		backing.read((char*)blkhdrs, sizeof(unsigned long)*4);
		
		if (blkhdrs[1] != blkhdrs[2] && blkhdrs[0] == 0 && jmpfrom == NULL)
			std::cerr << "Corrupt header in block @ " << blockpos << " (sizes differ, but no JMP)" << std::endl;
		
		char content[blkhdrs[2]];
		unsigned long contentlen = blkhdrs[1];
		backing.read(content, contentlen);

		if (blkhdrs[0] != 0) {
			Item tomerge = _ReadBlock(blkhdrs[0], blockpos);
			for (unsigned long i; i < tomerge.itemsize; i++) {
				content[contentlen] = tomerge.item[i];
				contentlen++;
			}
		}

		_ReleaseLock(blockpos, true);
		Item toret;
		toret.itemsize = contentlen;
		toret.item = content;
		return toret;
	}

	unsigned long _WriteBlock(Item towrite, unsigned long blockpos) {
	}

	_IndexArr* Database::_ReadIndex(char* indexpos) {
	}
	
	Item* Database::operator[] (const unsigned long id) {
	}

	unsigned long Database::AddItem(Item* item) {
	}
}
