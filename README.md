# Peer-to-Peer Group Based File Sharing System

## Dependencies 

*. g++ compiler
   - `sudo apt install g++`
*. OpenSSL library
   - `sudo apt install openssl`

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

## Assumptions

1. Only one tracker is implemented and that tracker should always be online.
2. Cross client login is restricted, ie. ip and port are bound to the user during user creation.
3. File paths should be absolute.

