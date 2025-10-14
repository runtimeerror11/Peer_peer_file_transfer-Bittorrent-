#include "tracker.h"

// functions to write are
//
//  4.1User and Group Management
//  *  Users interact with the system through the following commands:
//  *  create user <user id> <password> - Register a new user account
//  * login <user id> <password> - Authenticate and start a session
//  * create group <group id> - Create a new group (user becomes owner)
//  * join group <group id> - Request to join an existing group
//  * leave group <group id> - Leave a group you’re a member of
//  * list groups - Display all available groups in the system
//  * list requests <group id> - Show pending join requests (owner only)
//  * accept request <group id> <user id> - Accept a join request (owner only)
//  * logout - End current session and stop sharing files

void execute_cmd_tracker(int &clinet_socket, vector<string> inpt, int flag /*if flag==1 means its from sync cmds ...so it wants something back to knwo tht its write was successful*/, string c_id)
{
    // now modify this function to execute the cmds received from other tracker similar to execute_cmd_client but without writing back to socket,except when flag is 1 and also few functions working changes...like as this tracker connection with other tracker is acting as client for it ...once it does login it will not allow to login again which will login comds of  toher clinets which this tracker has executed and its forwarding it to tht tracker ...so tht it xecutes those this and update those changes!!...but for tracker it acts kike same client is trying to login multipple times as per execute trackers functionloty it will block tht request....similaly there are other few changes please update them too!!!
    // cpde please
    //  This function executes commands received from another tracker.
    //  It does not write back to the socket unless flag == 1.
    //  Some logic is adjusted for tracker-to-tracker sync.

    string client_uid = c_id;
    string client_gid = "";

    if (c_id != "-1" && userToGroup.end() != userToGroup.find(client_uid))
        client_gid = userToGroup[client_uid];

    if (inpt[0] == "create_user")
    {
        if (inpt.size() != 3)
        {
            if (flag == 1)
                write(clinet_socket, "Invalid argument count", 22);
        }
        else
        {
            if (createUser(inpt) < 0)
            {
                if (flag == 1)
                    write(clinet_socket, "User exists", 11);
            }
            else
            {
                if (flag == 1)
                    write(clinet_socket, "Account created", 15);
            }
        }
    }
    else if (inpt[0] == "login")
    {
        // For tracker sync, do not allow multiple logins for same user
        if (inpt.size() != 3)
        {
            if (flag == 1)
                write(clinet_socket, "Invalid argument count", 22);
        }
        else
        {
            int r;
            if ((r = validateLogin(inpt)) < 0)
            {
                if (flag == 1)
                    write(clinet_socket, "Username/password incorrect", 28);
            }
            else if (r > 0)
            {
                // Block multiple logins for tracker sync
                if (flag == 1)
                    write(clinet_socket, "Already logged in", 17);
            }
            else
            {
                if (flag == 1)
                    write(clinet_socket, "Login Successful", 16);
                client_uid = inpt[1];
                // For tracker sync, skip reading peer address
            }
        }
    }
    else if (inpt[0] == "logout")
    {
        if (inpt.size() != 1)
        {
            if (flag == 1)
                write(clinet_socket, "Invalid argument count", 22);
        }
        else
        {
            is_logged_in[c_id] = false;
            if (flag == 1)
                write(clinet_socket, "Logout Successful", 17);
            writeLog("logout success\n");
        }
    }
    else if (inpt[0] == "create_group")
    {
        if (inpt.size() != 2)
        {
            if (flag == 1)
                write(clinet_socket, "Invalid argument count", 22);
        }
        else
        {
            string uid = (inpt.size() > 2) ? inpt[2] : ""; // If user id is passed
            if (uid.empty())
                uid = client_uid;
            if (create_group(inpt, clinet_socket, uid) >= 0)
            {
                client_gid = inpt[1];
                userToGroup[uid] = client_gid; // Update userToGroup mapping
                if (flag == 1)
                    write(clinet_socket, "Group created", 13);
            }
            else
            {
                if (flag == 1)
                    write(clinet_socket, "Group exists", 12);
            }
        }
    }
    else if (inpt[0] == "list_groups")
    {
        if (flag == 1)
            list_groups(inpt, clinet_socket);
        // Otherwise, do nothing
    }
    else if (inpt[0] == "join_group")
    {
        string uid = (inpt.size() > 2) ? inpt[2] : "";
        if (uid.empty())
            uid = client_uid;
        if (flag == 1)
            join_group(inpt, clinet_socket, uid);
        else
            join_group(inpt, -1, uid); // -1 disables socket write
    }
    else if (inpt[0] == "list_requests")
    {
        string uid = (inpt.size() > 2) ? inpt[2] : "";
        if (uid.empty())
            uid = client_uid;
        if (flag == 1)
            list_requests(inpt, clinet_socket, uid);
    }
    else if (inpt[0] == "accept_request")
    {
        string uid = (inpt.size() > 3) ? inpt[3] : "";
        if (uid.empty())
            uid = client_uid;
        if (flag == 1)
            accept_request(inpt, clinet_socket, uid);
        else
            accept_request(inpt, -1, uid);
    }
    else if (inpt[0] == "leave_group")
    {
        string uid = (inpt.size() > 2) ? inpt[2] : "";
        if (uid.empty())
            uid = client_uid;
        if (flag == 1)
            leave_group(inpt, clinet_socket, uid);
        else
            leave_group(inpt, -1, uid);
    }
    else if (inpt[0] == "upload_file")
    {
        // inpt: upload_file <group_id> <file_name> <file_size> <piecewise_hash> <user_id>
        if (inpt.size() < 6)
        {
            writeLog("Invalid upload_file sync command received");
            if (flag == 1)
                write(clinet_socket, "Uploaded", 8);
            return;
        }
        string group_id = inpt[1];
        string file_name = inpt[2];
        string file_size = inpt[3];
        string piecewise_hash = inpt[4];
        string user_id = inpt[5];

        piecewiseHash[file_name] = piecewise_hash;
        seederList[group_id][file_name].insert(user_id);
        fileSize[file_name] = file_size;

        writeLog("Synchronized upload_file: " + file_name + " by " + user_id);
        if (flag == 1)
            write(clinet_socket, "Uploaded", 8);
        // Do NOT forward again!
    }
    else if (inpt[0] == "download_file")
    {
        if (inpt.size() == 4)
        {
            if (flag == 1)
                write(clinet_socket, "Uploaded", 8);
        }
        else if (inpt.size() == 3)
        {
            // Tracker-to-tracker sync: c_id should be a valid user id
            if (is_logged_in.find(c_id) != is_logged_in.end())
            {
                seederList[inpt[1]][inpt[2]].insert(c_id);
                writeLog("Synchronized download_file: " + inpt[2] + " by " + c_id);
            }
            else
            {
                writeLog("Invalid c_id in tracker-to-tracker download_file sync: " + c_id);
            }
            if (flag == 1)
                write(clinet_socket, "Uploaded", 8);
        }
        else
        {
            if (flag == 1)
                write(clinet_socket, "Uploaded", 8);
            writeLog("Invalid download_file command received");
        }
    }
    else if (inpt[0] == "list_files")
    {
        if (flag == 1)
            write(clinet_socket, "Loading...", 10);
    }
    else if (inpt[0] == "show_downloads")
    {
        if (flag == 1)
            write(clinet_socket, "Loading...", 10);
    }
    else if (inpt[0] == "stop_share")
    {
        if (flag == 1)
        {
            stop_share(inpt, clinet_socket, c_id);
            return;
        }
        if (inpt.size() != 3)
        {
            return;
        }
        if (grpAdmins.find(inpt[1]) == grpAdmins.end())
        {
            return;
        }
        else if (seederList[inpt[1]].find(inpt[2]) == seederList[inpt[1]].end())
        {
            return;
        }
        else
        {
            seederList[inpt[1]][inpt[2]].erase(client_uid);
            if (seederList[inpt[1]][inpt[2]].size() == 0)
            {
                seederList[inpt[1]].erase(inpt[2]);
            }
            writeLog("Synchronized stop_share: " + inpt[2] + " by " + client_uid);
        }
    }

    else
    {
        if (flag == 1)
            write(clinet_socket, "Invalid command", 16);
    }
}
void execute_cmd_client(int &client_socket, vector<string> inpt, string &client_uid, string &client_gid)
{ // cout<<"in execute cmd client\n";
    if (inpt[0] == "create_user")
    { // cout<<"input array size: "<<inpt.size()<<endl;
        if (inpt.size() != 3)
        {
            write(client_socket, "Invalid argument count", 22);
        }
        else
        {
            if (createUser(inpt) < 0)
            {
                write(client_socket, "User exists", 11);
            }
            else
            {
                write(client_socket, "Account created", 15);
            }
        }
    }
    else if (inpt[0] == "login")
    {
        if (inpt.size() != 3)
        {
            write(client_socket, "Invalid argument count", 22);
        }
        else
        {
            int r;
            if ((r = validateLogin(inpt)) < 0)
            {
                write(client_socket, "user doesnt exists or Username/password incorrect", 52);
            }
            else if (r > 0)
            {
                write(client_socket, "You already have one active session", 35);
            }
            else
            {
                write(client_socket, "Login Successful", 16);
                client_uid = inpt[1];
                char buf[96];
                read(client_socket, buf, 96);
                string peerAddress = string(buf);
                unameToPort[client_uid] = peerAddress;
            }
        }
    }
    else if (inpt[0] == "logout")
    {
        cout << "in logout\n";
        is_logged_in[client_uid] = false;
        write(client_socket, "Logout Successful", 17);
        writeLog("logout sucess\n");
    }

    else if (inpt[0] == "create_group")
    {
        if (create_group(inpt, client_socket, client_uid) >= 0)
        {
            client_gid = inpt[1];
            write(client_socket, "Group created", 13);
        }
        else
        {
            write(client_socket, "Group exists", 12);
        }
    }
    else if (inpt[0] == "list_groups")
    {
        list_groups(inpt, client_socket);
    }
    else if (inpt[0] == "join_group")
    {
        join_group(inpt, client_socket, client_uid);
    }
    else if (inpt[0] == "list_requests")
    {
        list_requests(inpt, client_socket, client_uid);
    }
    else if (inpt[0] == "accept_request")
    {
        accept_request(inpt, client_socket, client_uid);
    }
    else if (inpt[0] == "leave_group")
    {
        leave_group(inpt, client_socket, client_uid);
    }
    else if (inpt[0] == "upload_file")
    {
        uploadFile(inpt, client_socket, client_uid);
    }
    else if (inpt[0] == "download_file")
    {
        downloadFile(inpt, client_socket, client_uid, client_gid);
    }
    else if (inpt[0] == "list_files")
    {
        list_files(inpt, client_socket);
    }
    else if (inpt[0] == "show_downloads")
    {
        write(client_socket, "Loading...", 10);
    }
    else if (inpt[0] == "stop_share")
    {
        stop_share(inpt, client_socket, client_uid);
    }
    else
    {
        write(client_socket, "Invalid command", 16);
    }
}

