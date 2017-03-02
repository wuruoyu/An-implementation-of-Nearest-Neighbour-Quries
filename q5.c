#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sqlite3.h"

int main()
{
  sqlite3 *db;
  clock_t rtClock = 0, idxClock = 0;
  int i, rc, length, coordX[100], coordY[100], count, rtTime, idxTime;
  char idxStmt[50], *idxList[] = {"minX", "maxX", "minY", "maxY"}, *errmsg = 0, qryStmt[150];
  
  printf("Enter the length of side: ");
  scanf("%d", &length);

  for (i = 0; i < 100; i++){
    coordX[i] = rand() % 1000;
    coordY[i] = rand() % 1000;
  }

  rc = sqlite3_open("assignment2.db", &db);
  if (rc){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
/*
  for (i = 0; i < 4; i++){
    sprintf(idxStmt, "CREATE INDEX %s_idx ON poi_transformed(%s)", idxList[i], idxList[i]);
    rc = sqlite3_exec(db, idxStmt, NULL, NULL, &errmsg);
   if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", errmsg);
      sqlite3_free(errmsg);
      sqlite3_close(db);
      return 1;
    }
  }
*/
  
  for (count = 0; count < 20; count++){
    for (i = 0; i < 100; i++){
      clock_t rtStart = clock();
      sprintf(qryStmt, "SELECT COUNT(*) FROM poi_Rtree " \
	      "WHERE minX >= %d AND maxX <= %d AND minY >= %d AND maxY <= %d;",
	      coordX[i], coordX[i] + length, coordY[i], coordY[i] + length);
      rc = sqlite3_exec(db, qryStmt, NULL, NULL, &errmsg);
      if (rc != SQLITE_OK){
	fprintf(stderr, "SQL error: %s\n", errmsg);
	sqlite3_free(errmsg);
	sqlite3_close(db);
  clock_t rtend = clock();
  rtClock += rtend - rtStart;
	return 1;
      }
    }
  }


  for (count = 0; count < 20; count++){
    for (i = 0; i < 100; i++){
      clock_t idxStart = clock();
      sprintf(qryStmt, "SELECT COUNT(*) FROM poi_transformed "			\
	      "WHERE minX <= %d AND maxX >= %d AND minY <= %d AND maxY >= %d;",
	      coordX[i], coordX[i] + length, coordY[i], coordY[i] + length);
      rc = sqlite3_exec(db, qryStmt, NULL, NULL, &errmsg);
      if (rc != SQLITE_OK){
	fprintf(stderr, "SQL error: %s\n", errmsg);
	sqlite3_free(errmsg);
	sqlite3_close(db);
  clock_t idxend = clock();
  idxClock += idxend - idxStart;
	return 1;
      }
    }
  }
  

  rtTime = rtClock * 1000 / CLOCKS_PER_SEC / 20;
  idxTime = idxClock * 1000 / CLOCKS_PER_SEC / 20;

  printf("Parameter l: %d\n", length);
  printf("Average runtime with r_tree: %dms\n", rtTime);
  printf("Average runtime without r_tree: %dms\n", idxTime);

  sqlite3_close(db);
  return 0;
}
