/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */
#include <iostream>
#include "mongoose.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "sqliteDB.h"
#include <fstream>

using namespace std;

static sig_atomic_t s_signal_received = 0;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
vector<mg_connection *> connections;
unordered_map<string, vector<mg_connection>> roomConnections;
sqliteDB * sqlDatabase = new sqliteDB();


static void signal_handler(int sig_num) {
	signal(sig_num, signal_handler);  // Reinstantiate signal handler
	s_signal_received = sig_num;
}

static int is_websocket(const struct mg_connection *nc) {
	return nc->flags & MG_F_IS_WEBSOCKET;
}

static void broadcast(struct mg_connection *nc, const struct mg_str msg) {
	int maxMsgLength = 700;
	if ((int)msg.len <= maxMsgLength) {
		string messageString(msg.p, msg.len);
		if (messageString == "hello") {
			cout << "Huwwo!" << endl;
		}
		//struct mg_connection *c;
		char buf[700];
		char addr[32];
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
			MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

		_snprintf_s(buf, sizeof(buf), "%s %.*s", addr, (int)msg.len, msg.p);
		printf("%s\n", buf); /* Local echo. */
		
		/*for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
			if (c == nc) continue; 
			mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
		}*/
		for (int i = 0; i < connections.size(); i++) {
			struct mg_connection *c = connections[i];
			if (c != nc) {
				mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));

			}
		}

	}
	else {
		cout << "Message was too large to send! (max length: " << maxMsgLength << ")" << endl;

	}
}




static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	switch (ev) {
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
		/* New websocket connection. Tell everybody. */
		broadcast(nc, mg_mk_str("++ joined"));
		connections.push_back(nc);
		break;
	}
	case MG_EV_WEBSOCKET_FRAME: {
		struct websocket_message *wm = (struct websocket_message *) ev_data;
		/* New websocket message. Tell everybody. */
		struct mg_str d = { (char *)wm->data, wm->size };
		broadcast(nc, d);
		break;
	}
	case MG_EV_HTTP_REQUEST: {
		mg_serve_http(nc, (struct http_message *) ev_data, s_http_server_opts);
		break;
	}
	case MG_EV_CLOSE: {
		/* Disconnect. Tell everybody. */
		if (is_websocket(nc)) {
			broadcast(nc, mg_mk_str("-- left"));
		}
		break;
	}
	}
}


/*

HTTP EVENT HANDLERS


*/