int createUser(vector<string> inpt)
{
    string user_id = inpt[1];
    string passwd = inpt[2];

    if (login_creds.find(user_id) == login_creds.end())
    {
        login_creds.insert({user_id, passwd});
    }
    else
    {
        return -1;
    }
    return 0;
}

int validateLogin(vector<string> inpt)
{
    string user_id = inpt[1];
    string passwd = inpt[2];

    if (login_creds.find(user_id) == login_creds.end() || login_creds[user_id] != passwd)
    {
        return -1;
    }

    if (is_logged_in.find(user_id) == is_logged_in.end())
    {
        is_logged_in.insert({user_id, true});
    }
    else
    {
        if (is_logged_in[user_id])
        {
            return 1;
        }
        else
        {
            is_logged_in[user_id] = true;
        }
    }
    return 0;
}

int create_group(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - [create_group gid]
    if (inpt.size() != 2)
    {
        write(client_socket, "Invalid argument count", 22);
        return -1;
    }
    for (auto i : allGroups)
    {
        if (i == inpt[1])
            return -1;
    }
    grpAdmins.insert({inpt[1], client_uid});
    allGroups.push_back(inpt[1]);
    groupMembers[inpt[1]].insert(client_uid);
    return 0;
}

void list_groups(vector<string> inpt, int client_socket)
{
    // inpt - [list_groups];
    if (inpt.size() != 1)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "All groups:", 11);

    char dum[5];
    read(client_socket, dum, 5);

    if (allGroups.size() == 0)
    {
        write(client_socket, "No groups found$$", 18);
        return;
    }

    string reply = "";
    for (size_t i = 0; i < allGroups.size(); i++)
    {
        reply += allGroups[i] + "$$";
    }
    write(client_socket, &reply[0], reply.length());
}

