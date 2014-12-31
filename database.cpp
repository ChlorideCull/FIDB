#include "database.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

//Allocate blocks at multiples of this size to limit fragmentation.
//(otherwise most changes to blocks would generate single byte or very small blocks)
#define OVERALLOC_SIZE 512

//Version, do not change without written consent from lead developer.
#define FIDBVER '3'

namespace FIDB {
	std::fstream backing;

	Database::Database(char* file) {
		backing.open(file, std::ios::in|std::ios::out|std::ios::binary);
		if (backing.eof()) {
			char ver = FIDBVER;
			backing.write(&ver, 1);
		} else {
			char usedver = '\0';
			backing.read(&usedver, 1);
			if (usedver != FIDBVER) {
				std::cerr << "Version " << usedver << " in file differs from " << FIDBVER << " in library!" << std::endl;
				return;
			}
		}
	}
	Database::~Database() {
		backing.close();
	}
	
	void Database::_GetLock(uint64_t blockpos, bool readonly) {
		char blocklock = 1;
		backing.seekg(blockpos);
		backing.read(&blocklock, 1);
		while (blocklock == 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); //Thanks StackOverflow!
			backing.seekg(blockpos);
			backing.read(&blocklock, 1);
		}
		if (readonly) {
			blocklock = 1;
			backing.seekp(blockpos);
			backing.write(&blocklock, 1);
		}
	}

	void Database::_ReleaseLock(uint64_t blockpos, bool readonly) {
		if (readonly) {
			char blocklock = 0;
			backing.seekp(blockpos);
			backing.write(&blocklock, 1);
		}
	}

	Item Database::_ReadBlock(uint64_t blockpos, uint64_t jmpfrom) {
		_GetLock(blockpos, true);
		uint64_t blkhdrs[4];
		backing.read((char*)blkhdrs, sizeof(uint64_t)*4);
		
		if (blkhdrs[1] != blkhdrs[2] && blkhdrs[0] == 0 && jmpfrom == 0)
			std::cerr << "Corrupt header in block @ " << blockpos << " (sizes differ, but no JMP)" << std::endl;
		
		char content[blkhdrs[2]];
		uint64_t contentlen = blkhdrs[1];
		backing.read(content, contentlen);

		if (blkhdrs[0] != 0) {
			Item tomerge = _ReadBlock(blkhdrs[0], blockpos);
			for (uint64_t i; i < tomerge.itemsize; i++) {
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

	void Database::_WriteBlock(Item towrite, uint64_t blockpos) {
		backing.seekg(blockpos);
		backing.seekp(blockpos);
		bool newblk;
		uint64_t blkhdrs[4];
		if (backing.eof()) {
			blkhdrs[0] = 0;
			blkhdrs[1] = towrite.itemsize;
			blkhdrs[2] = towrite.itemsize;
			blkhdrs[3] = towrite.itemsize + (sizeof(uint64_t)*4) + 1;
			backing.write(0, 1);
			backing.write((char*)&blkhdrs, sizeof(uint64_t)*4);
			backing.write(towrite.item, towrite.itemsize);
			backing.flush();
		} else {
			_GetLock(blockpos, true);
			backing.read((char*)blkhdrs, sizeof(uint64_t)*4);
			if (towrite.itemsize + 1 + (sizeof(uint64_t)*4) <= blkhdrs[3]) {
				//Block large enough or non-existent block
				backing.write(0, 1);
				blkhdrs[0] = 0;
				blkhdrs[1] = towrite.itemsize;
				blkhdrs[2] = towrite.itemsize;
				//Keep blkhdrs[3]
				backing.write((char*)&blkhdrs, sizeof(uint64_t)*4);
				backing.write(towrite.item, towrite.itemsize);
				backing.flush();
				_ReleaseLock(blockpos, true);
			} else {
				//Have to continue to a new block, allocate or reuse
				blkhdrs[0] = 0; //Find a new block position? How?
				//Keep blkhdrs[1]
				blkhdrs[2] = towrite.itemsize;
				//Keep blkhdrs[3]
				backing.write(0, 1);
				backing.write((char*)&blkhdrs, sizeof(uint64_t)*4);
				backing.write(towrite.item, towrite.itemsize);
				backing.flush();
				_ReleaseLock(blockpos, true);
			}
		}
	}

	std::vector<Index> Database::_ReadIndex(const uint64_t indexpos) {
	}
	
	Item* Database::operator[] (const uint64_t id) {
	}

	Item* Database::operator[] (const char* id) {
	}

	uint64_t Database::AddItem(Item* item) {
	}
}
