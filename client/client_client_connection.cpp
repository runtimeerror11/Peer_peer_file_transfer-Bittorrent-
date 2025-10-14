#include "client.h"
#include "threadss.h"




std::mutex mtx; // mutex for thread-safe access

// Helper struct for priority queue
struct BlockInfo
{
    long long index;
    size_t numSeeders;

    bool operator<(const BlockInfo &other) const
    {
        return numSeeders > other.numSeeders; // smaller numSeeders = higher priority
    }
};

// ==========================
// Helper: calculate SHA1 hash of a block
string get_block_hash(const string &filepath, int blockNum)
{
    ifstream in(filepath, ios::binary);
    if (!in.is_open()) return "";

    in.seekg(blockNum * BLOCK_SIZE, ios::beg);
    vector<char> buffer(BLOCK_SIZE);
    in.read(buffer.data(), BLOCK_SIZE);
    buffer.resize(in.gcount()); // last block may be smaller
    in.close();

    return calculate_sha1(buffer); // use your existing helper
}

// ==========================
// reply from server side client
void reply_client_to_client_socket(int client_socket_fd)
{
    char inptline[1024] = {0};
    if (read(client_socket_fd, inptline, 1024) <= 0)
    {
        close(client_socket_fd);
        return;
    }

    vector<string> inpt = splitString(string(inptline), "$$");
    if (inpt[0] == "get_chunk_vector")
    {
        string filename = inpt[1];
        vector<int> chnkvec = fileChunkInfo[filename];
        string tmp = "";
        for (int i : chnkvec)
            tmp += to_string(i);
        char *reply = &tmp[0];
        write(client_socket_fd, reply, strlen(reply));
    }
    else if (inpt[0] == "get_chunk")
    {
        string filepath = fileToFilePath[inpt[1]];
        long long chunkNum = stoll(inpt[2]);
        sendChunk(&filepath[0], chunkNum, client_socket_fd);
    }
}

// ==========================
// client acting as server thread
void *client_as_server(void *arg)
{
    int socket_fd;
    struct sockaddr_in address;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return NULL;
    }
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    address.sin_family = AF_INET;
    address.sin_port = htons(set_client_port);
    inet_pton(AF_INET, &set_client_IP[0], &address.sin_addr);

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    if (listen(socket_fd, 6) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int addrlen = sizeof(address);
    vector<thread> vThread;

    while (true)
    {
        int client_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0)
        {
            perror("Accept error");
            continue;
        }

        vThread.push_back(thread(reply_client_to_client_socket, client_socket));
    }

    for (auto &t : vThread)
        if (t.joinable()) t.join();

    close(socket_fd);
}