void join_group(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - [join_group gid]
    if (inpt.size() != 2)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    writeLog("join_group function ..");

    if (grpAdmins.find(inpt[1]) == grpAdmins.end())
    {
        write(client_socket, "Invalid group ID.", 18);
    }
    else if (groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end())
    {
        grpPendngRequests[inpt[1]].insert(client_uid);
        write(client_socket, "Group request sent", 18);
    }
    else
    {
        write(client_socket, "You are already in this group", 30);
    }
}

void list_requests(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - [list_requests groupid]
    if (inpt.size() != 2)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching group requests...", 27);

    char dum[5];
    read(client_socket, dum, 5);

    writeLog("hereeee");
    if (grpAdmins.find(inpt[1]) == grpAdmins.end() || grpAdmins[inpt[1]] != client_uid)
    {
        writeLog("iffff");
        write(client_socket, "**err**", 7);
    }
    else if (grpPendngRequests[inpt[1]].size() == 0)
    {
        write(client_socket, "**er2**", 7);
    }
    else
    {
        string reply = "";
        writeLog("pending request size: " + to_string(grpPendngRequests[inpt[1]].size()));
        for (auto i = grpPendngRequests[inpt[1]].begin(); i != grpPendngRequests[inpt[1]].end(); i++)
        {
            reply += string(*i) + "$$";
        }
        write(client_socket, &reply[0], reply.length());
        writeLog("reply :" + reply);
    }
}