static void handle_login(struct mg_connection *servcon, int ev, void *p) {
	char pass[200];
	char user[50];
	struct http_message *hm = (struct http_message *) p;
	int pl = mg_get_http_var(&hm->body, "password", pass, sizeof(pass));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	string password = "";
	string username = "";
	if (pl > 0 && user > 0) {
		
		for (int i = 0; i < pl; i++) {
			password += pass[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		
	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "602";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int loginResult = sqlDatabase->login(username, password);
	if (loginResult != 100) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(loginResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}
	
	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	int userSessID = sqlDatabase->createSessID(username);
	string theMsg = "Successfully logged in with sessID: "+to_string(userSessID);
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void handle_signup(struct mg_connection *servcon, int ev, void *p) {
	char passOne[200];
	char passTwo[200];
	char user[50];
	char name[100];
	struct http_message *hm = (struct http_message *) p;
	int pl = mg_get_http_var(&hm->body, "passwordOne", passOne, sizeof(passOne));
	int pl2 = mg_get_http_var(&hm->body, "passwordTwo", passTwo, sizeof(passTwo));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	int nl = mg_get_http_var(&hm->body, "name", name, sizeof(name));
	string passwordOne = "";
	string passwordTwo = "";
	string username = "";
	string thename = "";
	if (pl > 0 && user > 0 && pl2 > 0) {

		for (int i = 0; i < pl; i++) {
			passwordOne += passOne[i];

		}
		for (int i = 0; i < pl2; i++) {
			passwordTwo += passTwo[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		for (int i = 0; i < nl; i++) {
			thename += name[i];

		}

	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "504";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	if (passwordOne != passwordTwo) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "503";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int signupResult = sqlDatabase->signup(username, thename, passwordOne, passwordTwo);
	if (signupResult != 100) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(signupResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	int userSessID = sqlDatabase->createSessID(username);
	string theMsg = "Signed up "+username + " with sessID: " + to_string(userSessID);
	
	
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void handle_createadmin(struct mg_connection *servcon, int ev, void *p) {
	char passOne[200];
	char passTwo[200];
	char user[50];
	char secr[200];
	char name[100];
	struct http_message *hm = (struct http_message *) p;
	int pl = mg_get_http_var(&hm->body, "passwordOne", passOne, sizeof(passOne));
	int pl2 = mg_get_http_var(&hm->body, "passwordTwo", passTwo, sizeof(passTwo));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	int sl = mg_get_http_var(&hm->body, "secret", secr, sizeof(secr));
	int nl = mg_get_http_var(&hm->body, "name", name, sizeof(name));
	string passwordOne = "";
	string passwordTwo = "";
	string username = "";
	string secret = "";
	string thename = "";
	if (pl > 0 && user > 0 && nl > 0 && pl2 > 0 && sl > 0) {

		for (int i = 0; i < pl; i++) {
			passwordOne += passOne[i];

		}
		for (int i = 0; i < pl2; i++) {
			passwordTwo += passTwo[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		for (int i = 0; i < sl; i++) {
			secret += secr[i];

		}
		for (int i = 0; i < nl; i++) {
			thename += name[i];

		}

	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "504";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	if (passwordOne != passwordTwo) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "503";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int signupResult = sqlDatabase->createAdmin(username, thename, passwordOne, passwordTwo, secret);
	if (signupResult != 100) {
		
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(signupResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	int userSessID = sqlDatabase->createSessID(username);
	string theMsg = "Succesfully created admin: " + username + " with sessID: " + to_string(userSessID);
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void handle_checksess(struct mg_connection *servcon, int ev, void *p) {
	char sess[200];
	char user[50];
	struct http_message *hm = (struct http_message *) p;
	int sl = mg_get_http_var(&hm->body, "sessID", sess, sizeof(sess));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	string sessID = "";
	string username = "";
	if (sl > 0 && user > 0) {

		for (int i = 0; i < sl; i++) {
			sessID += sess[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}

	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "602";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int sessResult = sqlDatabase->userSessMatch(username, sessID, 0);
	if (sessResult != 100) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(sessResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	string theMsg = "Pass " + username + " Sent";
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void handle_roomjoin(struct mg_connection *servcon, int ev, void *p) {
	char sess[200];
	char room[100];
	char user[50];
	struct http_message *hm = (struct http_message *) p;
	int sl = mg_get_http_var(&hm->body, "sessID", sess, sizeof(sess));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	int rl = mg_get_http_var(&hm->body, "roomname", user, sizeof(user));
	string sessID = "";
	string username = "";
	string roomname = "";
	if (sl > 0 && user > 0) {

		for (int i = 0; i < sl; i++) {
			sessID += sess[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		for (int i = 0; i < rl; i++) {
			roomname += room[i];

		}

	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "602";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int sessResult = sqlDatabase->userSessMatch(username, sessID, 0);
	if (sessResult != 100) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(sessResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	string theMsg = "Pass " + username + " Sent";
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void handle_joinasmod(struct mg_connection *servcon, int ev, void *p) {
	char sess[200];
	char room[100];
	char user[50];
	struct http_message *hm = (struct http_message *) p;
	int sl = mg_get_http_var(&hm->body, "sessID", sess, sizeof(sess));
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	int rl = mg_get_http_var(&hm->body, "roomname", user, sizeof(user));
	string sessID = "";
	string username = "";
	string roomname = "";
	if (sl > 0 && user > 0) {

		for (int i = 0; i < sl; i++) {
			sessID += sess[i];

		}
		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		for (int i = 0; i < rl; i++) {
			roomname += room[i];

		}

	}
	else {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = "602";
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;

	}
	int sessResult = sqlDatabase->userSessMatch(username, sessID, 1);
	if (sessResult != 100) {
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string intAsString = to_string(sessResult);
		mg_printf(servcon, intAsString.c_str());
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}

	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	string theMsg = "Pass " + username + " Sent";
	const char *  passstring = theMsg.c_str();
	mg_printf(servcon, passstring);
	servcon->flags |= MG_F_SEND_AND_CLOSE;
}

int main(void) {
	string dbfilename = "databases/UTDQB.db";
	ifstream f(dbfilename.c_str());
	if (!f.good()) {
		sqlDatabase->createDB();
	}
	else {
		sqlDatabase->openDB();

	}
	sqlDatabase->initvars();

	struct mg_mgr mgr;
	struct mg_connection *nc;

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);


	mg_mgr_init(&mgr, NULL);

	nc = mg_bind(&mgr, s_http_port, ev_handler);
	mg_register_http_endpoint(nc, "/login", handle_login);
	
	mg_register_http_endpoint(nc, "/signup", handle_signup);
	mg_register_http_endpoint(nc, "/createadmin", handle_createadmin);
	mg_register_http_endpoint(nc, "/checksess", handle_checksess);
	mg_register_http_endpoint(nc, "/joinroom", handle_roomjoin);
	mg_register_http_endpoint(nc, "/joinasmod", handle_joinasmod);
	/*mg_register_http_endpoint(nc, "/createroom", handle_roomcreate);
	mg_register_http_endpoint(nc, "/addscore", handle_addscore);
	*/

	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = "htdocs";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";

	printf("Started on port %s\n", s_http_port);
	while (s_signal_received == 0) {
		mg_mgr_poll(&mgr, 200);
	}
	mg_mgr_free(&mgr);
	sqlDatabase->closeDB();
	return 0;
}