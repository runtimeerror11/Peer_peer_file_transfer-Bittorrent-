#include "tracker.h"


int main(int argc, char *argv[])
{

    // create socket
    // bind socket
    // listen on socket
    // create thread to listen for exit command
    // while true
    //     accept connection
    //     create thread to handle connection
    // join all threads before exiting
    // return 0
    if (argc != 3)
    {
        std::cout << "Usage: ./tracker <tracker info file> <tracker number>\n";
        return 1;
    }
    initialize_args(argc, argv);

    int socket_fd;
    struct sockaddr_in address;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(1);
    }
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    address.sin_family = AF_INET;
    address.sin_port = htons(set_tracker_port);
    if (inet_pton(AF_INET, &set_tracker_IP[0], &address.sin_addr) <= 0)
    {
        cout << "Invalid address/ Address not supported\n";
        return -1;
    }
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }
    if (listen(socket_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(1);
    }

    pthread_t exit_detection_thread;
    if (pthread_create(&exit_detection_thread, NULL, check_if_exit, NULL) != 0)
    {
        perror("pthread");
        exit(1);
    }
    vector<thread> thread_vector;
    int addr_len = sizeof(address);
    clearLog();
    ping_first();
    // now wait to read all the prev cmds if the connection with other tracker is completeeeeed!!!....it will send $ as no more cmds
    if (other_tracker_write_socket_fd != -1)
    {
        cout << "going for syncing i.e. read\n";
        // sync cmds here itself
        int tracker_read_sock = -1;
        if ((tracker_read_sock = accept(socket_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0)
        {
            perror("Accept failed");
            writeLog("Error in accept");
        }
        cout << "Accepted read socket!\n";
        other_tracker_read_socket_fd = tracker_read_sock;
        if (other_tracker_read_socket_fd!=-1)
        {
            cout << "Syncing !!!\n";
            char buffer[1024];
            string all_cmds = "";
            while (true)
            {
                memset(buffer, 0, sizeof(buffer));
                int read_count = read(other_tracker_read_socket_fd, buffer, 1024);
                if (read_count <= 0)
                {
                    writeLog("Error reading from socket");
                    break;
                }
                string new_cmd(buffer, read_count);
                if (new_cmd == "$")
                {   cout<<"All prev cmds synced from other tracker\n";
                    writeLog("All prev cmds synced from other tracker");
                    thread_vector.push_back(thread(manage_connection, other_tracker_read_socket_fd, 1));
                    cout<<"Spawned thread to manage other tracker connection\n";
                    connected_trackers = 1;
                    break;
                }
                initial_manage_connection(other_tracker_read_socket_fd,new_cmd);
                send(tracker_read_sock, "Cmd received", 12, 0);
            }
        
        } // synchronize_prev_cmds();//if other tracker is online it will connect and sync the previous cmds
    }
    if (other_tracker_read_socket_fd != -1 && other_tracker_write_socket_fd != -1)
        cout << "BOTH trakcer connections are synced now\n";
    else
        cout << "Running as standalone tracker\n";
    // now keep accepting connections from clients and other tracker
    while (true)
    {
        // here in this while function i want to check if connection request if from tracker or client if from tracker  then keep other thread tht keeps on reading from data sent from toher trakcer and for now lets just print tht thing...later we need to execute tht cmds and update datastructures
        // if from client then spawn a thread to handle tht client as it is present below

        cout << "Waiting for connections on " << set_tracker_IP << ":" << set_tracker_port << "...\n";
        int client_socket_fd;
        if ((client_socket_fd = accept(socket_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0)
        {
            perror("Accept failed");
            writeLog("Error in accept");
        }
        // see if other_tracker_socket_fd is -1 if yes then this connection is from other tracker if not yet set
        // so keep on trying to connect to other tracker every 5 sec
        // if other_tracker_socket_fc is not -1 then connection has been establised all older commds are processed so no just keep sending the new cmds to other tracker

        writeLog("Connection Accepted");
        thread_vector.push_back(thread(manage_connection, client_socket_fd,0));
        cout << "Spawned thread for new connection. Total threads: " << thread_vector.size() << endl;
    }
    cout << "Out of main loop\n";
    return 0;
}

// input commands for code

// 4.1User and Group Management
// *  Users interact with the system through the following commands:
// *  create user <user id> <password> - Register a new user account
// * login <user id> <password> - Authenticate and start a session
// * create group <group id> - Create a new group (user becomes owner)
// * join group <group id> - Request to join an existing group
// * leave group <group id> - Leave a group you’re a member of
// * list groups - Display all available groups in the system
// * list requests <group id> - Show pending join requests (owner only)
// * accept request <group id> <user id> - Accept a join request (owner only)
// * logout - End current session and stop sharing files
// 4.2
// File Operations
// File sharing and downloading operations include:
// * upload file <group id> <file path> - Share a file with a group
// * list files <group id> - Show all files available in a group
// * download file <group id> <file name> <destination path> - Download a file from the
// group
// * show downloads - Display current download progress
// * stop share <group id> <file name> - Stop sharing a specific file
// 4.3
// System Execution
// To run the system components:
// ./tracker tracker info.txt tracker no - Start a tracker server
// ./client <IP>:<PORT> tracker info.txt - Start a client application
// * quit - Shutdown tracker (used within tracker console)