void accept_request(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - [accept_request groupid user_id]
    if (inpt.size() != 3)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Accepting request...", 21);

    char dum[5];
    read(client_socket, dum, 5);

    if (grpAdmins.find(inpt[1]) == grpAdmins.end())
    {
        writeLog("inside accept_request if");
        write(client_socket, "Invalid group ID.", 18);
    }
    else if (grpAdmins.find(inpt[1])->second == client_uid)
    {
        writeLog("inside accept_request else if with pending list:");
        for (auto i : grpPendngRequests[inpt[1]])
        {
            writeLog(i);
        }
        grpPendngRequests[inpt[1]].erase(inpt[2]);
        groupMembers[inpt[1]].insert(inpt[2]);
        write(client_socket, "Request accepted.", 18);
    }
    else
    {
        writeLog("inside accept_request else");
        // cout << grpAdmins.find(inpt[1])->second << " " << client_uid <<  endl;
        write(client_socket, "You are not the admin of this group", 35);
    }
}

void leave_group(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - [leave_group groupid]
    if (inpt.size() != 2)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Leaving group...", 17);

    if (grpAdmins.find(inpt[1]) == grpAdmins.end())
    {
        write(client_socket, "Invalid group ID.", 18);
    }
    else if (groupMembers[inpt[1]].find(client_uid) != groupMembers[inpt[1]].end())
    {
        if (grpAdmins[inpt[1]] == client_uid)
        {
            write(client_socket, "You are the admin of this group, you cant leave!", 48);
        }
        else
        {
            groupMembers[inpt[1]].erase(client_uid);
            // also remove it from seeders
            for (auto &it : seederList[inpt[1]])
            {
                it.second.erase(client_uid);
            }
            userToGroup[client_uid] = "";

            write(client_socket, "Group left succesfully", 23);
        }
    }
    else
    {
        write(client_socket, "You are not in this group", 25);
    }
}

// * upload_file <group id> <file path> - Share a file with a group
// * list_files <group id> - Show all files available in a group
// * download_file <group id> <file name> <destination path> - Download a file from the group
// * show_downloads - Display current download progress
// * stop_share <group id> <file name> - Stop sharing a specific file

void uploadFile(vector<string> inpt, int client_socket, string client_uid)
{
    // want -->  inpt - upload_file​ <file_path> <group_id​>
    // have -->  inpt -upload_file <group id> <file path> thus swap
    string temp = inpt[1]; // stored gid in temp
    inpt[1] = inpt[2];     // stored file_path ate 1st index
    inpt[2] = temp;        // stored gid at 2nd index
    if (inpt.size() != 3)
    {
        write(client_socket, "Invalid argument count", 22);
    }
    else if (groupMembers.find(inpt[2]) == groupMembers.end())
    {
        // grp doent exists!
        write(client_socket, "Error 1:", 8);
    }
    else if (groupMembers[inpt[2]].find(client_uid) == groupMembers[inpt[2]].end())
    {
        // not member of the grp
        write(client_socket, "Error 2:", 8);
    }
    else
    {
        char file_metadata[524288] = {0};
        write(client_socket, "Send metadata", 13);
        writeLog("uploading");

        // waiting to receive the file hashes ..sizes..etc...i.e file meta data
        if (read(client_socket, file_metadata, 524288))
        {
            // read filemetadata
            if (string(file_metadata) == "error")
                return; // file already uploaded

            vector<string> fdet = splitstring(string(file_metadata), "$$");

            // fdet = [filepath, peer address, file size, file hash, piecewise hash]
            // here piecewise-hash is also split now !!!

            // extracting last part
            string filename = splitstring(string(fdet[0]), "/").back();
            string piecewiseHash_of_file = "";
            for (size_t i = 4; i < fdet.size(); i++)
            {
                piecewiseHash_of_file += fdet[i];
                if (i != fdet.size() - 1)
                    piecewiseHash_of_file += "$$";
            }
            // piecewisehash stored ...which will be given to the downloaders ...
            // so tht they cna verify the data received from other clients seeders/owners directly!!
            piecewiseHash[filename] = piecewiseHash_of_file;
            // inpt - upload_file​ <file_path> <group_id​>
            //             0           1         2
            if (seederList[inpt[2]].find(filename) != seederList[inpt[2]].end())
            {
                seederList[inpt[2]][filename].insert(client_uid);
            }
            else
            {
                seederList[inpt[2]].insert({filename, {client_uid}});
            }
            fileSize[filename] = fdet[2];

            write(client_socket, "Uploaded", 8);

            // After updating piecewiseHash, seederList, fileSize, and sending "Uploaded"
            string sync_cmd = "upload_file " + inpt[2] + " " + filename + " " + fdet[2] + " " + piecewiseHash_of_file + " " + client_uid;
            if (other_tracker_write_socket_fd != -1)
            {
                write_to_other_tracker(sync_cmd);
            }
            else
            {
                cmds.push(sync_cmd); // queue for later sync
            }
        }
    }
}

