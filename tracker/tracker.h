#ifndef SERVER_HEADER 
#define SERVER_HEADER


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
#include <pthread.h>
#include <string>
#include <sstream>
#include <queue>
#include <mutex>
#include <set>
using namespace std;

extern queue<string> cmds;
// extern int flag;
extern string logFileName, tracker1_ip, tracker2_ip, set_tracker_IP, seederFileName;
extern uint16_t tracker1_port, tracker2_port, set_tracker_port;
extern unordered_map<string, bool> is_logged_in;
extern unordered_map<string, string> login_creds;
extern int current_tracker_id;
extern int other_tracker_read_socket_fd;
extern int other_tracker_write_socket_fd;
extern int connected_trackers;
extern mutex log_mutex;
extern unordered_map<string, string> fileSize;//filename--> size
extern unordered_map<string, string> grpAdmins;
extern vector<string> allGroups;
extern unordered_map<string, set<string>> groupMembers;
extern unordered_map<string, set<string>> grpPendngRequests;
extern unordered_map<string, string> unameToPort;
extern unordered_map<string, string> userToGroup;//for tracker-tracker connection to know which user is in which group
extern unordered_map<string, string> piecewiseHash; //filename-->piecewiseshashes_separated_by_$$
extern unordered_map<string, unordered_map<string, set<string>>> seederList; // {map of==> (groupid -> {map of==>(filenames -> peer address)})}
extern vector<int> active_client_sockets;
extern mutex client_list_mutex;


void execute_cmd_tracker(int &clinet_socket,vector<string> inpt,int flag/*if flag==1 means its from sync cmds ...so it wants something back to knwo tht its write was successful*/,string );
void execute_cmd_client(int &client_socket,vector<string> inpt,string& ,string&);
void leave_group(vector<string>, int, string);
void accept_request(vector<string>, int, string);
void list_requests(vector<string>, int, string);
void join_group(vector<string>, int, string);
void list_groups(vector<string>, int);
int create_group(vector<string>, int, string);
int validateLogin(vector<string>);
int createUser(vector<string>);
void ping_first();
void synchronize_prev_cmds();//will one by one write all cmds in cmds string to other tracker
void write_to_other_tracker(const string &message);
void writeLog(const string &message);
void clearLog();
void initialize_args(int &argc, char* argv[]);
void create_log_file(string &tracker_info_file);
void* check_if_exit(void*);
void manage_connection(int client_socket_fd,int fg);
void initial_manage_connection(int &client_socket,string new_cmd);
vector<string> splitstring(string, string);
void downloadFile(vector<string> inpt, int client_socket, string client_uid,string client_gid);
void uploadFile(vector<string> inpt, int client_socket, string client_uid);
void list_files(vector<string> inpt, int client_socket);
void stop_share(vector<string> inpt, int client_socket, string client_uid);

#endif