#include "database.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <cstring>

//Allocate blocks at multiples of this size to limit fragmentation.
//(otherwise most changes to blocks would generate single byte or very small blocks)
#define OVERALLOC_SIZE 512

//Version, do not change without written consent from lead developer.
#define FIDBVER '3'

namespace FIDB {
	std::fstream backing;
	IndexBlock indexstore;

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
		indexstore = _ReadIndex(1); //Index is always on second byte
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
			for (uint64_t i = 0; i < tomerge.itemsize; i++) {
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
				std::streampos currentpos = backing.tellp();
				backing.seekp(0, std::ios::end);
				std::streampos endpos = backing.tellp();
				backing.seekp(currentpos, std::ios::beg);
				blkhdrs[0] = (uint64_t) endpos;

				//Keep blkhdrs[1]
				blkhdrs[2] = towrite.itemsize;
				//Keep blkhdrs[3]

				uint64_t blockcancontain = blkhdrs[3] - ((sizeof(uint64_t)*4) + 1);
				backing.write(0, 1);
				backing.write((char*)&blkhdrs, sizeof(uint64_t)*4);
				backing.write(towrite.item, blockcancontain);
				backing.flush();
				_ReleaseLock(blockpos, true);

				//Continue to next block!
				Item process;
				process.itemsize = towrite.itemsize-blockcancontain;
				process.item = towrite.item+blockcancontain;
				_WriteBlock(process, blockpos);
			}
		}
	}

	IndexBlock Database::_ReadIndex(const uint64_t indexpos) {
		Item indexblockraw = _ReadBlock(indexpos, 0);
		IndexBlock output;
		output.HighestID = *((uint64_t*)indexblockraw.item);
		/*
			ulong - highest used row ID
 			ulong - row ID
 			uint - name length
			cstr - name
			ulong - byte location of block
		 */
		uint64_t codepos = sizeof(uint64_t); //Because I'm lazy, and whenever a compiler fucks up the standard

		while (indexblockraw.itemsize < codepos) {
			Index toinput;
			toinput.id = *((uint64_t*)indexblockraw.item+codepos);
			codepos += sizeof(toinput.id);
			uint32_t namelen = *((uint32_t*)indexblockraw.item+codepos);
			codepos += sizeof(namelen);
			toinput.name = (char*)malloc(namelen);
			strcpy(toinput.name, indexblockraw.item+codepos);
			codepos += namelen;
			toinput.blockoffset = *((uint64_t*)indexblockraw.item+codepos);
			codepos += sizeof(toinput.blockoffset);
			output.Indexes.insert(output.Indexes.end(), toinput);
		}

		return output;
	}
	
	Item* Database::operator[] (const uint64_t id) {
		Item target = _ReadBlock(indexstore.Indexes[id].blockoffset, 0);
		Item *output = (Item *) malloc(sizeof(Item));
		*output = target;
		return output;
	}

	Item* Database::operator[] (const char* id) {
		for (uint64_t i = 0; i < indexstore.Indexes.size(); ++i) {
			if (strcmp(indexstore.Indexes[i].name, id)) {
				Item target = _ReadBlock(indexstore.Indexes[i].blockoffset, 0);
				Item *output = (Item *) malloc(sizeof(Item));
				*output = target;
				return output;
			}
			throw Exceptions::ITEM_NOT_FOUND;
		}
	}

	uint64_t Database::AddItem(const Item item) {
		Index toadd;
		Item topmostblock = _ReadBlock(indexstore.HighestID, 0);
		uint64_t blockpos = indexstore.Indexes[indexstore.HighestID].blockoffset+topmostblock.itemsize;
		_WriteBlock(item, blockpos);

		toadd.blockoffset = blockpos;
		toadd.id = indexstore.HighestID+1;
		toadd.name = (char*)malloc(strlen(item.name));
		strcpy(toadd.name, item.name);

		indexstore.Indexes.insert(indexstore.Indexes.end(), toadd);
		indexstore.HighestID++;
		return toadd.id;
	}
}
