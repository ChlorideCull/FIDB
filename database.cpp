#include "database.hpp"

namespace FIDB {
	std::fstream backing;

	Database::Database(char* file) {
		backing.open(file, std::ios::in|std::ios::out|std::ios::binary);
	}
	Database::~Database() {
		backing.close();
	}
	
	Item Database::_ReadBlock(char* blockpos) {
	}
	
	_IndexArr* Database::_ReadIndex(char* indexpos) {
	}
	
	Item* Database::operator[] (const unsigned long id) {
	}

	unsigned long Database::AddItem(Item* item) {
	}
}
