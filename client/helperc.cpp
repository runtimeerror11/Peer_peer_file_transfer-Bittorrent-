#include "client.h"
bool loggedIn = false;
string logFileName, tracker1_ip, tracker2_ip, set_client_IP;
uint16_t tracker1_port, tracker2_port, set_client_port;
string connected_tracker_ip;
uint16_t connected_tracker_port;
unordered_map<string, unordered_map<string, bool>> isUploaded; // group -> filename -> bool
unordered_map<string, string> fileToFilePath;
unordered_map<string, vector<int>> fileChunkInfo;
unordered_map<string, string> downloaded_files;
vector<string> file_block_hash;
vector<vector<string>> blockwise_seeders;
bool isCorruptedFile=false;

void clearLog()
{
    // open n clear it
    FILE *file = fopen(logFileName.c_str(), "w");
    if (file == NULL)
    {
        cerr << "Could not open log file: " << logFileName << endl;
        return;
    }
    fclose(file);
}
// give me function tht will split for the tht respective delimiter
void writeLog(const string &message)
{
    // append on newline
    // clearLog();
    FILE *file = fopen(logFileName.c_str(), "a");
    if (file == NULL)
    {
        cerr << "Could not open log file: " << logFileName << endl;
        return;
    }

    fprintf(file, "%s\n", message.c_str());
    fclose(file);
}
void create_log_file(string &tracker_info_file)
{
    logFileName = tracker_info_file + "_log.txt";
    FILE *file = fopen(logFileName.c_str(), "w");
    if (file == NULL)
    {
        cerr << "Could not create the log file: " << logFileName << endl;
        return;
    }
    fclose(file);
}

void process_args(int &argc, char *argv[])

{
    // this willl be input  tracker_info.txt tracker no - Start a tracker server....we need to set server accordingly and store its values in gloabla as
    string tracker_info_file = argv[2];
    vector<string> parts = splitString(argv[1], ":");
    if (parts.size() != 2)
    {
        cerr << "Invalid address format. Use <IP>:<PORT>\n";
        exit(1);
    }
    set_client_IP = parts[0];
    set_client_port = static_cast<uint16_t>(stoi(parts[1]));

    // create a log file and also write function for it
    create_log_file(tracker_info_file);
    // use cstyle file hanling
    FILE *file = fopen(tracker_info_file.c_str(), "r");
    if (file == NULL)
    {
        cerr << "Could not open the file: " << tracker_info_file << endl;
        return;
    }
    char line[256];
    int current_line = 0;
    while (fgets(line, sizeof(line), file))
    {
        string str_line(line);
        str_line.erase(str_line.find_last_not_of(" \n\r\t") + 1); // Trim trailing whitespace
        if (current_line == 0)
        {
            tracker1_ip = str_line;
        }
        else if (current_line == 1)
        {
            tracker1_port = static_cast<uint16_t>(stoi(str_line));
        }
        else if (current_line == 2)
        {
            tracker2_ip = str_line;
        }
        else if (current_line == 3)
        {
            tracker2_port = static_cast<uint16_t>(stoi(str_line));
        }
        else if (current_line == 4)
        {
            logFileName = str_line;
        }
        current_line++;
    }
    fclose(file);

    // Debug prints to verify correct reading
    cout << "Tracker 1 IP: " << tracker1_ip << ", Port: " << tracker1_port << endl;
    cout << "Tracker 2 IP: " << tracker2_ip << ", Port: " << tracker2_port << endl;
    cout << "Current client IP: " << set_client_IP << ", Port: " << set_client_port << endl;
    cout << "Log File: " << logFileName << endl;
}

void connection_to_tracker(int &socket_fd, struct sockaddr_in &address, int flag)
{
    address.sin_family = AF_INET;
    if (flag == 1)
    {

        connected_tracker_ip = tracker1_ip;
        connected_tracker_port = tracker1_port;
    }
    else
    {

        connected_tracker_ip = tracker2_ip;
        connected_tracker_port = tracker2_port;
    }
    bool connected = true;
    address.sin_port = htons(connected_tracker_port);
    if (inet_pton(AF_INET, &connected_tracker_ip[0], &address.sin_addr) <= 0)
    {
        connected = false;
    }
    if (connected && connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        connected = false;
    }
    if (!connected && flag == 1)
    {
        writeLog("Could not connect to Tracker 1. Trying Tracker 2.");
        cout << "Could not connect to Tracker 1. Trying Tracker 2.\n";
        connection_to_tracker(socket_fd, address, 2);
        return;
    }
    if (!connected && flag == 2)
    {
        writeLog("Could not connect to Tracker 2. Exiting.");
        cout << "Could not connect to Tracker 2. Exiting.\n";
        exit(1);
    }
    cout << "Connected to server with Tracker" << flag << " at " << connected_tracker_ip << ":" << connected_tracker_port << "\n";
}

vector<string> splitString(string address, string delim = ":")
{
    vector<string> res;

    size_t pos = 0;
    while ((pos = address.find(delim)) != string::npos)
    {
        string t = address.substr(0, pos);
        res.push_back(t);
        address.erase(0, pos + delim.length());
    }
    res.push_back(address);

    return res;
}


bool check_if_path_is_correct(const string& s1) {
    string s = s1;
    // Trim trailing spaces/newlines
    s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());

    struct stat buffer;
    int i = stat(s.c_str(), &buffer);
    cout << "Checking path: [" << s << "], stat result=" << i << endl;

    if (i != 0) {
        perror("stat failed");  // prints exact reason (e.g. No such file, Permission denied)
    }

    return i == 0;
}


void setChunkVector(string filename, long long l, long long r, bool isUpload)
{
    if (isUpload)
    { // setting everything to 1 as it holds all chunks!!
        vector<int> tmp(r - l + 1, 1);
        fileChunkInfo[filename] = tmp;
    }
    else
    { // setting specific chunk as 1 !! as it is downloader!!
        fileChunkInfo[filename][l] = 1;
        writeLog("chunk vector updated for " + filename + " at " + to_string(l));
    }
}