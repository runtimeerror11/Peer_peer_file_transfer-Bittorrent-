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
#include <sys/socket.h> 
#include <sys/stat.h>
#include <fcntl.h>      // for open() and O_RDONLY
#include <unistd.h> 
#include <openssl/sha.h>
#include <math.h>
#include <queue>
#include <fstream>
#include <mutex>
#include <algorithm>  // for remove_if
#include <cctype>  
#include <random>   // For thread-safe random numbers
#include <chrono> 
#include <iomanip>

#include "client.h"
#include "threadss.h"
#include <fstream>
#include <future>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <vector>
#include <thread>
#include <cmath>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#define SIZE 32768//32kb
#define BLOCK_SIZE 524288
#define maxThreads 6
using namespace std;


extern string logFileName, tracker1_ip, tracker2_ip, set_client_IP;
extern uint16_t tracker1_port, tracker2_port, set_client_port;
extern string connected_tracker_ip;
extern uint16_t connected_tracker_port;
extern bool loggedIn;
extern unordered_map<string, unordered_map<string, bool>> isUploaded; // group -> filename -> bool
extern unordered_map<string, string> fileToFilePath;
extern unordered_map<string, vector<int>> fileChunkInfo;
extern unordered_map<string, string> downloaded_files;//filename-->grp_id
extern vector<string> file_block_hash;//
extern vector<vector<string>> blockwise_seeders;
extern bool isCorruptedFile;
extern std::mutex mtx; // mutex for thread-safe access


vector<string> splitString(string address, string delim);
void connection_to_tracker(int &socket_fd, struct sockaddr_in &address, int flag = 1);
void writeLog(const string &message);
void clearLog();

void process_args(int &argc, char* argv[]);
void create_log_file(string &tracker_info_file);
int execute_cmd(vector<string> &inpt, int sock);
void* client_as_server(void*);
void reply_client_to_client_socket(int client_socket_fd);
bool check_if_path_is_correct(const string &s1);
int uploadFile(vector<string> &inpt, int client_socket);
string get_peicewise_Hash(char*);//get_peicewise_Hash ==> getHash
long long getFileSize(const string &fromPath);
void hash_of_string(string, string&);
string getFileHash(char *);
void setChunkVector(string filename, long long l, long long r, bool isUpload);
int downloadFile(vector<string> &inpt, int client_socket);
void download_from_peers(vector<string> &inpt, vector<string> &seeder_info);
void get_chunk_vectors(string ip_port, string filename, long long num_of_blocks);
int request_client_to_client_socket(int flag, string ip_port, string filename);
int writeChunk(int peersock, long long chunkNum, char *filepath);
void sendChunk(char* filepath, int chunkNum, int client_socket);
void list_files(int sock);
string calculate_sha1(const vector<char> &data);
string get_block_hash(const string &filepath, int blockNum);
void show_downloads();

#endif

/*
string logFileName, tracker1_ip, tracker2_ip, peer_ip, seederFileName;
uint16_t peer_port, tracker1_port, tracker2_port;
bool loggedIn;*/