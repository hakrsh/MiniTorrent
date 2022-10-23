#include "global.h"
using namespace std;

const long long CHUNK_SIZE = 524288;
int MAX_DOWNLOAD_THREADS = 4;

string sha(string file_name) {
  unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
  FILE *inFile = fopen(file_name.c_str(), "rb");
  SHA_CTX sha1;
  SHA1_Init(&sha1);
  int bytes;
  unsigned char data[1024];
  while ((bytes = fread(data, 1, 1024, inFile)) != 0)
    SHA1_Update(&sha1, data, bytes);
  SHA1_Final(hash, &sha1);
  fclose(inFile);
  // convert sha1 to string
  stringstream ss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    ss << hex << setw(2) << setfill('0') << (int)hash[i];
  }
  return ss.str();
}

struct sockaddr_in create_socket(string ip,int port,int &sockfd, bool isServer){
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                    sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);

    if(isServer){
      // Forcefully attaching socket to the port
      if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
          perror("bind failed");
          exit(EXIT_FAILURE);
      }
    }
    return address;
}

struct tracker_details {
  string ip;
  int port;
};
vector<tracker_details> trackers;


map<int, int> clients;
// peer details structure
struct peer_details {
  int port;
  string ip;
  string path;
  long long size;
};

void download(int new_socket) {
  char buffer[1024];
  // clear the buffer
  memset(buffer, 0, sizeof(buffer));

  // read from client
  int valread = read(new_socket, buffer, 1024);
  // split the buffer into path and chunk_no using stringstream
  stringstream ss(buffer);
  string path;
  string chunk_no;
  ss >> path >> chunk_no;
  
  // open the file using open()
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  long long chunk_no_ll = stoll(chunk_no);
  char *chunk = (char *)malloc(CHUNK_SIZE * sizeof(char));
  // clear the chunk
  memset(chunk, 0, CHUNK_SIZE);
  LOG(INFO) << "Reading chunk " + chunk_no + " from " + path;
  // uncomment the following line to simulate slow download
  /* int sleep_time = rand() % 3 + 1;
  LOG(INFO) << "chunk_no: " + chunk_no + " delay: " + to_string(sleep_time);
  this_thread::sleep_for(chrono::seconds(sleep_time)); */
  int bytes_read = pread(fd, chunk, CHUNK_SIZE, chunk_no_ll * CHUNK_SIZE);
  close(fd);
  if (bytes_read == -1) {
    printf("error reading file\n");
    exit(1);
    // return -1;
  }
  // sent chunk to client
  LOG(INFO) << "Sending chunk " + chunk_no + " to client";
  write(new_socket, chunk, bytes_read);
  // close the socket
  close(new_socket);
  free(chunk);
}

