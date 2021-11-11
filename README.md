# Peer-to-Peer Group Based File Sharing System

## Dependencies 

1. g++ compiler
   - `sudo apt-get install g++`
2. OpenSSL library
   - `sudo apt-get install openssl`

**Platform:** Linux <br/>

## Build Instructions

```
make -j2
```

## Usage

### Tracker

- Run Tracker:

```
./tracker​ <TRACKER INFO FILE> <TRACKER NUMBER>
ex: ./tracker tracker_info.txt 1
```

`<TRACKER INFO FILE>` contains the IP, Port details of all the trackers.

```
Ex:
127.0.0.1
8080
127.0.0.1
8088
```

- Close Tracker:

```
quit
```

### Client:

1. Run Client:

```
./client​ <IP>:<PORT> <TRACKER INFO FILE>
ex: ./client 127.0.0.1:18000 tracker_info.txt
```

2. Create user account:

```
create_user​ <user_id> <password>
```

3. Login:

```
login​ <user_id> <password>
```

4. Create Group:

```
create_group​ <group_id>
```

5. Join Group:

```
join_group​ <group_id>
```

6. Leave Group:

```
leave_group​ <group_id>
```

7. List pending requests:

```
list_requests ​<group_id>
```

8. Accept Group Joining Request:

```
accept_request​ <group_id> <user_id>
```
9. Reject Group Joining Request:

```
reject_request​ <group_id> <user_id>
```

10. List All Group In Network:

```
list_groups
```

11. List All sharable Files In Group:

```
list_files​ <group_id>
```

12. Upload File:

```
​upload_file​ <file_path> <group_id​>
```

13. Download File:​

```
download_file​ <group_id> <file_name> <destination_path>
```

14. Logout:​

```
logout
```

15. Show_downloads: ​

```
show_downloads
```

16. Stop sharing: ​

```
stop_share ​<group_id> <file_name>
```

## Working

1. User should create an account and register with tracker.
2. Login using the user credentials.
3. Tracker maintains information of clients with their files(shared by client) to assist the clients for the communication between peers.
4. User can create Group and hence will become admin of that group.
5. User can fetch list of all Groups in server.
6. User can join/leave group.
7. Group admin can accept group join requests.
8. Share file across group: Shares the filename and SHA1 hash of the complete file as well as piecewise SHA1 with the tracker.
9. Fetch list of all sharable files in a Group.
10. Download:
    1. Retrieve peer information from tracker for the file.
    2. Download file from multiple peers (different pieces of file from different peers - ​piece selection algorithm​) simultaneously and all the files which client downloads will be shareable to other users in the same group. File integrity is ensured using SHA1 comparison.
11. Piece selection algorithm used: Selecting peers in circular fashion
12. Show downloads.
13. Stop sharing file.
14. Logout - stops sharing all files.
15. Whenever client logins, all previously shared files before logout should automatically be on sharing mode.

## Assumptions

1. Only one tracker is implemented and that tracker should always be online.
2. Cross client login is restricted, ie. ip and port are bound to the user during user creation.
3. File paths should be absolute.

