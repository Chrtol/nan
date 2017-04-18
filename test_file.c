#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
   char *zErrMsg = 0;
   int rc;
   char* sql = malloc(99);
   sqlite3* db;
   sqlite3_stmt* res;

   chroot("/home/christian/nan");

   rc = sqlite3_open("phone.db", &db);

   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else{
      fprintf(stderr, "Opened database successfully\n");
   }

    snprintf(sql, 99, "INSERT INTO phonebook (id, tlf, name) VALUES (200, 98765432, 'amigo');");

    sqlite3_prepare_v2(db, sql, -1, &res, 0);
    sqlite3_step(res);
    sqlite3_finalize(res);
    sqlite3_close(db);
}