void server(string ip, int port) {
  int server_fd, client_socket, valread;
  struct sockaddr_in address = create_socket(ip, port, server_fd, true);
  int addrlen = sizeof(address);

  if (listen(server_fd, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  vector<thread> download_threads;
  LOG(INFO) << "Client running as server on ip:port " + ip + ":" + to_string(port);
  int clientid = 0;
  while (true) {
    // cout << "waiting for connection...\n";
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    // cout << "connected.\n";
    // clients[client_socket] = clientid++;
    LOG(INFO) << "Got connection from " + string(inet_ntoa(address.sin_addr)) + ":" + to_string(ntohs(address.sin_port));
    LOG(INFO) << "Starting download thread...";
    download_threads.push_back(thread(download, client_socket));
    // handle_conn(client_socket);
  }
  for (auto &i : download_threads)
    if (i.joinable())
      i.join();
}

int connect_to_server(string src, int chunk_no, int output_file_fd, string ip,
                      int port) {
  // printf("connecting to-> %s:%d\n", ip.c_str(), port);
  
  LOG(INFO) << "connecting to-> " + ip + ":" + to_string(port) + " for chunk " +
               to_string(chunk_no);
  int sock, valread;
  struct sockaddr_in serv_addr = create_socket(ip, port, sock, false);

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  // send file name to server and chunk#
  string req = src + " " + to_string(chunk_no);
  send(sock, req.c_str(), req.length(), 0);
  // allocate buffer to receive data
  char buffer[CHUNK_SIZE];
  // clear buffer
  memset(buffer, 0, CHUNK_SIZE);
  // read response from server till 0

  int offset = 0;
  while (1) {
    int n = read(sock, buffer + offset, CHUNK_SIZE);
    if (n <= 0) {
      break;
    }
    offset += n;
  }
  
  LOG(INFO) << "received chunk " + to_string(chunk_no) + " from " + ip + ":" +
               to_string(port);
  // write to file using pwrite()
  LOG(INFO) << "writing chunk " + to_string(chunk_no) + " to output file at location (chunk_no * CHUNK_SIZE): " + to_string(chunk_no * CHUNK_SIZE);
  pwrite(output_file_fd, buffer, offset, chunk_no * CHUNK_SIZE);
  close(sock);
  return 0;
}

void getchunks(vector<peer_details> peers, int output_file_fd,
               string output_file, string gid, string fname, int tracker_sock) {
  long long fsize = peers[0].size;
  vector<thread> threads;
  int num_of_chunks = ceil(fsize / float(CHUNK_SIZE));
  LOG(INFO) << "Total chunks: " + to_string(num_of_chunks);
  // loop chunks
  // pic ip in round robin fashion
  int index = 0;

  int i = 0;
  while(i<num_of_chunks){
    for(int j=0;j<min(MAX_DOWNLOAD_THREADS, num_of_chunks-i);j++){
      threads.push_back(thread(connect_to_server, peers[index].path, i,
                             output_file_fd, peers[index].ip,
                             peers[index].port)); // connect to peer
      index = (index + 1) % peers.size();
      i++;
    }
    LOG(INFO) << "Waiting for threads to finish...";
    for(auto &i:threads)
      i.join();
    LOG(INFO) << "Threads finished.";
    threads.clear();
  }

  // for (int i = 0; i < num_of_chunks; i++) {
  //   // create thread to download chunk
  //   LOG(INFO) << "Creating thread to download chunk: " + to_string(i);
  //   threads.push_back(thread(connect_to_server, peers[index].path, i,
  //                            output_file_fd, peers[index].ip,
  //                            peers[index].port)); // connect to peer
  //   index = (index + 1) % peers.size();
  // }
  // // join threads
  // for (auto &t : threads) {
  //   t.join();
  // }
  close(output_file_fd);
  LOG(INFO) << fname + " downloaded successfully.";
  // print size of output file
  struct stat st;
  stat(output_file.c_str(), &st);
  LOG(INFO) << "Size of output file: " + fname + " is " + to_string(st.st_size);
  // uploading file
  LOG(INFO) << "Seeding file... " + fname;
  string shasum = sha(output_file);
  string upload_req = "upload_file " + output_file + " " + gid + " " + fname +
                      " " + to_string(fsize) + " " + shasum + " peer";
  send(tracker_sock, upload_req.c_str(), upload_req.length(), 0);
  LOG(INFO) << "Seeding complete";
  // read respose from server
  char temp[1024] = {0};
  int valread = read(tracker_sock, temp, 1024);
}
int main(int argc, char const *argv[]) {
  srand(time(0));
  if(argc != 4) {
    printf("Usage: ./client <ip>:<port> <tracker_info.txt> <tracker_no> \n");
    exit(1);
  }
  string ip_port = argv[1];
  string tracker_info_file = argv[2];
  int tracker_no = atoi(argv[3]);
  // parse ip and port
  string ip, port;
  int pos = ip_port.find(":");
  ip = ip_port.substr(0, pos);
  port = ip_port.substr(pos + 1);
  // printf("ip: %s, port: %s\n", ip.c_str(), port.c_str());
  
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
  tracker_no--; // tracker_no is 1 indexed
  int tracker_port = trackers[tracker_no].port;
  string tracker_ip = trackers[tracker_no].ip;

  string logfile = ip_port + ".log";
  google::InitGoogleLogging(logfile.c_str());
  google::SetLogDestination(google::GLOG_INFO, "../log/");

  // run as server
  LOG(INFO) << "Running as server on : " + ip + ":" + port;
  auto server_thread = thread(server, ip, atoi(port.c_str()));

  int sock = 0, valread;
  string curr_user = "$$$";
  while (true) {
    printf(">>");
    string buf;
    getline(cin, buf);
    if (buf.empty())
      continue;
    vector<string> tokens;
    stringstream ss(buf);
    string temp;
    while (ss >> temp)
      tokens.push_back(temp);
    if (tokens[0] == "create_user")
      buf = buf + " " + ip + " " + port;
    if (tokens[0] == "login"){
      tokens[0] = "login_res";
      buf = buf + " " + ip + " " + port; 
    }
    if (tokens[0] == "upload_file") {
      if (tokens.size() < 3) {
        cout << "missing args\n";
        continue;
      } else if (tokens.size() > 3) {
        cout << "too many args\n";
        continue;
      } else if (filesystem::exists(tokens[1]) == 0) {
        cout << "invalid file\n";
        continue;
      } 
      else if (filesystem::is_directory(tokens[1])) {
        cout << "Not a file\n";
        continue;
      }
      else {
        printf("uploading file...\n");
        auto path = tokens[1];
        auto fname = filesystem::path(path).filename();
        long long fsize = filesystem::file_size(path);

        string shasum = sha(path);
        buf = buf + " " + fname.string() + " " + to_string(fsize) + " " +
              shasum + " seeder";
      }
    }
    if (tokens[0] == "download_file") {
      if (tokens.size() < 4) {
        cout << "missing args\n";
        continue;
      } else if (tokens.size() > 4) {
        cout << "too many args\n";
        continue;
      } else if (filesystem::exists(tokens[3]) == 0) {
        cout << "invalid destination\n";
        continue;
      } else
        tokens[0] = "download_res";
    }
    struct sockaddr_in serv_addr = create_socket(tracker_ip, tracker_port, sock, false);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       LOG(INFO) << "Tracker " << tracker_no << " is down";
       LOG(INFO) << "Trying to connect to another tracker";
       for(int i=0; i<trackers.size(); i++) {
         if(i == tracker_no) continue;
         tracker_port = trackers[i].port;
         tracker_ip = trackers[i].ip;
         serv_addr = create_socket(tracker_ip, tracker_port, sock, false);
         if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
           LOG(INFO) << "Tracker " << i << " is down";
           continue;
         }
         else {
           LOG(INFO) << "Connected to tracker " << i;
           tracker_no = i;
           break;
         }
       }
    }
    buf += " " + curr_user; // append curr_user to request 
    send(sock, buf.c_str(), buf.length(), 0);
    char buffer[1024] = {0};
    valread = read(sock, buffer, 1024);
    if (valread < 0)
      continue;
    if (tokens[0] == "download_res" && buffer[0] == '1') {
      // printf("list of peers: %s\n", buffer);
      printf("Downloading...\n");
      auto peers = vector<peer_details>();
      stringstream ss(buffer);
      string p_ip, p_port, p_path, p_size;
      string path;
      long long fsize;
      while (ss >> p_ip >> p_port >> p_path >> p_size) {
        peer_details pd;
        pd.ip = p_ip;
        pd.port = atoi(p_port.c_str());
        pd.path = p_path;
        pd.size = atoll(p_size.c_str());
        peers.push_back(pd);
      }
      string gid = tokens[1];
      string fname = tokens[2];
      string dest_path = tokens[3];
      // remove / from dest_path is present
      if (dest_path[dest_path.length() - 1] == '/')
        dest_path.pop_back();
      string output_file = dest_path + "/" + fname;

      int output_file_fd = open(output_file.c_str(), O_CREAT | O_WRONLY, 0777);
      LOG(INFO) << "downloading " + output_file;
      LOG(INFO) << "list of peers: ";
      for (auto p : peers) {
        LOG(INFO) << p.ip + " " + to_string(p.port) + " " + p.path + " " +
                     to_string(p.size);
      }
      auto getchunks_thread = thread(getchunks, peers, output_file_fd,
                                     output_file, gid, fname, sock);
      getchunks_thread.detach();

    } 
    else if(tokens[0] == "login_res" && buffer[0] == '1') {
      curr_user = tokens[1];
      printf("%s\n", buffer+1);
      LOG(INFO) << "Logged in as " << curr_user;
    }
    else
      printf("%s\n", buffer);
    close(sock);
  }
  return 0;
}
