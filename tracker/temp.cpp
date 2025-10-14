#include "tracker.h"
void manage_connection(int client_socket_fd, int fg)
{
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
            if (client_uid == " ")
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
            if (client_uid == " ")
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
        string s, in = string(new_cmd);
        stringstream ss(in);
        vector<string> inpt;

        while (ss >> s)
        {
            inpt.push_back(s);
        }
        if (flag == 1)
        {    string c_uid=inpt.back();
            inpt.pop_back();
            cout << "Processing command: " << new_cmd << " under tracker-trcaker thread" << endl;
            execute_cmd_tracker(client_socket_fd, inpt, 0,c_uid);
        }
        else if (flag == 0)
        {    inpt.pop_back();
            cout << "Processing command: " << new_cmd << " under client-trcaker thread" << endl;
            execute_cmd_client(client_socket_fd, inpt, client_uid, client_gid);
        }
        cout << "Processed command: " << new_cmd << endl;
    }
    cout << "Closing connection from client\n";
    close(client_socket_fd);
}

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



// change synchronize....
// change execute_for_tracekr...where tracker shouldnot write back!!!

