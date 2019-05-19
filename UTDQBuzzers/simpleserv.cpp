/* Class for hosting a buzzer server without SQL integration 
(saves the ranks and scores in a hashmap, so scores reset every time the app is run)
 *
 *
 *
 *
 */
#include <iostream>
#include "mongoose.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm>

using namespace std;

static sig_atomic_t s_signal_received = 0;

//Changes these values
string ipAddress = "ws://localhost";
string adminSecret = "v3rys3cur3p@ss";
string portNumber = "80";

static const char *s_http_port = portNumber.c_str();
static struct mg_serve_http_opts s_http_server_opts;
unordered_map<string, struct mg_connection*> connectionNames;
unordered_map<struct mg_connection*, string> connectionToNames;
unordered_map<string, int> scores;
unordered_map<int, string> ranks;
int rankOn = 1;
string roomAdmin = "";


bool lockedout = false;

vector<string> splitup(string stringToSplit) {
	vector<string> finalSplit;
	string currentChunk = "";
	for (int i = 0; i < stringToSplit.length(); i++) {
		char currentChar = stringToSplit[i];
		if (currentChar == ':' || i == stringToSplit.length() - 1) {
			if (i == stringToSplit.length() - 1) {
				currentChunk += currentChar;
			}
			if (currentChunk.length() > 0) {
				finalSplit.push_back(currentChunk);
			}
			currentChunk = "";
		}
		else {
			currentChunk += currentChar;
		}
	}
	return finalSplit;

}

int getPlayerRank(string theplayer) {
	for (int i = 1; i < rankOn; i++) {
		if (ranks[i] == theplayer) {
			return i;
		}
	}
	return -1;
}

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

		struct mg_connection *c;
		char buf[702];
		char addr[32];
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
			MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

		//_snprintf_s(buf, sizeof(buf), "%s %.*s", addr, (int)msg.len, msg.p);
		_snprintf_s(buf, sizeof(buf), (int)msg.len, msg.p);
		//_snprintf_s(buf, sizeof(buf), "%.", (int)msg.len, msg.p);
		//buf[(int)msg.len] = '\\';
		//buf[(int)msg.len + 1] = 'n';
		printf("%s\n", buf); /* Local echo. */
		for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
			//if (c == nc) continue; /* Don't send to the sender. */
			mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
			
		}

	}
	else {
		cout << "Message was too large to send! (max length: " << maxMsgLength << ")" << endl;

	}
}


static void handle_buzzer(struct mg_connection *servcon, int ev, void *p) {
	
	struct mbuf *io = &servcon->recv_mbuf;

	mbuf_remove(io, io->len);
	//mbuf_remove(io, io->len);
	//

	
	string bufferString = string(io->buf, (int)io->len);
	if (bufferString.length() > 0) {
		cout << "buffer length now: " << (int)io->len << endl;
		
	}
	
	
	struct http_message *hm = (struct http_message *) p;
	char user[50];
	
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	
	string username = "";
	if (ul > 0) {

		for (int i = 0; i < ul; i++) {
			username += user[i];

		}

	}
	else {
		//mg_printf(servcon, "\n");
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string fileString;
		ifstream file("templates/loginbuzzer.html");
		if (file.is_open()) {
			std::string line;
			while (getline(file, line)) {
				//cout << "Line is:" << line << endl;
				if (line == "enterusernameline;") {
					fileString += "playerName = '" + username + "';";
				}
				
				else {
					fileString += line;
				}
				fileString += "\n";
			}
			file.close();
		}
		//fileString += "asd";
		mg_printf(servcon, fileString.c_str());
		//mg_printf(servcon, "test");
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}
	//mg_printf(servcon, "\n");
	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	string fileString;
	ifstream file("templates/buzzer.html");
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			
			if (line == "enterusernameline;") {
				fileString += "playerName = '" + username + "';";
			}
			else if (line == "enteripaddress;") {
				fileString += "ip = '" + ipAddress + "';";
			}
			else {
				fileString += line;
			}
			fileString += "\n";
		}
		file.close();
	}
	const char *  finalString = fileString.c_str();
	mg_printf(servcon, finalString);

	//struct mbuf *io = &servcon->recv_mbuf;
	int bufLength = (int)io->len;
	char* startPoint = io->buf;
	//string bufferString = string(startPoint, bufLength);

	cout << "buffer string:"<< bufferString << endl;

	servcon->flags |= MG_F_SEND_AND_CLOSE;
}