// ==========================
// request to peer client
int request_client_to_client_socket(int flag, string ip_port, string filename)
{
    vector<string> split_ip = splitString(ip_port, ":");
    string ip = split_ip[0];
    string port = split_ip[1];

    int peersock = socket(AF_INET, SOCK_STREAM, 0);
    if (peersock < 0)
    {
        perror("Socket creation error");
        return 1;
    }

    struct sockaddr_in peer_serv_addr{};
    peer_serv_addr.sin_family = AF_INET;
    peer_serv_addr.sin_port = htons(stoi(port));
    inet_pton(AF_INET, ip.c_str(), &peer_serv_addr.sin_addr);

    struct timeval timeout{};
    timeout.tv_sec = 3;
    setsockopt(peersock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    if (connect(peersock, (struct sockaddr *)&peer_serv_addr, sizeof(peer_serv_addr)) < 0)
    {
        perror("Peer Connection Error");
        close(peersock);
        return 1;
    }

    if (flag == 1) // get chunk vector
    {
        string command = "get_chunk_vector$$" + filename;
        send(peersock, command.c_str(), command.size(), MSG_NOSIGNAL);

        char server_reply[10240] = {0};
        ssize_t n = read(peersock, server_reply, sizeof(server_reply) - 1);
        if (n > 0)
        {
            server_reply[n] = '\0';
            for (size_t i = 0; i < blockwise_seeders.size(); i++)
                if (server_reply[i] == '1') blockwise_seeders[i].push_back(ip_port);
        }

        close(peersock);
        return 0;
    }
    else if (flag == 2) // get chunk
    {
        vector<string> cmdtokens = splitString(filename, "$$");
        string despath = cmdtokens[3];
        long long chunkNum = stoll(cmdtokens[2]);
        writeChunk(peersock, chunkNum, &despath[0]);
        close(peersock);
        return 0;
    }

    return 1;
}

// ==========================
// write chunk to file
int writeChunk(int peersock, long long chunkNum, char *filepath)
{
    int tot = 0;
    char buffer[BLOCK_SIZE];
    string content = "";

    while (tot < BLOCK_SIZE)
    {
        int n = read(peersock, buffer, BLOCK_SIZE - 1);
        if (n <= 0) break;
        buffer[n] = 0;

        fstream outfile(filepath, ios::in | ios::out | ios::binary);
        outfile.seekp(chunkNum * BLOCK_SIZE + tot, ios::beg);
        outfile.write(buffer, n);
        outfile.close();

        content.append(buffer, n);
        tot += n;
    }

    string hash = "";
    hash_of_string(content, hash);
    hash.pop_back();
    hash.pop_back();
    if (hash != file_block_hash[chunkNum]) isCorruptedFile = true;

    string filename = splitString(string(filepath), "/").back();
    setChunkVector(filename, chunkNum, chunkNum, false);

    return 0;
}

// ==========================
// send chunk to client
void sendChunk(char *filepath, int chunkNum, int client_socket)
{
    ifstream fp(filepath, ios::binary);
    fp.seekg(chunkNum * BLOCK_SIZE, ios::beg);

    char buffer[BLOCK_SIZE] = {0};
    fp.read(buffer, BLOCK_SIZE);
    int count = fp.gcount();

    send(client_socket, buffer, count, 0);
    fp.close();
}

// ==========================
// download chunk with retry & verification
bool downloadChunkWithRetry(int chunkNum, const string &filename,
                            const vector<string> &peersForChunk,
                            const string &destPath,
                            int maxRetries = 3)
{
    if (peersForChunk.empty()) return false;
    int attempt = 0, totalPeers = peersForChunk.size();

    while (attempt < maxRetries)
    {
        string peer = peersForChunk[attempt % totalPeers];
        string command = "get_chunk$$" + filename + "$$" + to_string(chunkNum) + "$$" + destPath;

        int ret = request_client_to_client_socket(2, peer, command);
        if (ret != 0) { attempt++; continue; }

        string actualHash = get_block_hash(destPath, chunkNum);
        if (actualHash == file_block_hash[chunkNum])
        {
            lock_guard<mutex> lock(mtx);
            setChunkVector(filename, chunkNum, chunkNum, false);
            return true;
        }
        attempt++;
    }
    return false;
}

// ==========================
// main download_from_peers function
// void download_from_peers(vector<string> &inpt, vector<string> &seeder_info)
// {
//     long long curr_file_size = stoll(seeder_info.back());
//     long long num_of_blocks = ceil((double)curr_file_size / BLOCK_SIZE);
//     seeder_info.pop_back();

//     blockwise_seeders = vector<vector<string>>(num_of_blocks);

//     // query seeders
//     vector<thread> query_threads;
//     for (auto &info : seeder_info)
//         query_threads.push_back(thread(request_client_to_client_socket, 1, info, inpt[2]));
//     for (auto &t : query_threads)
//         if (t.joinable()) t.join();

//     // check availability
//     for (auto &b : blockwise_seeders)
//         if (b.empty()) { cout << "All parts not available"; return; }

//     string des_path = inpt[3] + "/" + inpt[2];
//     fstream outFile(des_path, ios::out | ios::binary);
//     string empty(curr_file_size, '\0');
//     outFile.write(empty.c_str(), empty.size());
//     outFile.close();

//     fileChunkInfo[inpt[2]].resize(num_of_blocks, 0);
//     isCorruptedFile = false;

//     ThreadPool pool(4);
//     vector<future<bool>> futures;

//     for (int i = 0; i < num_of_blocks; i++)
//     {
//         int blockNum = i;
//         futures.push_back(pool.enqueue([=]() {
//             return downloadChunkWithRetry(blockNum, inpt[2], blockwise_seeders[blockNum], des_path);
//         }));
//     }

//     bool allSuccess = true;
//     for (auto &f : futures)
//         if (!f.get()) allSuccess = false;

//     if (!allSuccess) cout << "Download failed: some chunks corrupted.\n";
//     else cout << "Download completed. No corruption detected.\n";

//     downloaded_files[inpt[2]] = inpt[1];
//     fileToFilePath[inpt[2]] = des_path;
// }
void download_from_peers(vector<string> &inpt, vector<string> &seeder_info)
{
    // inpt --> [download_file    <group id>    <file name>    <destination path>]
    // seeder_info -->   (ip:ports),(ip:port),....,filesize;
    long long curr_file_size = stoll(seeder_info.back());
    long long num_of_blocks = ceil(static_cast<double>(curr_file_size) / BLOCK_SIZE);
    seeder_info.pop_back();
    cout << "Number of seeders=" << seeder_info.size() << endl;
    vector<thread> query_threads;
    blockwise_seeders.clear();
    blockwise_seeders = vector<vector<string>>(num_of_blocks, vector<string>());
    writeLog("getting blockwise seeder info");
    for (int i = 0; i < seeder_info.size(); i++)
    {
        query_threads.push_back(thread(request_client_to_client_socket, 1, seeder_info[i], inpt[2]));
    }
    for (auto it = query_threads.begin(); it != query_threads.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    writeLog("Collected blockwise seeders");
    for (size_t i = 0; i < blockwise_seeders.size(); i++)
    {
        if (blockwise_seeders[i].size() == 0)
        {
            cout << "All parts of the file are not available." << endl;
            writeLog("All parts of file are not available!!!.....Thus terminating");
            return;
        }
    }

    string des_path = inpt[3] + "/" + inpt[2];
    FILE *fp = fopen(des_path.c_str(), "r+");
    if (fp != nullptr)
    {
        printf("The file already exists.\n");
        fclose(fp);
        return;
    }
    string ss(curr_file_size, '\0');
    fstream out(des_path, ios::out | ios::binary);
    out.write(ss.c_str(), ss.size());
    out.close();

    fileChunkInfo[inpt[2]].assign(num_of_blocks, 0);
    isCorruptedFile = false;

    // 3. Build priority queue of blocks (rarest-first)
    priority_queue<BlockInfo> pq;
    for (long long i = 0; i < num_of_blocks; i++)
    {
        pq.push({i, blockwise_seeders[i].size()});
    }

    // 4. Worker function
    auto worker = [&]()
    {
        while (true)
        {
            mtx.lock();
            if (pq.empty())
            {
                mtx.unlock();
                break; // all blocks processed
            }
            BlockInfo block = pq.top();
            pq.pop();
            if (fileChunkInfo[inpt[2]][block.index] == 1)
            { // already downloaded
                mtx.unlock();
                continue;
            }
            fileChunkInfo[inpt[2]][block.index] = 1; // mark as in-progress
            mtx.unlock();

            // pick random seeder for this block
            vector<string> &seeders = blockwise_seeders[block.index];
            string chosen_peer = seeders[rand() % seeders.size()];
            string ccmd = "get_chunk$$" + inpt[2] + "$$" + to_string(block.index) + "$$" + des_path;
            request_client_to_client_socket(2, chosen_peer, ccmd);

            // string command = "get_chunk$$" + filename + "$$" + to_string(chunkNum) + "$$" + destination
        }
    };
    writeLog("Defined Lambada function and now going to fetch the data!!");

    // 5. Launch thread pool
    vector<thread> downloadThreads;
    for (int i = 0; i < min(maxThreads, (int)num_of_blocks); i++)
    {
        downloadThreads.push_back(thread(worker));
    }
    for (auto &t : downloadThreads)
        if (t.joinable())
            t.join();
    bool all_done = true;
    writeLog("All threads joined!!");
    for (size_t i = 0; i < fileChunkInfo[inpt[2]].size(); i++)
    {
        if (fileChunkInfo[inpt[2]][i] != 1)
        {
            all_done = false;
            break;
        }
    }
    writeLog("Checking final status");

    if (all_done && !isCorruptedFile)
    {
        cout << "Download complete ✅ File saved at: " << des_path << endl;
        writeLog("Download complete for file: " + inpt[2]);
    }
    else if (isCorruptedFile)
    {
        cout << "Download completed, but file may be corrupted." << endl;
        writeLog("Download finished with corruption detected: " + inpt[2]);
    }
    else
    {
        cout << "Download incomplete: some chunks missing." << endl;
        writeLog("Download incomplete: some chunks missing for file: " + inpt[2]);
    }
    // if (isCorruptedFile)
    //     cout << "Download completed. File may be corrupted." << endl;
    // else
    //     cout << "Download completed. No corruption detected." << endl;

    downloaded_files.insert({inpt[2], inpt[1]});
}