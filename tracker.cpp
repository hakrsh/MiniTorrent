#include "global.h"
using namespace std;

const long long CHUNK_SIZE = 524288;

map<int, int> clients;
set<string> commands{
    "create_user",    "login",       "create_group",  "join_group",
    "leave_group",    "list_groups", "list_requests", "accept_request",
    "reject_request", "upload_file", "list_files",    "download_file",
    "logout",         "list_peers",  "stop_sharing",  "show_downloads"};

struct tracker_details {
  string ip;
  int port;
};
vector<tracker_details> trackers;

struct download_info {
  string gid, fname, status, msg;
  // char status;
};

struct user_info {
  string name, passwd;
  string ip;
  string port;
  bool alive;
  set<string> grps;
  map<string, download_info> downloads;
};
map<string,bool> user_status;

struct file_info {
  string name, gid, path, hash;
  map<string, string> peers;
  long long size;
};

struct group_info {
  string id;
  string owner;
  set<string> users;
  map<string, file_info> files;
  set<string> pendingReq;
};

map<string, user_info> users;
map<string, group_info> groups;

struct sockaddr_in create_socket(string ip,int port,int &server_fd, bool isServer){
    int client_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                    sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);

    if(isServer){
      // Forcefully attaching socket to the port
      if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
          perror("bind failed");
          exit(EXIT_FAILURE);
      }
    }
    return address;
}

int tracker_no;
void sync(string msg){
  for(int i=0;i<trackers.size();i++){
    if(i==tracker_no) continue;
    int client_socket, valread;
    struct sockaddr_in address;
    address = create_socket(trackers[i].ip,trackers[i].port,client_socket,false);
    int addrlen = sizeof(address);
    if (connect(client_socket, (struct sockaddr *)&address, sizeof(address)) <
        0) {
        LOG(INFO) << "Tracker " << i+1 << " is down";
        continue;
    }
    send(client_socket, msg.c_str(), msg.length(), 0);
    close(client_socket);
  }
}
vector<string> tokens;
string create_user() {
  string res;
  if (tokens.size() < 5)
    res = "missing args";
  else if (tokens.size() > 5)
    res = "too many args";
  else {
    string username = tokens[1];
    string passwd = tokens[2];
    string ip = tokens[3];
    string port = tokens[4];
    if (users.count(username))
      res = "User already exist!";
    else {
      user_info usr;
      usr.name = username;
      usr.passwd = passwd;
      usr.ip = ip;
      usr.port = port;
      users[username] = usr;
      res = "User created";
    }
  }
  return res;
}
string login(string &curr_user) {
  string res;
  // ip port including total 5 args
  if (tokens.size() < 5)
    res = "missing args";
  else if (tokens.size() > 5)
    res = "too many args";
  else {
    string username = tokens[1];
    string passwd = tokens[2];
    string ip = tokens[3];
    string port = tokens[4];

    if (users.count(username) == 0)
      res = "User doesn't exist!";
    else if (user_status[username])
      res = "One user already logged in";
    else if (users[username].passwd != passwd) {
      res = "Invalid password";
    } else if (users[username].ip != ip || users[username].port != port)
      res = "Cross client login restricted!";
    else {
      user_status[username] = true;
      // users[username].ip = ip;
      // users[username].port = port;
      users[username].alive = true;
      curr_user = username;
      res = "1login successful";
    }
  }
  return res;
}
string logout(string curr_user) {
  string res;
  // ip port including total 5 args
  if (tokens.size() > 1)
    res = "too many args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    user_status[curr_user] = false;
    users[curr_user].alive = false;
    res = "logout successful";
  }
  return res;
}
string create_group(string curr_usr) {
  string res;
  if (tokens.size() < 2)
    res = "missing args";
  else if (tokens.size() > 2)
    res = "too many args";
  else {
    string gid = tokens[1];
    if (groups.count(gid))
      res = "group already exists";
    else if (!user_status[curr_usr])
      res = "Please login first";
    else {
      group_info grp;
      grp.id = gid;
      grp.owner = curr_usr;
      grp.users.insert(curr_usr);
      groups[gid] = grp;
      users[curr_usr].grps.insert(gid);
      res = "group created";
    }
  }
  return res;
}
string list_groups(string curr_usr) {
  string res;

  if (tokens.size() > 1)
    res = "too many args";
  else if (!user_status[curr_usr])
    res = "Please login first";
  else if (groups.empty())
    res = "No groups";
  else {
    for (auto &i : groups)
      res = res + i.first + "\n";
    res.pop_back();
  }
  return res;
}
string join_group(string curr_user) {
  string res;
  if (tokens.size() > 2)
    res = "too many args";
  else if (tokens.size() < 2)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.users.count(curr_user))
        res = "you are already a member";
      else {
        grp.pendingReq.insert(curr_user);
        groups[gid] = grp;
        res = "Request has been sent";
      }
    }
  }
  return res;
}
string leave_group(string curr_user) {
  string res;
  if (tokens.size() > 2)
    res = "too many args";
  else if (tokens.size() < 2)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.users.count(curr_user) == 0)
        res = "you are not a member";
      else if (grp.owner == curr_user){
        bool no_users = true;
        bool no_files = false;
        bool no_pending = false;
        if(grp.pendingReq.empty()){
          no_pending = true;
        }
        if(grp.files.empty()){
          no_files = true;
        }
        for (auto &i : grp.users)
          if (i != curr_user){
            no_users = false;
            break;
          }
        if(no_users && no_files && no_pending){
          groups.erase(gid);
          res = "group deleted";
        }
        else{
          res = "Admin cant leave: group has files/users/pending requests";
        }
      }
      else {
        bool sharing = false;
        for (auto &i : grp.files)
          if (i.second.peers.count(curr_user)) {
            sharing = true;
            break;
          }
        if (sharing)
          res = "cant leave, you are sharing files";
        else {
          grp.users.erase(curr_user);
          groups[gid] = grp;
          users[curr_user].grps.erase(gid);
          res = "left group";
        }
      }
    }
  }
  return res;
}
string list_requests(string curr_user) {
  string res;
  if (tokens.size() > 2)
    res = "too many args";
  else if (tokens.size() < 2)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.owner != curr_user)
        res = "you are not the admin";
      else {
        if (grp.pendingReq.empty())
          res = "No pending requests";
        else {
          for (auto &i : grp.pendingReq)
            res = res + i + "\n";
          res.pop_back();
        }
      }
    }
  }
  return res;
}

