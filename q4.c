#include <stdio.h>
#include "sqlite3.h"

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Error: wrong number of parameters");
        return -1;
    }

    /* declare the variables */
    float xMin, xMax, yMin, yMax = 0;
    int rc;
    sqlite3* db = 0;
	char* zErrMsg = 0;
	char* sql = 0;
	sqlite3_stmt* stmt = 0;
	char* class_value = 0;


    if (sscanf(argv[1], "%f", &xMin) != 1) {
        fprintf(stderr, "Error: convert argv[1] to xMin");
        return -1;
    }
    
    if (sscanf(argv[2], "%f", &xMax) != 1) {
        fprintf(stderr, "Error: convert argv[2] to xMin");
        return -1;
    }
    
    if (sscanf(argv[3], "%f", &yMin) != 1) {
        fprintf(stderr, "Error: convert argv[3] to yMin");
        return -1;
    }

    if (sscanf(argv[4], "%f", &yMax) != 1) {
        fprintf(stderr, "Error: convert argv[4] to yMax");
        return -1;
    }

    class_value = argv[5];

    /* open database */
	rc = sqlite3_open("assignment2.db", &db);
	if (rc) {
		fprintf(stderr, "Can't open database\n");
		return 0;
	}


	/* create sql statement */
	sql = 	"SELECT poi_Rtree.id, poi_Rtree.minX, poi_Rtree.maxX, poi_Rtree.minY, poi_Rtree.maxY \
			FROM poi_Rtree, poi_tag \
			WHERE poi_Rtree.id = poi_tag.id AND \
			poi_Rtree.minX >= ? AND \
			poi_Rtree.maxX <= ? AND \
			poi_Rtree.minY >= ? AND \
			poi_Rtree.maxY <= ? AND \
			poi_tag.key = \"class\" AND \
			poi_tag.value = ?";

	/* prepare the statement */
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (rc != SQLITE_OK) {  
        fprintf(stderr, "Preparation failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    /* bind the parameter */
    sqlite3_bind_double(stmt, 1, xMin);
    sqlite3_bind_double(stmt, 2, xMax);
    sqlite3_bind_double(stmt, 3, yMin);
    sqlite3_bind_double(stmt, 4, yMax);
    sqlite3_bind_text(stmt, 5, class_value, -1, NULL);

    /* execute */
   	while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int col;
        for(col = 0; col < sqlite3_column_count(stmt) - 1; col ++) {
        	printf("%s|", sqlite3_column_text(stmt, col));
        }
        printf("%s", sqlite3_column_text(stmt, col));
        printf("\n");
    }

    sqlite3_finalize(stmt);   
    sqlite3_close(db);
    
    return 0;
}
