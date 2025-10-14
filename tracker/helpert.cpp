#include "tracker.h"
string logFileName, tracker1_ip, tracker2_ip, set_tracker_IP, seederFileName;
uint16_t tracker1_port, tracker2_port, set_tracker_port;
unordered_map<string, bool> is_logged_in;
unordered_map<string, string> login_creds;
int current_tracker_id;
int other_tracker_read_socket_fd = -1;
int other_tracker_write_socket_fd = -1;
 unordered_map<string, string> fileSize;
 unordered_map<string, string> grpAdmins;
 vector<string> allGroups;
 unordered_map<string, set<string>> groupMembers;
 unordered_map<string, set<string>> grpPendngRequests;
 unordered_map<string, string> unameToPort;
unordered_map<string, string> userToGroup;
int connected_trackers = 0;
mutex log_mutex;
vector<int> active_client_sockets;
mutex client_list_mutex;

queue<string> cmds;


 unordered_map<string, string> piecewiseHash; 
 unordered_map<string, unordered_map<string, set<string>>> seederList; 

// let me explain u my idea...see whenever the other tracker is cretead it will ping this tracker ...it iwll be handled as normal client thread...so now we need ot modify the read in clinet thread tht first read will be outside of loop
// for this loop if its normal client the it will send its commn ...i =f not then tracker will send the $ ...now we know tht thread is executig the socket for thread...this will be read only...now this socket will create another socket as write to other socket...the other socket will wait till this all happens...then will will start listening for other clients too....also before listenin gto other clinets...the older tracker s it was string cmds...it will pop them out and write thos one by one in tht write socket to newly creatd tracker....after completion all will execute onrmally in both tracker...
//  now eaceh tracker on receving cmd will write it to write socket to other cmd direclty only tht new_cmd and will execut too....thts all my logic
//  within tht thread of read from socket of other tracker we will have flag==1 indicating its read from the other trackre and thus wont forward those cmds to other tracker like other clients!!

void write_to_other_tracker(const string &message)
{
    lock_guard<mutex> lock(log_mutex);
    if (other_tracker_write_socket_fd != -1)
    {     cout<<"Writing to other tracker: "<<message<<endl;
        ssize_t sent = send(other_tracker_write_socket_fd, message.c_str(), message.size(), 0);
        cout << "Sent new command to other tracker: " << message << endl;
        if (sent == -1)
        {
            perror("Send to other tracker failed");
            std::cout << "Connection to other tracker lost. Continuing as standalone tracker.\n";
            close(other_tracker_write_socket_fd);
            other_tracker_write_socket_fd = -1;
        }
        else
        {
            writeLog("Sent command to other tracker: " + message);
        }
    }
    else
    {
        cout << "Other tracker write socket not connected. Command not sent: " << message << endl;
        writeLog("Other tracker write socket not connected. Command not sent: " + message);
    }
}