string accept_request(string curr_user) {
  string res;
  if (tokens.size() > 3)
    res = "too many args";
  else if (tokens.size() < 3)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid, uid;
    gid = tokens[1];
    uid = tokens[2];
    if (users.count(uid) == 0)
      res = "Invalid User!";
    else if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.owner != curr_user)
        res = "you are not the admin";
      else if (grp.users.count(uid))
        res = "already a member";
      else if (grp.pendingReq.empty())
        res = "Invalid request";
      else {
        grp.users.insert(uid);
        grp.pendingReq.erase(uid);
        groups[gid] = grp;
        users[uid].grps.insert(gid);
        res = "request accepted";
      }
    }
  }
  return res;
}
string reject_request(string curr_user) {
  string res;
  if (tokens.size() > 3)
    res = "too many args";
  else if (tokens.size() < 3)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid, uid;
    gid = tokens[1];
    uid = tokens[2];
    if (users.count(uid) == 0)
      res = "Invalid User!";
    else if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.owner != curr_user)
        res = "you are not the admin";
      else if (grp.users.count(uid))
        res = "already a member";
      else if (grp.pendingReq.empty())
        res = "Invalid request";
      else {
        grp.pendingReq.erase(uid);
        groups[gid] = grp;
        res = "request rejected";
      }
    }
  }
  return res;
}
string upload_file(string curr_user) {
  string res;
  // if (tokens.size() > 3)
  //   res = "too many args";
  // else if (tokens.size() < 3)
  //   res = "missing args";
  if (!user_status[curr_user])
    res = "Please login first";
  else {
    string path = tokens[1];
    string gid = tokens[2];
    string fname = tokens[3];
    long long fsize = stoll(tokens[4]);
    string fhash = tokens[5];
    string type = tokens[6];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    // else if (filesystem::exists(path) == 0)
    //   res = "Invalid file";
    else if (users[curr_user].grps.count(gid) == 0)
      res = "You are not a member of this group";
    else {
      // auto fname = filesystem::path(path).filename();
      if (groups[gid].files.count(fname) == 0) {
        file_info f;
        f.name = fname;
        f.path = path;
        f.gid = gid;
        f.size = fsize;
        f.hash = fhash;
        // f.peers.insert(curr_user);
        f.peers[curr_user] = path;
        // files[fname] = f;
        groups[gid].files[fname] = f;
        // groups[gid].files.insert(fname);
      } else {
        // groups[gid].files[fname].peers.insert(curr_user);
        groups[gid].files[fname].peers[curr_user] = path;
      }
      if (type == "peer") {
        users[curr_user].downloads[fname].status = "C";
        if (groups[gid].files[fname].hash != fhash)
          users[curr_user].downloads[fname].msg = "Corrupted";
        else
          users[curr_user].downloads[fname].msg = "OK";
      }
      res = "uploaded";
    }
  }
  return res;
}
string list_files(string curr_user) {
  string res;
  if (tokens.size() > 2)
    res = "too many args";
  else if (tokens.size() < 2)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    else {
      auto grp = groups[gid];
      if (grp.users.count(curr_user) == 0)
        res = "you are not a member of this group";
      else {
        if (grp.files.empty())
          res = "No files";
        else {
          for (auto &i : grp.files)
            res = res + i.first + "\n";
          res.pop_back();
        }
      }
    }
  }
  return res;
}
string download_file(string curr_user) {
  string res;
  // if (tokens.size() > 4)
  //   res = "too many args";
  // else if (tokens.size() < 4)
  //   res = "missing args";
  if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    string fname = tokens[2];
    string path = tokens[3];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    // else if (filesystem::exists(path) == 0)
    //   res = "Invalid path";
    else if (users[curr_user].grps.count(gid) == 0)
      res = "You are not a member of this group";
    else if (users[curr_user].downloads.count(fname) != 0) {
      if (users[curr_user].downloads[fname].status == "D")
        res = "already downloading";
      else if (users[curr_user].downloads[fname].status == "C")
        res = "already downloaded";
    } else {
      auto grp = groups[gid];
      if (grp.files.count(fname) == 0)
        res = "File doesn't exist";
      else {
        bool nopeer = true;
        for (auto &[peer, path] : grp.files[fname].peers)
          if (users[peer].alive) {
            nopeer = false;
            res = res + users[peer].ip + " " + users[peer].port + " " + path +
                  " " + to_string(grp.files[fname].size) + " ";
            download_info di;
            di.gid = gid;
            di.status = "D";
            users[curr_user].downloads[fname] = di;
          }
        if (nopeer)
          res = "No active peers";
      }
    }
  }
  return res;
}
string list_peers(string curr_user) {
  string res;
  if (tokens.size() > 3)
    res = "too many args";
  else if (tokens.size() < 3)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    string fname = tokens[2];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    // else if (filesystem::exists(path) == 0)
    //   res = "Invalid path";
    else if (users[curr_user].grps.count(gid) == 0)
      res = "You are not a member of this group";
    else {
      auto grp = groups[gid];
      if (grp.files.count(fname) == 0)
        res = "File doesn't exist";
      else {
        bool nopeer = true;
        for (auto &[peer, path] : grp.files[fname].peers)
          if (users[peer].alive) {
            nopeer = false;
            res = res + users[peer].ip + " " + users[peer].port + " " + path +
                  " " + to_string(grp.files[fname].size) + "\n";
          }
        if (nopeer)
          res = "No active peers!";
        else
          res.pop_back(); // remove that last \n
      }
    }
  }
  return res;
}
string stop_sharing(string curr_user) {
  string res;
  if (tokens.size() > 3)
    res = "too many args";
  else if (tokens.size() < 3)
    res = "missing args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else {
    string gid = tokens[1];
    string fname = tokens[2];
    if (groups.count(gid) == 0)
      res = "group doestn't exist";
    // else if (filesystem::exists(path) == 0)
    //   res = "Invalid path";
    else if (users[curr_user].grps.count(gid) == 0)
      res = "You are not a member of this group";
    else {
      auto grp = groups[gid];
      if (grp.files.count(fname) == 0)
        res = "File doesn't exist";
      else if (grp.files[fname].peers.count(curr_user) == 0)
        res = "You are not sharing this file";
      else {
        grp.files[fname].peers.erase(curr_user);
        if (grp.files[fname].peers.empty())
          grp.files.erase(fname);
        groups[gid] = grp;
        res = "Stopped sharing";
      }
    }
  }
  return res;
}
string show_downloads(string curr_user) {
  string res;

  if (tokens.size() > 1)
    res = "too many args";
  else if (!user_status[curr_user])
    res = "Please login first";
  else if (users[curr_user].downloads.empty())
    res = "No downloads";
  else {
    for (auto &i : users[curr_user].downloads) {
      res = res + "[" + i.second.status + "] " + i.first + " " + i.second.gid +
            " " + i.second.msg + "\n";
    }
    res.pop_back(); // remove that last \n
  }
  return res;
}
void parsecmd(string s) {
  tokens.clear();
  stringstream ss(s);
  string temp;
  while (ss >> temp)
    tokens.push_back(temp);
}
void handle_conn(int client_socket) {
    char req[1024] = {0};
    int valread = read(client_socket, req, 1024);
    if (valread < 1)
      return;
    // printf("%d\n", valread);
    parsecmd(req);
    printf("client%d: %s\n", clients[client_socket], req);
    string res,curr_user;
    curr_user = tokens.back(); // last token is the username
    tokens.pop_back();
    if (commands.count(tokens[0]) == 0)
      res = "Invalid Command!";
    else if (tokens[0] == "create_user")
      res = create_user();
    else if (tokens[0] == "login")
      res = login(curr_user);
    else if (tokens[0] == "logout")
      res = logout(curr_user);
    else if (tokens[0] == "create_group")
      res = create_group(curr_user);
    else if (tokens[0] == "list_groups")
      res = list_groups(curr_user);
    else if (tokens[0] == "join_group")
      res = join_group(curr_user);
    else if (tokens[0] == "leave_group")
      res = leave_group(curr_user);
    else if (tokens[0] == "list_requests")
      res = list_requests(curr_user);
    else if (tokens[0] == "accept_request")
      res = accept_request(curr_user);
    else if (tokens[0] == "reject_request")
      res = reject_request(curr_user);
    else if (tokens[0] == "upload_file")
      res = upload_file(curr_user);
    else if (tokens[0] == "list_files")
      res = list_files(curr_user);
    else if (tokens[0] == "download_file")
      res = download_file(curr_user);
    else if (tokens[0] == "stop_sharing")
      res = stop_sharing(curr_user);
    else if (tokens[0] == "list_peers")
      res = list_peers(curr_user);
    else if (tokens[0] == "show_downloads")
      res = show_downloads(curr_user);
    else
      res = "invalid command";
    // getline(cin, buf);

    //   send(client_socket, hello, strlen(hello), 0);
    send(client_socket, res.c_str(), res.length(), 0);
    // printf("Hello message sent\n");
    close(client_socket);
    LOG(INFO) << "Connection Closed";
}
void quit(string tracker_info_file) {
  string s;
  while (1) {
    getline(cin, s);
    if (s == "quit") 
      exit(0);
  }
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printf("Usage: ./tracker <tracker_info.txt> <tracker_no>\n");
    exit(1);
  }
  string tracker_info_file = argv[1];
  tracker_no = atoi(argv[2]);
  
  ifstream tracker_info;
  tracker_info.open(tracker_info_file);
  if (!tracker_info.is_open()) {
    printf("Error: tracker_info.txt not found\n");
    exit(1);
  }
  while (tracker_info.good()) {
    tracker_details temp;
    tracker_info >> temp.ip >> temp.port;
    trackers.push_back(temp);
  }
  tracker_info.close();
  if (tracker_no >= trackers.size()) {
    printf("Error: tracker_no is invalid\n");
    exit(1);
  }
  int tracker_port = trackers[tracker_no - 1].port;
  string tracker_ip = trackers[tracker_no - 1].ip;
  
  string logfile = tracker_ip + to_string(tracker_port) + ".log";
  google::InitGoogleLogging(logfile.c_str());
  google::SetLogDestination(google::GLOG_INFO, "../log/");

  // create thread to handle quit command
  LOG(INFO) << "Creating thread to handle quit command";
  thread exit_thread(quit, tracker_info_file);
  exit_thread.detach();

  int server_fd, client_socket, valread;
  LOG(INFO) << "Creating socket";
  struct sockaddr_in address = create_socket(tracker_ip, tracker_port, server_fd,true);
  int addrlen = sizeof(address);
  LOG(INFO) << "Socket created";
  cout << "Tracker " << tracker_no << " is up and running" << endl;
  cout << "Tracker IP Address: " << tracker_ip << endl;
  cout << "Tracker Port Number: " << tracker_port << endl;
  cout << "Enter 'quit' to exit" << endl;
  cout << "----------------------------------------------" << endl;

  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  vector<thread> threads;

  int clientid = 0;
  while (true) {
    // cout << "waiting for connection...\n";
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    // cout << clientid << " connected" << endl;
    clients[client_socket] = clientid++;

    threads.push_back(thread(handle_conn, client_socket));
    // handle_conn(client_socket);
  }
  for (auto &i : threads)
    if (i.joinable())
      i.join();
  return 0;
}
