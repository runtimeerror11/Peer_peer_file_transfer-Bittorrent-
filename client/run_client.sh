# #!/bin/bash
# # Usage: ./run_client.sh <client_ip:port> <tracker_info.txt>
# g++ helperc.cpp mainc.cpp commands_c.cpp -o client
# if [ $? -eq 0 ]; then
#     ./client "$1" "$2"
# else
#     echo "Client compilation failed."
# fi

# # Cretaing executable file -->
# # chmod +x run_client.sh

# #Run the client -->
# # ./run_client.sh 127.0.0.1:5001 tracker_info.txt



#!/bin/bash
# Usage: ./run_client.sh <client_ip:port> <tracker_info.txt>

# Compile all client files
g++ -I. client_client_connection.cpp commands_c.cpp mainc.cpp hashing.cpp helperc.cpp -o client -lssl -lcrypto -pthread -Wno-deprecated-declarations

if [ $? -eq 0 ]; then
    ./client "$1" "$2"
else
    echo "Client compilation failed."
fi

