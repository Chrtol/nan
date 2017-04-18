#include <arpa/inet.h>
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sqlite3.h>

#define LOKAL_PORT  80 
#define BAK_LOGG    10 //Max incoming connections in queue 
#define READ_SIZE   8192

char* get_mime_type(char* buf);
char* split_filename(char* buffer);
char* get_request(char* buffer);
char* parse_xml(char* xml, char* tag);
char* xml_from_request(char* req);

int main () {

  struct sockaddr_in  lok_adr;
  int sd, ny_sd;
  uid_t nuid = 1001;
  gid_t ngid = 1001;
  char* bufbuf = (char*) malloc(READ_SIZE);
  char* mime_type;
  char* fname;
  char* file_name;
  char* request_name;
  int fd;
  char* buf = (char*) malloc(READ_SIZE);
  int size_read;
  char* xml_req = (char*) malloc(READ_SIZE);
  char* buffer;
  sqlite3* db;
  sqlite3_stmt* res;

  // Establish socket structure
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(-1 == sd)
    exit(1);
  
  if(-1 == setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)))
    exit(2);

  // Initialize local address
  lok_adr.sin_family      = AF_INET;
  lok_adr.sin_port        = htons((u_short)LOKAL_PORT); 
  lok_adr.sin_addr.s_addr = htonl(         INADDR_ANY);

  // Connect socket and local address
  if (0 == bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)))
    printf("Process %d is connected to port %d.\n", getpid(), LOKAL_PORT);
  else
    exit(3);

  //Demonizing
  if(0 != fork())
    exit(2);

  //Change webroot
  chroot("/home/cft/nan");

  //Make session leader
  setsid();

  //Ignore child processes
  signal(SIGHUP,  SIG_IGN);
  signal(SIGCHLD, SIG_IGN);

  //Privilege separation for using port 80
  setuid(nuid);
  setgid(ngid);

  if(0 != fork())
    exit(2);

  // Wait for request
  if(-1 == listen(sd, BAK_LOGG))
    exit(4); 

  while(1) { 
    // Accept incoming requests
    ny_sd = accept(sd, NULL, NULL);    

    if(0 == fork()) {
      dup2(ny_sd, 1); // socket -> stdout
      dup2(ny_sd, 0); // socket -> stdin

      //Read HTTP request and save to buffer
      int bytecount = read(0, bufbuf, READ_SIZE);
      buffer = malloc(strlen(bufbuf));
      strcpy(buffer, bufbuf); 

      if (-1 != bytecount) {
        file_name = split_filename(bufbuf);
        
        if(0 == strcmp(file_name, "/")) {
          file_name = malloc(12);
          strcpy(file_name, "/index.html");
        }

        fname = malloc(strlen(file_name) * sizeof(char));
        strcpy(fname, file_name);
        request_name = get_request(bufbuf);

        mime_type = get_mime_type(file_name);

        if(0 == strcmp(request_name, "GET")) {
          printf("HTTP/1.1 200 OK\n");
          printf("Content-Type: %s\n\n", mime_type);

          if (0 == strcmp(fname, "api/phonebook")) {
            char* req;
            char* sql = malloc(128);
            sqlite3* db;
            sqlite3_stmt* res;

            sqlite3_open("phone.db", &db);
            sqlite3_prepare_v2(db, "SELECT * FROM phonebook;", -1, &res, 0);
            sqlite3_step(res);
            sqlite3_finalize(res);
            sqlite3_close(db);
          } else {
           fd = open(fname, O_RDONLY);
           while ((int) (size_read = read(fd, buf, READ_SIZE)) > 0)
             write(1, buf, size_read);
          }
        } //End "get"
        else if (0 == strcmp(request_name, "POST")) {
          char* name;
          char* tmp_tlf;
          char* tmp_id;
          int tlf;
          int id;
          char* xml;
          char* sql = malloc(128);
          int rc;

          xml = xml_from_request(buffer);

          name      = parse_xml(xml, "name");
          tmp_tlf   = parse_xml(xml, "tlf");
          tmp_id    = parse_xml(xml, "id");

          tlf = atoi(tmp_tlf);
          id  = atoi(tmp_id);

          printf("HTTP/1.1 200 OK\n");
          printf("Content-Type: text/plain\n\n");

          snprintf(sql, 128, "INSERT INTO phonebook(id, tlf, name) VALUES(?, ?, ?);");

          rc = sqlite3_open("db/new.db", &db);
          if (rc) {
            fprintf(stderr, "Open failed: %s\n", sqlite3_errmsg(db));
            return(0);
          }
          rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
          if (rc) {
            fprintf(stderr, "Preparation failed: %s\n", sqlite3_errmsg(db));
            return(0);
          }

          sqlite3_bind_int(res, 1, id);
          sqlite3_bind_int(res, 2, tlf);
          sqlite3_bind_text(res, 3, name, strlen(name), SQLITE_STATIC);

          rc = sqlite3_step(res);
          /*if (rc) {
            fprintf(stderr, "Step failed: %s\n", sqlite3_errmsg(db));
            return(0);
          } else
            fprintf(stderr, "Step successful"); */

          rc = sqlite3_finalize(res);
          if (rc) {
            fprintf(stderr, "Finalization failed: %s\n", sqlite3_errmsg(db));
            return(0);
          }
          rc = sqlite3_close(db);
          if (rc) {
            fprintf(stderr, "Closing failed: %s\n", sqlite3_errmsg(db));
            return(0);
          }
        } //End "post"
        else if (0 == strcmp(request_name, "PUT")) {

        } //End "put"
        else if (0 == strcmp(request_name, "DELETE")) {

        } //End "delete"
      }
      shutdown(ny_sd, SHUT_RDWR);
      exit(0);
    }
    close(ny_sd);
  }
  return 0;
}

