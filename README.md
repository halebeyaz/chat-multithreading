chat-halebeyaz
chat-halebeyaz created by GitHub Classroom

how to run server: gcc chat_server.c -o server ./server

how to run client: gcc chat_client.c -o client -pthread ./client 127.0.0.1

!!! clients cant see the rooms created by others. messaging and seeing other messages work fine; but since the clients cant enter the other rooms created by other clients, 
they cant message each other unless we remove the if clause that checks if they are in the same room in the function receive_handler