void ping_first()
{
    // this function will try to connect to other tracker and if it is online then it will set the other_tracker_socket_fd
    string other_tracker_ip;
    uint16_t other_tracker_port;
    if (current_tracker_id == 1)
    {
        other_tracker_ip = tracker2_ip;
        other_tracker_port = tracker2_port;
    }
    else
    {
        other_tracker_ip = tracker1_ip;
        other_tracker_port = tracker1_port;
    }
    int other_socket_fd;
    struct sockaddr_in other_address;
    if ((other_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed for other tracker");
        return;
    }
    other_address.sin_family = AF_INET;
    other_address.sin_port = htons(other_tracker_port);
    if (inet_pton(AF_INET, other_tracker_ip.c_str(), &other_address.sin_addr) <= 0)
    {
        std::cout << "Invalid address/ Address not supported for other tracker\n";
        return;
    }
    if (connect(other_socket_fd, (struct sockaddr *)&other_address, sizeof(other_address)) < 0)
    {
        std::cout << "Other tracker is not online. Continuing as standalone tracker.\n";
        return;
    }
    other_tracker_write_socket_fd = other_socket_fd;
    // write $ in it to indicate its from the other tracker
    string msg = "$";
    ssize_t sent = send(other_tracker_write_socket_fd, msg.c_str(), msg.size(), 0);
    if (sent == -1)
    {
        perror("Send to other tracker failed");
        close(other_tracker_write_socket_fd);
        other_tracker_write_socket_fd = -1;
        return;
    }
    other_tracker_read_socket_fd = other_socket_fd;

    std::cout << "Connected to other tracker at " << other_tracker_ip << ":" << other_tracker_port << "\n";
    writeLog("Connected to other tracker at " + other_tracker_ip + ":" + to_string(other_tracker_port));
}

void synchronize_prev_cmds()
{
    // here we will check setid and set ips and then we will ping connect the other tracker...and if it is online it willsend the old cmds which is has stored in its string cmds....and this tracker will now updates its....datastrcutures only not execute thise cmds...make somse flasg too...and from noew on if flag is set ...means other tracker is also active nad then from now on if this tracker receive the cmd with will send same to the other trackers which will execute tt cmd too...thus data structure will be synced
    // if setid is 1 then set ip to tracker1 else tracker2
    // ping the other tracker if it is online then send the cmds string to it
    // other tracker will execute the cmds and update its data structures only
    // set a flag to indicate that both trackers are active
    // from now on if flag is set then send cmds to other tracker for every cmd received
    // else do nothing
    // code please

    // now send the old cmds to other tracker by creting a new socket for writing to other tracker
    // first make socket for writign to other tracker and assign itot other_tracker_write_socket_fd
    if (other_tracker_write_socket_fd == -1)
    {
        // crete new socket to write to other tracker
        string other_tracker_ip;
        uint16_t other_tracker_port;
        if (current_tracker_id == 1)
        {
            other_tracker_ip = tracker2_ip;
            other_tracker_port = tracker2_port;
        }
        else
        {
            other_tracker_ip = tracker1_ip;
            other_tracker_port = tracker1_port;
        }
        int other_socket_fd;
        struct sockaddr_in other_address;
        if ((other_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation failed for other tracker");
            return;
        }
        other_address.sin_family = AF_INET;
        other_address.sin_port = htons(other_tracker_port);
        if (inet_pton(AF_INET, other_tracker_ip.c_str(), &other_address.sin_addr) <= 0)
        {
            std::cout << "Invalid address/ Address not supported for other tracker\n";
            return;
        }
        if (connect(other_socket_fd, (struct sockaddr *)&other_address, sizeof(other_address)) < 0)
        {
            std::cout << "Other tracker is not online. Continuing as standalone tracker.\n";
            return;
        }
        other_tracker_write_socket_fd = other_socket_fd;
        char temp[1024];
        while (!cmds.empty())
        {   cout<<cmds.size()<<" cmds to sync\n";
            memset(temp, 0, sizeof(temp));
            string new_cmd = cmds.front();
            cmds.pop();
            ssize_t sent = send(other_tracker_write_socket_fd, new_cmd.c_str(), new_cmd.size(), 0);
            if (sent == -1)
            {
                perror("Send to other tracker failed");
                std::cout << "Connection to other tracker lost. Continuing as standalone tracker.\n";
                close(other_tracker_write_socket_fd);
                other_tracker_write_socket_fd = -1;
                return;
            }
            read(other_tracker_write_socket_fd, temp, 1024);
        }
        cout<<"Done with the syncing of prev cmds\n   Now sending $ to indicate end of syncing\n";
        string new_cmd = "$";
        ssize_t sent = send(other_tracker_write_socket_fd, new_cmd.c_str(), new_cmd.size(), 0);
        if (sent == -1)
        {
            perror("Send to other tracker failed");
            std::cout << "Connection to other tracker lost. Continuing as standalone tracker.\n";
            close(other_tracker_write_socket_fd);
            other_tracker_write_socket_fd = -1;
            return;
        }

        std::cout << "Connected and synced to other tracker at " << other_tracker_ip << ":" << other_tracker_port << "\n";
        writeLog("Connected and synced to other tracker at " + other_tracker_ip + ":" + to_string(other_tracker_port));
    }
    else
    {
        cout << "Other_tracker_write_socket_fd already set\n";
        writeLog("Other_tracker_write_socket_fd already set");
    }
    return;
    // if (other_traker_socket_fd != -1)
    // {
    //     // send tht new cmd to other tracker from its socket
    //     if (new_cmd != "")
    //     {
    //         string msg = new_cmd + " $";
    //         ssize_t sent = send(other_traker_socket_fd, msg.c_str(), msg.size(), 0);
    //         if (sent == -1)
    //         {
    //             perror("Send to other tracker failed");
    //             std::cout << "Connection to other tracker lost. Continuing as standalone tracker.\n";
    //             close(other_traker_socket_fd);
    //             other_traker_socket_fd = -1;
    //         }
    //         else
    //         {
    //             std::cout << "Sent new new_cmd to other tracker: " << new_cmd << std::endl;
    //         }
    //     }
    //     return;
    // }
    // else
    // {
    //     string other_tracker_ip;
    //     uint16_t other_tracker_port;
    //     if (current_tracker_id == 1)
    //     {
    //         other_tracker_ip = tracker2_ip;
    //         other_tracker_port = tracker2_port;
    //     }
    //     else
    //     {
    //         other_tracker_ip = tracker1_ip;
    //         other_tracker_port = tracker1_port;
    //     }
    //     int other_socket_fd;
    //     struct sockaddr_in other_address;
    //     if ((other_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    //     {
    //         perror("Socket creation failed for other tracker");
    //         return;
    //     }
    //     other_address.sin_family = AF_INET;
    //     other_address.sin_port = htons(other_tracker_port);
    //     if (inet_pton(AF_INET, other_tracker_ip.c_str(), &other_address.sin_addr) <= 0)
    //     {
    //         std::cout << "Invalid address/ Address not supported for other tracker\n";
    //         return;
    //     }
    //     if (connect(other_socket_fd, (struct sockaddr *)&other_address, sizeof(other_address)) < 0)
    //     {
    //         std::cout << "Other tracker is not online. Continuing as standalone tracker.\n";
    //         return;
    //     }
    //     other_traker_socket_fd = other_socket_fd;
    //     std::cout << "Connected to other tracker. Synchronizing previous commands...\n";
    //     writeLog("Connected to other tracker at " + other_tracker_ip + ":" + to_string(other_tracker_port));
    //     // now send the old cmds to other tracker
    //     if (!cmds.empty())
    //     {
    //         ssize_t sent = send(other_traker_socket_fd, cmds.c_str(), cmds.size(), 0);
    //         if (sent == -1)
    //         {
    //             perror("Send to other tracker failed");
    //             std::cout << "Connection to other tracker lost. Continuing as standalone tracker.\n";
    //             close(other_traker_socket_fd);
    //             other_traker_socket_fd = -1;
    //             return;
    //         }
    //         else
    //         {
    //             std::cout << "Synchronized previous commands with other tracker.\n";
    //         }
    //     }
    // }
}

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

void initialize_args(int &argc, char *argv[])
{
    // this willl be input  tracker_info.txt tracker no - Start a tracker server....we need to set server accordingly and store its values in gloabla as
    string tracker_info_file = argv[1];

    // create a log file and also write function for it
    create_log_file(tracker_info_file);

    int tracker_no = stoi(argv[2]);
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
    if (tracker_no == 1)
    {
        set_tracker_IP = tracker1_ip;
        set_tracker_port = tracker1_port;
        current_tracker_id = 1;
    }
    else if (tracker_no == 2)
    {
        set_tracker_IP = tracker2_ip;
        set_tracker_port = tracker2_port;
        current_tracker_id = 2;
    }
    else
    {
        cerr << "Invalid tracker number. Use 1 or 2." << endl;
        return;
    }
    // Debug prints to verify correct reading
    cout << "Tracker 1 IP: " << tracker1_ip << ", Port: " << tracker1_port << endl;
    cout << "Tracker 2 IP: " << tracker2_ip << ", Port: " << tracker2_port << endl;
    cout << "Current tracker IP: " << set_tracker_IP << ", Port: " << set_tracker_port << endl;
    writeLog("Set tracker IP: " + set_tracker_IP + ", Port: " + to_string(set_tracker_port));
    cout << "Log File: " << logFileName << endl
         << endl;
}

void *check_if_exit(void *arg)
{
    string new_cmd;
    while (true)
    {
        cin >> new_cmd;
        if (new_cmd == "quit"|| new_cmd=="Quit")
        {
            std::cout << "Exit found!!!\n";
            writeLog("Exiting as per user new_cmd");

            // If other tracker is active, log out all users there
            if (other_tracker_write_socket_fd != -1)
            {
                lock_guard<mutex> lock(log_mutex);
                for (const auto& kv : is_logged_in)
                {
                    if (kv.second) // if user is logged in
                    {
                        string logout_cmd = "logout " + kv.first;
                        ssize_t sent = send(other_tracker_write_socket_fd, logout_cmd.c_str(), logout_cmd.size(), 0);
                        writeLog("Sent logout for user " + kv.first + " to other tracker");
                    }
                }
            }

            // Notify and disconnect all clients
            {
                lock_guard<mutex> lock(client_list_mutex);
                for (int sock : active_client_sockets) {
                    string msg = "TRACKER_EXIT";
                    send(sock, msg.c_str(), msg.size(), 0);
                    close(sock);
                }
                active_client_sockets.clear();
            }
            exit(0);
        }
    }
}

// thread will execute this function to communicate with client
// void manage_connection(int client_socket_fd,int fg)
// {
//     int flag = fg;
//     char buffer[1024];
//     memset(buffer, 0, sizeof(buffer));
//     int read_count = read(client_socket_fd, buffer, 1024);
//     // store new cmds and also append new commds to cmd which store all cmds
//     // check if it is ping tht is $ from other tracker

//     if (read_count <= 0)
//     {
//         writeLog("Error reading from socket");
//         close(client_socket_fd);
//         cout << "Closing connection from client\n";
//         close(client_socket_fd);
//         return;
//     }
//     string new_cmd(buffer, read_count);

//     if (new_cmd == "$")
//     {
//         flag = 1; // indicating dont direclty forward cmds to other tracker

//         other_tracker_read_socket_fd = client_socket_fd;

//         std::cout << "Ping from other tracker received.\n";
//         writeLog("Ping from other tracker received.");
//         synchronize_prev_cmds(); // send this new cmd to other tracker if it is online
//         connected_trackers = 1;
//     }
//     else
//     {
//         cout << ">> ";

//         if (flag == 0 && connected_trackers)
//         {
//             write_to_other_tracker(new_cmd);
//         }
//         else if (flag == 0 && !connected_trackers)
//         {
//             cmds.push(new_cmd); // no connection so store command
//         }
//         writeLog("Received new_cmd: " + new_cmd);
//         std::cout << "Received new_cmd: " << new_cmd << endl;
//         string response = new_cmd + " - Message from Tracker";
//         strncpy(buffer, response.c_str(), sizeof(buffer));
//         buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
//         send(client_socket_fd, buffer, strlen(buffer), 0);
//     }
//     while (true)
//     {
//         cout << ">> ";
//         memset(buffer, 0, sizeof(buffer));
//         int read_count = read(client_socket_fd, buffer, 1024);
//         if (read_count <= 0)
//         {
//             writeLog("Error reading from socket");
//             close(client_socket_fd);

//             break;
//         }
//         string command(buffer, read_count);
//         if (flag == 0 && connected_trackers)
//         {
//             write_to_other_tracker(command);
//         }
//         else if (flag == 0 && !connected_trackers)
//         {
//             cmds.push(command); // no connection so store command
//         }
//         // execute command on this tracker side;
//         writeLog("Received command: " + command);
//         cout << "Received command: " << command << endl;
//         // now read and send back data as received+msg appneded

//         string response = command + " - Message from Tracker";
//         strncpy(buffer, response.c_str(), sizeof(buffer));
//         buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
//         if (flag == 0)
//             send(client_socket_fd, buffer, strlen(buffer), 0);
//     }
//     cout << "Closing connection from client\n";
//     close(client_socket_fd);
// }

// void initial_manage_connection(string new_cmd)
// {
//     cout << "Received from other tracker! -->" << new_cmd << endl;
//     // string s, in = string(new_cmd);

//     // stringstream ss(in);
//     // vector<string> inpt;

//     // while(ss >> s){
//     //     inpt.push_back(s);
//     // }

//     // if(inpt[0] == "create_user"){
//     //     if(inpt.size() != 3){
//     //         write(client_socket, "Invalid argument count", 22);
//     //     }
//     //     else{
//     //         if(createUser(inpt) < 0){
//     //             write(client_socket, "User exists", 11);
//     //         }
//     //         else{
//     //             write(client_socket, "Account created", 15);
//     //         }
//     //     }
//     // }
//     // else if(inpt[0] == "login"){
//     //     if(inpt.size() != 3){
//     //         write(client_socket, "Invalid argument count", 22);
//     //     }
//     //     else{
//     //         int r;
//     //         if((r = validateLogin(inpt)) < 0){
//     //             write(client_socket, "Username/password incorrect", 28);
//     //         }
//     //         else if(r > 0){
//     //             write(client_socket, "You already have one active session", 35);
//     //         }
//     //         else{
//     //             write(client_socket, "Login Successful", 16);
//     //             client_uid = inpt[1];
//     //             char buf[96];
//     //             read(client_socket, buf, 96);
//     //             string peerAddress = string(buf);
//     //             unameToPort[client_uid] = peerAddress;
//     //         }
//     //     }
//     // }
//     // else if(inpt[0] ==  "logout"){
//     //     isLoggedIn[client_uid] = false;
//     //     write(client_socket, "Logout Successful", 17);
//     //     writeLog("logout sucess\n");
//     // }
//     // else if(inpt[0] == "upload_file"){
//     //     uploadFile(inpt, client_socket, client_uid);
//     // }
//     // else if(inpt[0] == "download_file"){
//     //     downloadFile(inpt, client_socket, client_uid);
//     //     writeLog("after down");
//     // }
//     // else if(inpt[0] == "create_group"){
//     //     if(create_group(inpt, client_socket, client_uid) >=0){
//     //         client_gid = inpt[1];
//     //         write(client_socket, "Group created", 13);
//     //     }
//     //     else{
//     //         write(client_socket, "Group exists", 12);
//     //     }
//     // }
//     // else if(inpt[0] == "list_groups"){
//     //     list_groups(inpt, client_socket);
//     // }
//     // else if(inpt[0] == "join_group"){
//     //     join_group(inpt, client_socket, client_uid);
//     // }
//     // else if(inpt[0] == "list_requests"){
//     //     list_requests(inpt, client_socket, client_uid);
//     // }
//     // else if(inpt[0] == "accept_request"){
//     //     accept_request(inpt, client_socket, client_uid);
//     // }
//     // else if(inpt[0] == "leave_group"){
//     //     leave_group(inpt, client_socket, client_uid);
//     // }
//     // else if(inpt[0] == "list_files"){
//     //     list_files(inpt, client_socket);
//     // }
//     // else{
//     //     write(client_socket, "Invalid new_cmd", 15);
//     // }
// }


// void handle_connection(int client_socket){
//     string client_uid = "";
//     string client_gid = "";
//     writeLog("***********pthread started for client socket number " + to_string(client_socket));

//     //for continuously checking the commands sent by the client
//     while(true){
//         char inptline[1024] = {0}; 

//         if(read(client_socket , inptline, 1024) <=0){
//             is_logged_in[client_uid] = false;
//             close(client_socket);
//             break;
//         }
//         writeLog("client request:" + string(inptline));

//         string s, in = string(inptline);
//         stringstream ss(in);
//         vector<string> inpt;

//         while(ss >> s){
//             inpt.push_back(s);
//         }

//         if(inpt[0] == "create_user"){
//             if(inpt.size() != 3){
//                 write(client_socket, "Invalid argument count", 22);
//             }
//             else{
//                 if(createUser(inpt) < 0){
//                     write(client_socket, "User exists", 11);
//                 }
//                 else{
//                     write(client_socket, "Account created", 15);
//                 }
//             }
//         }
//         else if(inpt[0] == "login"){
//             if(inpt.size() != 3){
//                 write(client_socket, "Invalid argument count", 22);
//             }
//             else{
//                 int r;
//                 if((r = validateLogin(inpt)) < 0){
//                     write(client_socket, "Username/password incorrect", 28);
//                 }
//                 else if(r > 0){
//                     write(client_socket, "You already have one active session", 35);
//                 }
//                 else{
//                     write(client_socket, "Login Successful", 16);
//                     client_uid = inpt[1];
//                     char buf[96];
//                     read(client_socket, buf, 96);
//                     string peerAddress = string(buf);
//                     unameToPort[client_uid] = peerAddress;
//                 }
//             }            
//         }
//         else if(inpt[0] ==  "logout"){
//             is_logged_in[client_uid] = false;
//             write(client_socket, "Logout Successful", 17);
//             writeLog("logout sucess\n");
//         }
      
//         else if(inpt[0] == "create_group"){
//             if(create_group(inpt, client_socket, client_uid) >=0){
//                 client_gid = inpt[1];
//                 write(client_socket, "Group created", 13);
//             }
//             else{
//                 write(client_socket, "Group exists", 12);
//             }
//         }
//         else if(inpt[0] == "list_groups"){
//             list_groups(inpt, client_socket);
//         }
//         else if(inpt[0] == "join_group"){
//             join_group(inpt, client_socket, client_uid);
//         }
//         else if(inpt[0] == "list_requests"){
//             list_requests(inpt, client_socket, client_uid);
//         }
//         else if(inpt[0] == "accept_request"){
//             accept_request(inpt, client_socket, client_uid);
//         }
//         else if(inpt[0] == "leave_group"){
//             leave_group(inpt, client_socket, client_uid);
//         }
       
//         else{
//             write(client_socket, "Invalid command", 16);
//         }
//     }
//     writeLog("***********pthread ended for client socket number " + to_string(client_socket));
//     close(client_socket);
// }

//i want to merge the handle connection file in the manage connection file///all present stuff in connection manage file remains same ...just instead of prinintg we will process those input commands with respective functions!!



void manage_connection(int client_socket_fd, int fg)
{   {
    lock_guard<mutex> lock(client_list_mutex);
    active_client_sockets.push_back(client_socket_fd);
     }
    string client_uid = "";
    string client_gid = "";
    int flag = fg;
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int read_count = read(client_socket_fd, buffer, 1024);
    // store new cmds and also append new commds to cmd which store all cmds
    // check if it is ping tht is $ from other tracker

    if (read_count <= 0)
    {
        writeLog("Error reading from socket");
        close(client_socket_fd);
        cout << "Closing connection from client\n";
        close(client_socket_fd);
        return;
    }
    string new_cmd(buffer, read_count);

    if (new_cmd == "$")
    {
        flag = 1; // indicating dont direclty forward cmds to other tracker

        other_tracker_read_socket_fd = client_socket_fd;

        std::cout << "Ping from other tracker received.\n";
        writeLog("Ping from other tracker received.");
        synchronize_prev_cmds(); // send this new cmd to other tracker if it is online
        connected_trackers = 1;
    }
    else
    {
        cout << ">> ";
        if (flag == 0)
        {
            if (client_uid == "")
                new_cmd += " -1";
            else
                new_cmd += " " + client_uid;
        }
        if (flag == 0 && connected_trackers)
        {
            write_to_other_tracker(new_cmd);
        }
        else if (flag == 0 && !connected_trackers)
        {
            cmds.push(new_cmd); // no connection so store command
        }
        writeLog("Received new_cmd: " + new_cmd);
        // std::cout << "Received new_cmd: " << new_cmd << endl;
        // string response = new_cmd + " - Message from Tracker";
        // strncpy(buffer, response.c_str(), sizeof(buffer));
        // buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
        // send(client_socket_fd, buffer, strlen(buffer), 0);
        string s, in = string(new_cmd);
        stringstream ss(in);
        vector<string> inpt;
        // cout<<"Processing command: "<<new_cmd<<endl;
        while (ss >> s)
        {
            inpt.push_back(s);
        }
        if (flag == 1)
        {    string c_uid=inpt.back();
            inpt.pop_back(); // to remove the client_uid part added above
            cout << "Processing command: " << new_cmd << " under tracker-trcaker thread" << endl;
            execute_cmd_tracker(client_socket_fd, inpt, 0,c_uid);
        }
        else if (flag == 0)
        {   inpt.pop_back();//to remove the client_uid part added above
            cout << "Processing command: " << new_cmd << " under client-trcaker thread" << endl;
            execute_cmd_client(client_socket_fd, inpt, client_uid, client_gid);
        }
        cout << "Processed command: " << new_cmd << endl;
    }
    while (true)
    {
        cout << ">> ";
        memset(buffer, 0, sizeof(buffer));
        int read_count = read(client_socket_fd, buffer, 1024);
        if (read_count <= 0)
        {
            writeLog("Error reading from socket");
            close(client_socket_fd);

            break;
        }
        string command(buffer, read_count);
        if (flag == 0)
        {
            if (client_uid == "")
                command += " -1";
            else
                command += " " + client_uid;
        }
        if (flag == 0 && connected_trackers)
        {
            write_to_other_tracker(command);
        }
        else if (flag == 0 && !connected_trackers)
        {
            cmds.push(command); // no connection so store command
        }
        // execute command on this tracker side;
        writeLog("Received command: " + command);
        // cout << "Received command: " << command << endl;
        // // now read and send back data as received+msg appneded

        // string response = command + " - Message from Tracker";
        // strncpy(buffer, response.c_str(), sizeof(buffer));
        // buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
        // if (flag == 0)
        //     send(client_socket_fd, buffer, strlen(buffer), 0);
        string s, in = string(command);
        stringstream ss(in);
        vector<string> inpt;

        while (ss >> s)
        {
            inpt.push_back(s);
        }
        if (flag == 1)
        {    string c_uid=inpt.back();
            inpt.pop_back();
            cout << "Processing command: " << command << " under tracker-trcaker thread" << endl;
            execute_cmd_tracker(client_socket_fd, inpt, 0,c_uid);
        }
        else if (flag == 0)
        {    inpt.pop_back();
            cout << "Processing command: " << command << " under client-trcaker thread" << endl;
            execute_cmd_client(client_socket_fd, inpt, client_uid, client_gid);
        }
        cout << "Processed command: " << command << endl;
    }
    cout << "Closing connection from client\n";
    close(client_socket_fd);
}


//it is used for late waking tracker!
//it is nothing but just executing the commands from previous tracker!!
void initial_manage_connection(int &client_socket, string new_cmd)
{
    cout << "Received from other tracker! -->" << new_cmd << endl;
    string s, in = string(new_cmd);
    stringstream ss(in);
    vector<string> inpt;

    while (ss >> s)
    {
        inpt.push_back(s);
    }
    string c_uid=inpt.back();
            inpt.pop_back();
    cout << "Processing command: " << new_cmd << " under tracker-trcaker thread" << endl;
    execute_cmd_tracker(client_socket, inpt, 1,c_uid);
}


vector<string> splitstring(string s, string delim) {
    vector<string> tokens;
    size_t pos = 0;

    while ((pos = s.find(delim)) != string::npos) {
        string token = s.substr(0, pos);
        if (!token.empty()) 
            tokens.push_back(token);
        s.erase(0, pos + delim.length());
    }

    if (!s.empty())
        tokens.push_back(s);

    return tokens;
}






