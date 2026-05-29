gcc client.c -o client
gcc server.c -o server

./server &
SERVER_PID=$!

./client

kill "$SERVER_PID" >/dev/null 2>&1
wait "$SERVER_PID" >/dev/null 2>&1
