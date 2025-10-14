#include "client.h"
long long getFileSize(const std::string &fromPath)
{
    struct stat st;
    if (stat(fromPath.c_str(), &st) != 0)
    {
        // error, file not found
        return -1;
    }
    return st.st_size; // size in bytes
}

string get_peicewise_Hash(char *path)
{

    int i, accum;
    FILE *fp1;

    long long fileSize = getFileSize(path);
    if (fileSize == -1)
    {
        return "$";
    }
    int num_of_segments = ceil(static_cast<double>(fileSize) / BLOCK_SIZE);
    char line[SIZE + 1];
    string hash = "";

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("File not found");
        return "$";
    }

    for (int i = 0; i < num_of_segments; i++)
    {
        int accum = 0;
        string segmentString;

        while (accum < BLOCK_SIZE)
        {
            int toRead = min(SIZE, BLOCK_SIZE - accum);
            ssize_t rc = read(fd, line, toRead);
            if (rc <= 0)
                break; // EOF or error

            segmentString.append(line, rc);
            accum += rc;
        }

        hash_of_string(segmentString, hash);
    }

    close(fd);

    // Remove last two characters
    if (hash.size() >= 2)
    {
        hash.pop_back();
        hash.pop_back();
    }

    return hash;
}

void hash_of_string(string segmentString, string &hash)
{
    unsigned char md[20];
    if (!SHA1(reinterpret_cast<const unsigned char *>(&segmentString[0]), segmentString.length(), md))
    {
        printf("Error in hashing\n");
    }
    else
    {
        for (int i = 0; i < 20; i++)
        {
            char buf[3];
            sprintf(buf, "%02x", md[i] & 0xff);
            hash += string(buf);
        }
    }
    hash += "$$";
}

string getFileHash(char *path)
{

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        return "";
    }

    // Prepare SHA256 context
    SHA256_CTX shaCtx;
    if (!SHA256_Init(&shaCtx))
    {
        close(fd);
        fprintf(stderr, "Error initializing SHA256\n");
        return "";
    }

    const size_t BUF_SIZE = 4096; // 4KB buffer
    unsigned char buffer[BUF_SIZE];
    ssize_t bytesRead;

    // Read the file in chunks and update hash
    while ((bytesRead = read(fd, buffer, BUF_SIZE)) > 0)
    {
        if (!SHA256_Update(&shaCtx, buffer, bytesRead))
        {
            close(fd);
            fprintf(stderr, "Error updating SHA256\n");
            return "";
        }
    }

    close(fd);

    if (bytesRead < 0)
    {
        perror("Error reading file");
        return "";
    }

    // Finalize digest
    unsigned char digest[SHA256_DIGEST_LENGTH];
    if (!SHA256_Final(digest, &shaCtx))
    {
        fprintf(stderr, "Error finalizing SHA256\n");
        return "";
    }

    // Convert to hex string
    string hexHash;
    char hexByte[3];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hexByte, "%02x", digest[i] & 0xff);
        hexHash += string(hexByte);
    }

    return hexHash;
}




string calculate_sha1(const vector<char>& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}