static void sendUpdatedScores(struct mg_connection *nc) {
	string clearCMD = "clearScoreboard:9012983123\n";
	mg_str clearCMDstr = mg_mk_str(clearCMD.c_str());
	broadcast(nc, clearCMDstr);
	for (int i = 1; i < rankOn; i++) {
		string playerName = ranks[i];
		string playerScore = to_string(scores[playerName]);
		string playerRank = to_string(i);
		string updateMsg = "updateScore:091239231:" +playerRank+":" + playerName + ":"+playerScore;
		cout << "Score update: " << updateMsg << endl;
		struct mg_str msg = mg_mk_str(updateMsg.c_str());
		broadcast(nc, msg);
		/*
		char buf[700];
		char addr[32];
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
			MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

		_snprintf_s(buf, sizeof(buf), "%s %.*s", addr, (int)msg.len, msg.p);
		printf("%s\n", buf); /* Local echo. 

		mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, buf, strlen(buf));
		*/
	}
	
}

static void handle_admin(struct mg_connection *servcon, int ev, void *p) {
	struct mbuf *io = &servcon->recv_mbuf;
	

	mbuf_remove(io, io->len);

	struct http_message *hm = (struct http_message *) p;
	char user[50];
	char secr[100];
	int ul = mg_get_http_var(&hm->body, "username", user, sizeof(user));
	int sl = mg_get_http_var(&hm->body, "secret", secr, sizeof(secr));
	string username = "";
	string secret = "";
	if (ul > 0 && sl > 0) {

		for (int i = 0; i < ul; i++) {
			username += user[i];

		}
		for (int i = 0; i < sl; i++) {
			secret += secr[i];

		}

	}
	if(ul == 0 || sl == 0 || secret != adminSecret) {
		//mg_printf(servcon, "\n");
		mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
		string fileString;
		ifstream file("templates/loginadmin.html");
		if (file.is_open()) {
			std::string line;
			while (getline(file, line)) {
				if (line == "enterusernameline;") {
					fileString += "playerName = '" + username + "';";
				}
				else {
					fileString += line;
				}
				fileString += "\n";
			}
			file.close();
		}
		//fileString += "asd";
		mg_printf(servcon, fileString.c_str());
		//mg_printf(servcon, "test");
		servcon->flags |= MG_F_SEND_AND_CLOSE;
		return;
	}
	//mg_printf(servcon, "\n");
	mg_printf(servcon, "HTTP/1.0 200 OK\r\n\r\n");
	string fileString;
	ifstream file("templates/scoreReceive.html");
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			if (line == "enterusernameline;") {
				fileString += "adminName = '" + username + "';";
			}
			else if (line == "enteripaddress;") {
				//cout << "Line found!";
				fileString += "ip = '" + ipAddress + "';";
			}
			else {
				fileString += line;
			}
			fileString += "\n";
		}
		file.close();
	}
	const char *  finalString = fileString.c_str();
	mg_printf(servcon, finalString);


	servcon->flags |= MG_F_SEND_AND_CLOSE;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	switch (ev) {
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
		/* New websocket connection. Tell everybody. */
		//broadcast(nc, mg_mk_str("++ joined"));
		break;
	}
	case MG_EV_WEBSOCKET_FRAME: {
		struct websocket_message *wm = (struct websocket_message *) ev_data;
		/* New websocket message. Tell everybody. */
		struct mg_str d = { (char *)wm->data, wm->size };
		string messageStr(d.p, d.len);
		vector<string> splitupCmd = splitup(messageStr);
		if (splitupCmd.size() >= 3) {
			string cmdName = splitupCmd[0];
			if (cmdName == "auth" && splitupCmd[1] == "1821831") {
				string playerName = splitupCmd[2];
				connectionNames[playerName] = nc;
				connectionToNames[nc] = playerName;
				if (scores.find(playerName) == scores.end()) {
					//Player doesn't have a score-- let's initialize it!
					scores[playerName] = 0;
					ranks[rankOn] = playerName;
					rankOn++;
				}

				cout << "Added user: " << playerName <<endl;
				sendUpdatedScores(nc);
				string cmd = "addUser:" + playerName;
				//broadcast(nc, mg_mk_str(cmd.c_str()));
			}
			if (cmdName == "authAdmin" && splitupCmd[1] == "1821839091231") {
				string playerName = splitupCmd[2];
				roomAdmin = playerName;
				connectionNames[playerName] = nc;
				connectionToNames[nc] = playerName;
				sendUpdatedScores(nc);
				cout << "Added admin: " << playerName << endl;
			}
			if (cmdName == "resetScoreboard" && splitupCmd[1] == "4203491023") {
				cout << "resetting scoreboard!" << endl;
				for (int i = 0; i < rankOn; i++) {
					string playerName = ranks[i];
					scores[playerName] = 0;
					
				}
				struct mg_connection *c;
				for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
					sendUpdatedScores(c);
				}
			}
			if (cmdName == "addScore" && splitupCmd[1] == "231308123") {
				//string cmdToSend = "addUserScore:"+splitupCmd[2]+":"+splitupCmd[3];
				string playerName = splitupCmd[2];
				cout << "Added " + splitupCmd[3] + " to " + splitupCmd[2] + "'s score " << endl;
				int scoreToAdd = stoi(splitupCmd[3]);
				scores[playerName] += scoreToAdd;
				if (scoreToAdd > 0) {
					bool moveupLadder = true;
					//Moves player up the ladder based on their score
					while (moveupLadder) {
						int playerRank = getPlayerRank(playerName);
						int nextRank = playerRank - 1;
						if (nextRank <= 0 || scores[ranks[nextRank]] >= scores[playerName]) {
							break;
						}
						ranks[playerRank] = ranks[nextRank];
						ranks[nextRank] = playerName;
					}
				}
				else {
					bool movedownLadder = true;
					//Moves player up the ladder based on their score
					while (movedownLadder) {
						int playerRank = getPlayerRank(playerName);
						int nextRank = playerRank + 1;
						if (nextRank >= rankOn || scores[ranks[nextRank]] <= scores[playerName]) {
							break;
						}
						ranks[playerRank] = ranks[nextRank];
						ranks[nextRank] = playerName;
					}
				}
				sendUpdatedScores(nc);
				//broadcast(nc, mg_mk_str(cmdToSend.c_str()));
			}
			
			if (cmdName == "buzz" && splitupCmd[1] == "28023180" && !lockedout && getPlayerRank(splitupCmd[2]) != -1){
				lockedout = true;
				string playerBuzz = "buzzedIn:"+splitupCmd[2];
				broadcast(nc, mg_mk_str(playerBuzz.c_str()));

			}

			if (cmdName == "reset" && splitupCmd[1] == "79274239" && splitupCmd[2] == roomAdmin) {
				cmdName += "231231";
				lockedout = false;
				broadcast(nc, mg_mk_str(cmdName.c_str()));
			}
			if (cmdName == "deletePlayer" && splitupCmd[1] == "82317012406123") {
				string playerName = splitupCmd[2];
				int playerRank = getPlayerRank(playerName);
				if (playerRank != -1) {
					rankOn--;
					for (int i = playerRank; i < rankOn; i++) {
						ranks[i] = ranks[i + 1];
					}
					ranks.erase(rankOn + 1);
					scores.erase(playerName);
					struct mg_connection *c;
					for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
						sendUpdatedScores(c);
					}
					string broadcastCMD = "deletePlayer:213101274912:" + playerName;
					broadcast(nc, mg_mk_str(broadcastCMD.c_str()));
				}
				

			}
		}
		//broadcast(nc, d);
		break;
	}
	case MG_EV_HTTP_REQUEST: {
		mg_serve_http(nc, (struct http_message *) ev_data, s_http_server_opts);
		break;
	}
	case MG_EV_CLOSE: {
		/* Disconnect. Tell everybody. */
		if (is_websocket(nc)) {
			string playerName = connectionToNames[nc];
			connectionNames.erase(playerName);
			connectionToNames.erase(nc);
			broadcast(nc, mg_mk_str("-- left"));
		}
		break;
	}
	}
}

int main(void) {
	ipAddress += ":"+portNumber;
	struct mg_mgr mgr;
	struct mg_connection *nc;

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);


	mg_mgr_init(&mgr, NULL);

	nc = mg_bind(&mgr, s_http_port, ev_handler);
	mg_register_http_endpoint(nc, "/buzzer", handle_buzzer);
	mg_register_http_endpoint(nc, "/admin", handle_admin);
	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = "htdocs";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";
	printf("UTDQBuzzers \n");
	printf("A Program by " "\x1b[34m"  "Abdullah Akbar\n" "\x1b[0m");
	printf("Webserver started on port %s\n", s_http_port);
	while (s_signal_received == 0) {
		mg_mgr_poll(&mgr, 200);
	}
	mg_mgr_free(&mgr);

	return 0;
}