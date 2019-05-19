#include "sqlite3.h"
#include <string>
using namespace std;

class sqliteDB
{
private:
	sqlite3 *db;
	string key;
public:
	sqliteDB();
	~sqliteDB();
	void createDB();
	void openDB();
	void initvars();
	void closeDB();
	void createBackup();
	bool userSessMatch(string username, string sessID,  int checkingForWhat);
	int login(string username, string password);
	int signup(string username, string name, string passwordOne, string passwordTwo);
	int createAdmin(string username, string name, string passwordOne, string passwordTwo, string secret);
	int joinRoom(string roomName, string username, string sessID);
	int createRoom(string username, string sessID);
	int joinAsAdmin(string username, string sessID);
	int createSessID(string username);
	int removeSessID(string username);
};

