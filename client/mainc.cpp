// lets make simliar so tht trackers and clinet can communincate
#include "client.h"

int main(int argc, char *argv[])
{
    // input will be  <IP>:<PORT> tracker info.txt - Start a client application

    // missing thread to listen for connecting to tracker
    if (argc != 3)
    {
        cout << "Usage: ./client <client info file> <client number>\n";
        return 1;
    }
    process_args(argc, argv);
    pthread_t client_As_serverThread;
    // client_as_server();
    if(pthread_create(&client_As_serverThread, NULL, client_as_server, NULL) == -1){
        perror("pthread"); 
        exit(EXIT_FAILURE); 
    }
    int socket_fd;
    struct sockaddr_in address;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }
    int enable = 1;

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    // address.sin_family = AF_INET;
    // address.sin_port = htons(set_client_port);
    // if (inet_pton(AF_INET, &set_client_IP[0], &address.sin_addr) <= 0)
    // {
    //     cout << "Invalid address/ Address not supported\n";
    //     return -1;
    // }
    // if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    // {
    //     perror("Bind failed");
    //     exit(1);
    // }

    // updating for tracker connection
        clearLog();

    connection_to_tracker(socket_fd, address);
    
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        cout << ">> ";
        string input, s;
        getline(cin, input);
        // ssize_t sent = send(socket_fd, input.c_str(), input.size(), 0);
        // if (sent == -1)
        // {
        //     perror("Send failed");
        //     cout << "Connection lost while sending.\n";
        //     break;
        // }
        // cout << "Command sent\n";
        // writeLog("Command sent: " + input);
        // int valread = read(socket_fd, buffer, 1024);
        // if (valread <= 0)
        // {
        //     cout << "Server closed connection or error occurred\n";
        //     break;
        // }
        // string command(buffer, valread);
        // cout << "Response from server: " << command << endl;
        if (input.length() < 1)
            continue;

        stringstream ss(input);
        vector<string> inpt;
        while (ss >> s)
        {
            inpt.push_back(s);
        }

        if (inpt[0] == "login" && loggedIn)
        {
            cout << "You already have one active session" << endl;
            continue;
        }
        if (inpt[0] == "create_user" && loggedIn)
        {
            cout << "Logout first to create a new user" << endl;
            continue;
        }
        if (inpt[0] != "login" && inpt[0] != "create_user" && !loggedIn)
        {
            cout << "Please login / create an account" << endl;
            continue;
        }
         //send cmd to tracker and process the further working using exectue_cmd function
        if(inpt[0]=="upload_file"){
            if(!check_if_path_is_correct(inpt[2])){
                cout << "Incorrect file path\n";
                continue;
            }
        }
        if(inpt[0]=="download_file"){
            // download_file <group id> <file name> <destination path> 
            if(downloaded_files.find(inpt[2])!=downloaded_files.end()){
                //file name exists 
                if(downloaded_files[inpt[2]]==inpt[1]){
                    cout << "File already downloaded\n";
                    continue;
                }
            }
            if(!check_if_path_is_correct(inpt[3])){
                cout << "Incorrect file path\n";
                continue;
            }

        }
        
         if (send(socket_fd, &input[0], strlen(&input[0]), MSG_NOSIGNAL) == -1)
        {
            printf("Error: %s\n", strerror(errno));
            return -1;
        }
        writeLog("sent to server: " + inpt[0]);

        execute_cmd(inpt, socket_fd);

        if (input == "exit")
        {
            cout << "Exiting client\n";
            break;
        }
    }
    cout << "Closing connection from client\n";
    close(socket_fd);
    return 0;
}


// /home/suparshwa/Documents/aos/A1/test1.txt