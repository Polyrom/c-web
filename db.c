#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_ERROR "ERROR:"
#define LOG_INFO "INFO:"

#define DB_NAME "test.db"

int get_post(sqlite3 *db, const char *title_to_search, char *title,
             char *content) {
  int rc;
  char *err_msg = 0;
  char *sql = "SELECT title, content FROM posts WHERE title = ?";
  sqlite3_stmt *stmt;
  // prepare statement
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "%s Failed to prepare statement: %s\n", LOG_ERROR,
            sqlite3_errmsg(db));
    sqlite3_free(err_msg);
    return -1;
  }
  // bind parameter
  rc = sqlite3_bind_text(stmt, 1, title_to_search, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "%s Failed to bind parameter to statement: %s\n", LOG_ERROR,
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  // execute query
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // retrieve the title and content
    strcpy(title, (const char *)sqlite3_column_text(stmt, 0));
    strcpy(content, (const char *)sqlite3_column_text(stmt, 1));
  } else {
    fprintf(stderr, "%s Failed to execute statement: %s\n", LOG_ERROR,
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  sqlite3_finalize(stmt);
  return 0;
}

int populate_db(sqlite3 *db) {
  printf("%s Populating DB %s\n", LOG_INFO, DB_NAME);
  char *err_msg = 0;
  char *sql =
      "DROP TABLE IF EXISTS Posts;"
      "CREATE TABLE Posts(Id INTEGER PRIMARY KEY, Title TEXT, Content TEXT);"
      "INSERT INTO Posts(Title, Content) VALUES('Test post', 'Hello from "
      "sqlite3');";
  int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "%s Failed populate DB: %s\n", LOG_ERROR, err_msg);
    sqlite3_free(err_msg);
    return -1;
  }
  printf("%s DB %s populated\n", LOG_INFO, DB_NAME);
  return 0;
}

int main(void) {
  sqlite3 *db;
  int rc = sqlite3_open_v2(DB_NAME, &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  printf("%s Opening database %s\n", LOG_INFO, DB_NAME);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "%s Failed to open db %s: %s\n", LOG_ERROR, DB_NAME,
            sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  if (populate_db(db) < 0) {
    fprintf(stderr, "%s Failed to run statement: %s\n", LOG_ERROR,
            strerror(errno));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  char title[256];
  char content[1024];
  const char *title_to_search = "Test post";
  if (get_post(db, title_to_search, title, content) < 0) {
    fprintf(stderr, "%s Failed to run statement: %s\n", LOG_ERROR,
            strerror(errno));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  printf("title: %s, content: %s\n", title, content);
  sqlite3_close(db);
  return 0;
}
