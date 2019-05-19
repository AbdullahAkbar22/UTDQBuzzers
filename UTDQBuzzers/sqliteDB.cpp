#include "sqliteDB.h"
#include "sqlite3.h"
#include "encrypt.h"
#include <iostream>
#include <string>
#include <time.h>

using namespace std;


sqliteDB::sqliteDB()
{
}


sqliteDB::~sqliteDB()
{
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
int i;
for (i = 0; i < argc; i++) {
	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}

void sqliteDB::createDB() {
	int creator;
	creator = sqlite3_open("databases/UTDQB.db", &db);

	if (creator) {

		cout << "Can't open database: " << sqlite3_errmsg(db) << "\n";
	}
	else
	{
		cout << "Opened database successfully\n\n";
	}
	const char *sqlcmd;
	sqlcmd = "CREATE TABLE USERDATA("  \
		"username TEXT," \
		"name TEXT," \
		"password TEXT," \
		"sessID INT," \
		"isAdmin INT DEFAULT 0," \
		"roomPriv INT DEFAULT 1," \
		"roomIn INT DEFAULT -1);";
	char *zErrMsg = 0;
	creator = sqlite3_exec(db, sqlcmd, callback, 0, &zErrMsg);
	if (creator != SQLITE_OK) {
		cout << "SQL error:" << zErrMsg << endl;
		sqlite3_free(zErrMsg);
	}
	else {
		cout << "Table created successfully" << endl;
	}
	
}

void sqliteDB::openDB() {
	int opener;
	opener = sqlite3_open("databases/UTDQB.db", &db);

	if (opener) {

		cout << "Can't open database: " << sqlite3_errmsg(db) << "\n";
	}
	else
	{
		cout << "Opened database successfully\n\n";
	}
	
}

void sqliteDB::initvars() {
	key = "2HdesoVXPbMFswQe4PIpunMadJsrbISgWFQMzccPMZQTqPCsGdP9w5JwV85B46xIpX0m4";

}

void sqliteDB::closeDB() {

	sqlite3_close(db);
}

void sqliteDB::createBackup() {


}

bool sqliteDB::userSessMatch(string username, string sessID, int checkingForWhat) {
	//checkingForWhat: 0 if simple login, 1 if roomPriv = 2
	return false;
}

int sqliteDB::login(string username, string password) {
	string testIfUserCMD;
	testIfUserCMD = "SELECT * from USERDATA where username = '" + username + "'";
	const char * testIfUserCMDc = testIfUserCMD.c_str();
	struct sqlite3_stmt *selectstmt;
	int result = sqlite3_prepare_v2(db, testIfUserCMDc, -1, &selectstmt, NULL);
	if (result == SQLITE_OK)
	{
		if (sqlite3_step(selectstmt) != SQLITE_ROW)
		{
			//user doesn't exist
			return 602;

		}
	}
	sqlite3_finalize(selectstmt);
	string testIfPassCMD;
	testIfPassCMD = "SELECT * from USERDATA where username = '" + username + "' AND password = '"+encrypt(password,key)+"'";
	const char * testIfPassCMDc = testIfPassCMD.c_str();
	struct sqlite3_stmt *selectstmtTwo;
	result = sqlite3_prepare_v2(db, testIfPassCMDc, -1, &selectstmtTwo, NULL);
	if (result == SQLITE_OK)
	{
		if (sqlite3_step(selectstmtTwo) != SQLITE_ROW)
		{
			//Password is incorrect
			return 603;

		}
	}
	sqlite3_finalize(selectstmtTwo);


	//Otherwise, successful
	return 100;
}

int sqliteDB::signup(string username, string name, string passwordOne, string passwordTwo) {
	//IF password doesn't meet requirements

	string testIfUserCMD;
	testIfUserCMD = "SELECT * from USERDATA where username = '" + username + "'";
	const char * testIfUserCMDc = testIfUserCMD.c_str();
	struct sqlite3_stmt *selectstmt;
	int result = sqlite3_prepare_v2(db, testIfUserCMDc, -1, &selectstmt, NULL);
	if (result == SQLITE_OK)
	{
		if (sqlite3_step(selectstmt) == SQLITE_ROW)
		{
			return 502;

		}
	}
	sqlite3_finalize(selectstmt);

	if (passwordOne != passwordTwo) {
		//IF passwords don't match
		return 603;
	}

	//Otherwise, successful
	string sqlcmd;


	string encryptedPass = encrypt(passwordOne, key);

	sqlcmd = "INSERT INTO USERDATA(username, name, password, isAdmin, roomPriv)" \
		"VALUES ('" + username + "','" + name + "','" + encryptedPass + "',0,1)";
	const char *sqlcmdc = sqlcmd.c_str();
	char *zErrMsg = 0;
	int cmdINT;
	cmdINT = sqlite3_exec(db, sqlcmdc, callback, 0, &zErrMsg);
	
	return 100;
}

int sqliteDB::createAdmin(string username, string name, string passwordOne, string passwordTwo, string secret) {
	
	if (secret != "v3rys3cur3p@ss"){
		return 505;
	}
	if (passwordOne != passwordTwo) {
		//IF passwords don't match
		return 603;
	}

	string testIfUserCMD;
	testIfUserCMD = "SELECT * from USERDATA where username = '" + username + "'";
	const char * testIfUserCMDc = testIfUserCMD.c_str();
	struct sqlite3_stmt *selectstmt;
	int result = sqlite3_prepare_v2(db, testIfUserCMDc, -1, &selectstmt, NULL);
	if (result == SQLITE_OK)
	{
		if (sqlite3_step(selectstmt) == SQLITE_ROW)
		{
			return 502;

		}
	}
	sqlite3_finalize(selectstmt);
	

	
	string sqlcmd;
	
	
	string encryptedPass = encrypt(passwordOne, key);

	sqlcmd = "INSERT INTO USERDATA(username, name, password, isAdmin, roomPriv)" \
		"VALUES ('"+ username +"','"+name+"','"+encryptedPass+"',1,2)";
	const char *sqlcmdc = sqlcmd.c_str();
	char *zErrMsg = 0;
	int cmdINT;
	cmdINT = sqlite3_exec(db, sqlcmdc, callback, 0, &zErrMsg);
	
	//Otherwise, successful
	return 100;
}


int sqliteDB::joinRoom(string roomName, string username, string sessID) {
	if (!userSessMatch(username, sessID, 0)) {
		return 703;
	}
	//IF room does not exist
	return 702;

	//Otherwise, successful (must add user to room and set score to 0 if not so)
	return 100;
}

int sqliteDB::createRoom(string username, string sessID) {
	if (!userSessMatch(username, sessID, 1)) {
		return 802;
	}

	//Otherwise, successful (must create room and set admin to username)
	return 100;
}

int sqliteDB::joinAsAdmin(string username, string sessID) {
	if (!userSessMatch(username, sessID, 1)) {
		return 802;
	}
	//IF room has other admin
	return 803;

	//Otherwise, successful (must create room and set admin to username)
	return 100;
}

int sqliteDB::createSessID(string username) {
	srand(time(NULL));
	
	int sessID = rand() % 10000 + 1;
	string sessIDString = to_string(sessID);
	cout << "Sess id is: " << sessID << endl;
	string sessIDcmd = "UPDATE USERDATA SET sessID = "+sessIDString+" WHERE username = '" + username+"'";
	const char* sessIDcmdc = sessIDcmd.c_str();
	char *zErrMsg = 0;
	int cmdINT;
	cmdINT = sqlite3_exec(db, sessIDcmdc, callback, 0, &zErrMsg);
	if (cmdINT == SQLITE_OK) {
		//Command successfully executed
		
		return sessID;
	}

	
	return -1;
}

int sqliteDB::removeSessID(string username) {
	//int sessID = -1;
	string updatecmd;
	const char * updatecmdc;
	updatecmd = "UPDATE USERDATA SET sessID = -1, roomIn = -1 WHERE username = " + username;
	updatecmdc = updatecmd.c_str();
	char *zErrMsg = 0;
	int cmdINT;
	cmdINT = sqlite3_exec(db, updatecmdc, callback, 0, &zErrMsg);
	if (cmdINT == SQLITE_OK) {
		//Command successfully executed
		return 100;
	}
	

	return 99;

}