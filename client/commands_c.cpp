#include "client.h"

int list_groups(int sock)
{
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char reply[3 * SIZE];
    memset(reply, 0, sizeof(reply));
    read(sock, reply, 3 * SIZE);
    writeLog("list of groups reply: " + string(reply));

    vector<string> grps = splitString(string(reply), "$$");

    for (size_t i = 0; i < grps.size() - 1; i++)
    {
        cout << grps[i] << endl;
    }
    return 0;
}

int list_requests(int sock)
{
    writeLog("waiting for response");

    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char reply[3 * SIZE];
    memset(reply, 0, 3 * SIZE);
    read(sock, reply, 3 * SIZE);
    if (string(reply) == "**err**")
        return -1;
    if (string(reply) == "**er2**")
        return 1;
    writeLog("request list: " + string(reply));

    vector<string> requests = splitString(string(reply), "$$");
    writeLog("list request response size: " + to_string(requests.size()));
    for (size_t i = 0; i < requests.size() - 1; i++)
    {
        cout << requests[i] << endl;
    }
    return 0;
}

void accept_request(int sock)
{
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[96];
    read(sock, buf, 96);
    cout << buf << endl;
}

void leave_group(int sock)
{
    writeLog("waiting for response");
    char buf[96];
    read(sock, buf, 96);
    cout << buf << endl;
}