char* get_mime_type(char* buf) {
  char* org = buf;
  char* new = buf;

  strtok(buf, ".");

  if (strcmp(buf, org))
    return "text/plain";

  else {
    new = strtok(NULL, ".");

    if (0 == strcmp(new, "txt")) 
      return "text/plain";
    else if(0 == strcmp(new, "html"))
      return "text/html";
    else if (0 == strcmp(new, "png"))
      return "image/png";
    else if (0 == strcmp(new, "xml"))
      return "application/xml";
    else if (0 == strcmp(new, "xsl"))
      return "application/xslt+xml";
    else if (0 == strcmp(new, "css"))
      return "text/css";
    else if (0 == strcmp(new, "dtd"))
      return "application/xml-dtd";
    else if (0 == strcmp(new, "js"))
      return "application/javascript";
    else
      return "text/plain";
  }
  return (char*) '0';
}

char* split_filename(char* buffer) {
    char* filename = strtok(buffer, " \n");
    filename = strtok(NULL, " ");
    
    return filename;
}

char* get_request(char* buffer) {
  strtok(buffer, " ");
  return buffer;
}

char* parse_xml(char* xml, char* tag) {
    char* result;
    int i, str_length;
    int j  = 0;
    int k  = 0;
    int fp = 0;
    int max;

    if      (0 == strcmp(tag, "name"))
      max = 2;
    else if (0 == strcmp(tag, "tlf"))
      max = 4;
    else if (0 == strcmp(tag, "id"))
      max = 6;

    for(i = 0; i < (int) strlen(xml); i++) {
      if (xml[i] == '<') 
          j++;
      
      if (j > max && xml[i] == '<') {
        k = i+1;
          
        while (xml[k] != '<')
          k++;     
      }

      if (xml[i] == '>' && j > max)
        fp = i;
      else if (j > max && xml[i+1] == '<')
        break;
    } 

    k--;
    str_length = k-fp;
    
    result = malloc(str_length+1);
    memcpy(result, xml+fp+1, str_length);
    
    return result;
}

char* xml_from_request(char* req) {
  int i;
  int start = 0;
  char* result;
  int str_len;

  for (i = 0; i < strlen(req); i++) {
    if (req[i] == '<') {
      start = i;
      break;
    } 
  }

  str_len = strlen(req) - start;

  result = malloc(str_len +1);
  memcpy(result, req+start, str_len+1);

  return result;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
  int i;

  for (i = 0; i < argc; i++) {
    if (0 == (i+1) % 3) {
      printf("<contact>");
      printf("<id>%s</id>", argv[i] ? argv[i] : "");
    }

    else if (1 == (i+1) % 3)
      printf("<tlf>%s</tlf>", argv[i] ? argv[i] : "");

    else if (2 == (i+1) % 3) {
      printf("<name>%s</name>", argv[i] ? argv[i] : "");
      printf("</contact>");
    }
  }
}