void downloadFile(vector<string> inpt, int client_socket, string client_uid, string client_gid)
{
    // we obtain the gpid fromtthe cmd itself
    // check if uid is part of tht grp

    // inpt-->  download_file <group id> <file name> <destination path>
    if (inpt.size() != 4)
    {
        write(client_socket, "Invalid argument count", 22);
    }
    else if (groupMembers.find(inpt[1]) == groupMembers.end())
    {
        // grp doent exists!
        write(client_socket, "Error 1:", 8);
    }
    else if (groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end())
    {
        // not member of the grp
        write(client_socket, "Error 2:", 8);
    }
    else
    {
        // dummy write
        write(client_socket, "Downloading...", 13);
        char file_metadata[524288] = {0};
        if (read(client_socket, file_metadata, 524288))
        {
            // dummy read to know msg received;
            //   seederList; // {map of==> (groupid -> {map of==>(filenames -> peer address)})}

            string reply = "";
            if (seederList[inpt[1]].find(inpt[2]) != seederList[inpt[1]].end())
            {
                for (auto i : seederList[inpt[1]][inpt[2]])
                {
                    if (is_logged_in[i])
                    {
                        reply += unameToPort[i] + "$$";
                    }
                }
                reply += fileSize[inpt[2]];
                writeLog("seeder list: " + reply);
                write(client_socket, &reply[0], reply.length());

                char dum[5];
                read(client_socket, dum, 5);

                write(client_socket, &piecewiseHash[inpt[2]][0], piecewiseHash[inpt[2]].length());

                seederList[inpt[1]][inpt[2]].insert(client_uid);
                string sync_cmd = "download_file " + inpt[1] + " " + inpt[2] + " " + client_uid;
                if (other_tracker_write_socket_fd != -1)
                {
                    write_to_other_tracker(sync_cmd);
                }
                else
                {
                    cmds.push(sync_cmd); // queue for later sync
                }
            }
            else
            {
                write(client_socket, "File not found", 14);
            }
        }
    }
}

void list_files(vector<string> inpt, int client_socket)
{
    // inpt - list_files​ <group_id>
    if (inpt.size() != 2)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching files...", 17);

    char dum[5];
    read(client_socket, dum, 5);
    writeLog("dum read");

    if (grpAdmins.find(inpt[1]) == grpAdmins.end())
    {
        write(client_socket, "Invalid group ID.", 19);
    }
    else if (seederList[inpt[1]].size() == 0)
    {
        write(client_socket, "No files found.", 15);
    }
    else
    {
        writeLog("in else of list files");

        string reply = "";

        for (auto i : seederList[inpt[1]])
        {
            reply += i.first + "$$";
        }
        reply = reply.substr(0, reply.length() - 2);
        writeLog("list of files reply:" + reply);

        write(client_socket, &reply[0], reply.length());
    }
}

void stop_share(vector<string> inpt, int client_socket, string client_uid)
{
    // inpt - stop_share ​<group_id> <file_name>
    if (inpt.size() != 3)
    {
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    if (grpAdmins.find(inpt[1]) == grpAdmins.end())
    {
        write(client_socket, "Invalid group ID.", 19);
    }
    else if (seederList[inpt[1]].find(inpt[2]) == seederList[inpt[1]].end())
    {
        write(client_socket, "File not yet shared in the group", 32);
    }
    else
    {
        seederList[inpt[1]][inpt[2]].erase(client_uid);
        if (seederList[inpt[1]][inpt[2]].size() == 0)
        {
            seederList[inpt[1]].erase(inpt[2]);
        }
        write(client_socket, "Stopped sharing the file", 25);
    }
}