// we need to send the meta data !!
int uploadFile(vector<string> &inpt, int client_socket)
{
    if (inpt.size() != 3)
    {
        return 0;
    }
    string fileDetails = "";
    char *filepath = &inpt[2][0];
    writeLog("creating details for file-->" + inpt[2]);
    // extracting the file name!!
    string filename = splitString(string(filepath), "/").back();

    if (isUploaded[inpt[2]].find(filename) != isUploaded[inpt[2]].end())
    {
        cout << "File already uploaded in given group" << endl;
        if (send(client_socket, "error", 5, MSG_NOSIGNAL) == -1)
        {
            printf("Error: %s\n", strerror(errno));
            return -1;
        }
        return 0;
    }

    isUploaded[inpt[2]][filename] = true;
    fileToFilePath[filename] = string(filepath);

    string piecewiseHash = get_peicewise_Hash(filepath); // to check if chunk received correctly

    if (piecewiseHash == "$")
        return 0;                            // size 0or nothing to upload
    string filehash = getFileHash(filepath); // to check if file recevived correctly...first this will be check if mismatch then piece wise will be checked !!
    string filesize = to_string(getFileSize(filepath));

    fileDetails += string(filepath) + "$$";
    fileDetails += string(set_client_IP) + ":" + to_string(set_client_port) + "$$";
    fileDetails += filesize + "$$";
    fileDetails += filehash + "$$";
    fileDetails += piecewiseHash;
    // fileDetails have-->  Filepath,ip:port,filesize,filehash,piecewise_filehash

    // send metadata==> fileDeatils to the tracker where it updates its data !!
    writeLog("sending file details for upload: " + fileDetails);
    if (send(client_socket, &fileDetails[0], strlen(&fileDetails[0]), MSG_NOSIGNAL) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    char server_reply[10240] = {0};
    read(client_socket, server_reply, 10240);
    // received reply Uploaded
    cout << server_reply << endl;
    writeLog("server reply for send file: " + string(server_reply));

    // storing what all chuncks/blocks it has for tht file
    //  as it is uploader it has every block thus all 1
    setChunkVector(filename, 0, ceil(static_cast<double>(stoll(filesize)) / BLOCK_SIZE), true);
    writeLog("File uploaded at -->" + inpt[2]);
    return 0;
}

int downloadFile(vector<string> &inpt, int client_socket)
{
    write(client_socket, "Dummy", 5);
    char piece_wisee_hashh[524288] = {0};
    int r = read(client_socket, piece_wisee_hashh, 524288);
    if (r <= 0)
    {
        cout << "Error in reading socket 1!!\n";
        return 0;
    }
    if(string(piece_wisee_hashh) == "File not found"){
        cout << piece_wisee_hashh << endl;
        return 0;
    }
    vector<string> seeders_info = splitString(piece_wisee_hashh, "$$");
     write(client_socket, "test", 5);
     bzero(piece_wisee_hashh, 524288);
     r = read(client_socket, piece_wisee_hashh, 524288);
     if (r <= 0)
     {
         cout << "Error in reading socket 2!!\n";
         return 0;
     }
     file_block_hash = splitString(piece_wisee_hashh, "$$");
     writeLog("Calling peertopeer_algo  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
     download_from_peers(inpt, seeders_info);
     // After successful download and hash verification
    //write to tracker that download is complete
     return 0;
}

void list_files(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[1024];
    bzero(buf, 1024);
    read(sock, buf, 1024);
    vector<string> listOfFiles = splitString(string(buf), "$$");

    for(auto i: listOfFiles)
        cout << i << endl;
}
void show_downloads(){
    for(auto i: downloaded_files){
        cout << "[C] " << i.second << " " << i.first << endl;
    }
}


int execute_cmd(vector<string> &inpt, int sock)
{
    char server_reply[10240];
    bzero(server_reply, 10240);
    read(sock, server_reply, 10240);
    cout << server_reply << endl;
    writeLog("primary server response: " + string(server_reply));
     if(string(server_reply) == "TRACKER_EXIT") {
        cout << "Tracker is shutting down. Disconnecting..." << endl;
        close(sock);
        loggedIn = false;
        //connect to other tracker if fialed then exit from connection_to_tracker function itself
         int socket_fd;
    struct sockaddr_in address;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }
    int enable = 1;

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    connection_to_tracker(socket_fd, address,1);
        // Optionally: prompt user to connect to the other tracker and login again
        exit(0); // or return to main loop to reconnect
    }
    if (string(server_reply) == "Invalid argument count")
        return 0;
    if (inpt[0] == "login")
    {
        if (string(server_reply) == "Login Successful")
        {
            loggedIn = true;
            string peerAddress = set_client_IP + ":" + to_string(set_client_port);
            write(sock, &peerAddress[0], peerAddress.length());
        }
    }
    else if (inpt[0] == "logout")
    {
        loggedIn = false;
    }

    else if (inpt[0] == "list_groups")
    {
        return list_groups(sock);
    }
    else if (inpt[0] == "list_requests")
    {
        int t;
        if ((t = list_requests(sock)) < 0)
        {
            cout << "You are not the admin of this group\n";
        }
        else if (t > 0)
        {
            cout << "No pending requests\n";
        }
        else
            return 0;
    }
    else if (inpt[0] == "accept_request")
    {
        accept_request(sock);
    }
    else if (inpt[0] == "leave_group")
    {
        leave_group(sock);
    }
    else if (inpt[0] == "upload_file")
    {
        cout << "inside uploade file in execute_cmd function\n";
        if (string(server_reply) == "Error 1:")
        {
            cout << "Group doesn't exist" << endl;
            return 0;
        }
        else if (string(server_reply) == "Error 2:")
        {
            cout << "You are not a member of this group" << endl;
            return 0;
        }
        // means received "Uploading..." from tracker!!
        // now its waitning for use to send the hash values !!
        cout << "Calling uploadfile_function\n";
        return uploadFile(inpt, sock);
    }
    else if (inpt[0] == "download_file")
    {
        // have already read the first reply which verifies grps n all
        if (string(server_reply) == "Error 101:")
        {
            cout << "Group doesn't exist" << endl;
            return 0;
        }
        else if (string(server_reply) == "Error 102:")
        {
            cout << "You are not a member of this group" << endl;
            return 0;
        }
        else if (string(server_reply) == "Invalid argument count")
        {
            cout << "Invalid argument count\n";
            return 0;
        }
        // received reply as downloading ....so continue to actual downloading
        return downloadFile(inpt, sock);
    }
     else if(inpt[0] == "list_files"){
        list_files(sock);
    }
    else if(inpt[0] == "stop_share"){
        isUploaded[inpt[1]].erase(inpt[2]);
        //remove from the seeder list of that file in tracker
    }
    else if(inpt[0] == "show_downloads"){
        show_downloads();
    }

    return 0;
}