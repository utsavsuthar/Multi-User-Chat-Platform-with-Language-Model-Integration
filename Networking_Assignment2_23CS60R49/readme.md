Ensure FAQs.txt, uuid.c and uuid.h is present in the current directory where server is compiled.

To compile server use
gcc server.c 
To compile client use 
gcc client.c 
In one terminal run 
./server <Port Number>
Any number of terminals for client can be opened, to start the client run
./client <127.0.0.1> <POrt Number>
Note: Ensure port number is same in both client and server

The available queries are
/active : To fetch active users
/send recepient_uuid message : To send message to a peer
/logout : To logout of the server
/chatbot login : To use the chatbot feature which only answers certain queries
/chatbot logout : To logout of the chatbot
/history recepient_uuid : To fetch and print the chat history of the client with the recepient
/history_delete recepient_uuid: To delete the chat history of the client with the recepient from the client's chat history
/delete_all : To delete all chat history of the client
/chatbot_v2 login : to use the gpt chatbot feature which will answer all the question correct or incorrect.
/chatbot_v2 logout: to logout of the gpt chatbot.

Any other queries will not work

Assumptions:
If a client in peer connection logout and then while logging in again they will be provided with a new uuid and will be considered a new user.
Once logged out, the client is assumed to be inactive
Note that only one chat history log file is created for all the users who logs in and is in the peer to peer connection.
The codes in server side and client side are commented at right places to give better understanding of the code.