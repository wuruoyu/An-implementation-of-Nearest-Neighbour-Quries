// NOTICE: point to area distance: to the center

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>

struct Point {
	double x;
	double y;
};

/* struct for spatial information */
struct Rectangle {
	struct Point s;
	struct Point t;
	double dist;
	double mindist;
	double minmaxdist;
};

/* struct for node. for leaf node, node_no is id in table poi,
   for non-leaf node, node_no is node_id in rtree           */
struct Node {
	long node_no;
	char type;
	int count;
	// pruned_flag indicates this node is already pruned
	bool pruned_flag;
	struct Rectangle rect;
	struct Node* branch;
};

/* global varible indicating the nearest neighbour */
struct Node Nearest;

/* no need to open the database several times */
sqlite3* db;

/* global varible to record the MIN minmaxdist */
double MIN_minmaxdist = 1000000;


/* compute the distance between given_point to a rectangle
 * suppose that the distance equals to the distance between 
 * central point and given point
 * central point(x_central, y_central) = ((s.x + t.x)/2, (s.x + t.x)/2)
 */
double objectDIST(struct Rectangle rect, struct Point given_point) {
	return pow(given_point.x - (rect.s.x + rect.t.x)/2, 2) + pow(given_point.y - (rect.s.y + rect.t.y)/2, 2);
}



/* compute the distance between given_point to a rectangle
 * using mindist metric
 */
double compute_mindist(struct Rectangle rect, struct Point given_point) {
	double r1, r2;

  	if (given_point.x < rect.s.x)
    	r1 = rect.s.x;
  	else if (given_point.x > rect.t.x)
    	r1 = rect.t.x;
  	else
    	r1 = given_point.x;

  	if (given_point.y < rect.s.y)
    	r2 = rect.s.y;
  	else if (given_point.y > rect.t.y)
    	r2 = rect.t.y;
  	else 
    	r2 = given_point.y;

  	return  pow((given_point.x - r1), 2) + pow((given_point.y - r2), 2);
}


/* compute the distance between given_point to a rectangle
 * using minmaxdist metric
 */
double compute_minmaxdist(struct Rectangle rect, struct Point given_point) {
	double rM1, rM2, rm1, rm2, dist1, dist2;
	if (given_point.x <= (rect.s.x + rect.t.x) / 2)
		rm1 = rect.s.x;
	else
		rm1 = rect.t.x;

	if (given_point.y <= (rect.s.y + rect.t.y) / 2)
		rm2 = rect.s.y;
	else
		rm2 = rect.t.y;

	if (given_point.x >= (rect.s.x + rect.t.x) / 2)
		rM1 = rect.s.x;
	else
		rM1 = rect.t.x;

	if (given_point.y >= (rect.s.y + rect.t.y) / 2)
		rM2 = rect.s.y;
	else
		rM2 = rect.t.y;

	dist1 = pow((given_point.x - rm1), 2) + pow((given_point.y - rM2), 2);
	dist2 = pow((given_point.y - rm2), 2) + pow((given_point.x - rM1), 2);

	if (dist1 < dist2)
		return dist1;
	else
		return dist2;
}


/* get the child of node from sqlite DB, and compute the 
 * MINDIST and MINMAXDIST between, save them into node
 * and update the global varibals
 */
void genBranchList(struct Point given_point, struct Node* node) {
	char* sql;
	int rc;
	sqlite3_stmt* stmt;

	sql = "SELECT rtreenode(2, poi_rtree_node.data) \
			FROM poi_rtree_node \
			WHERE nodeno = ?;";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (rc) {
		fprintf(stderr, "Fail to prepare\n");
		return;
	}

	sqlite3_bind_int(stmt, 1, node->node_no);
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		char* toParse = sqlite3_column_text(stmt, 0);

		// count how many childs the node has
		int child_counter = 0;
		int i = 0;
		while (toParse[i] != '\0') {
			if (toParse[i] == '{')
				child_counter ++;
			i ++;
		}
		node->count = child_counter;


		// dynamic allocation
		node->branch = (struct Node*)malloc(sizeof(struct Node) * child_counter);
		for (int i = 0; i < node->count; i ++)
			node->branch[i].pruned_flag = false;

		
		// assign the value
		toParse ++;
		char* temp = strtok(toParse, "}");
		for (i = 0; i < child_counter; i ++) {
			sscanf(temp, "%ld %lf %lf %lf %lf", &((node->branch[i]).node_no), 
											   &((node->branch[i]).rect.s.x), 
											   &((node->branch[i]).rect.t.x), 
											   &((node->branch[i]).rect.s.y), 
											   &((node->branch[i]).rect.t.y));
			temp = strtok(NULL, "{");
			temp = strtok(NULL, "}");
		}

	}


	// compute mindist, min and update
	for (int i = 0; i < node->count; i ++) {
		(node->branch[i]).rect.mindist = compute_mindist((node->branch[i]).rect, given_point);
		(node->branch[i]).rect.minmaxdist = compute_minmaxdist((node->branch[i]).rect, given_point);
		if ((node->branch[i]).rect.minmaxdist < MIN_minmaxdist)
			MIN_minmaxdist = node->branch[i].rect.minmaxdist;
	}

	sqlite3_finalize(stmt);
}


/* use for qsort in sortBranchLisr */
int cmp(const void* a, const void* b) {
	return ((struct Node*)a)->rect.mindist - ((struct Node*)b)->rect.mindist;
}


/* given an address of array, sort them according to mindist metric
 * using qsort
 */
void sortBranchList(struct Node* node) {
	qsort(node->branch, node->count, sizeof(struct Node), cmp);
}

/* downward pruning
 * discard the nodes whose mindist is bigger than Min_minmaxdist
 */
void pruneBranchList_downward(struct Node* node, struct Point given_point) {
	for (int i = 0; i < node->count; i ++) {
		if (node->branch[i].rect.mindist > MIN_minmaxdist){
			for (int j = i; j < node->count; j ++)
				node->branch[i].pruned_flag = true;
			break;
		}
	}
}


/* upward pruning
 * discard the nodes whose mindst in bigger than the Nearest distance for now
 */
void pruneBranchList_upward(struct Node* node, struct Point given_point) {
	for (int i = 0; i < node->count; i ++) {
		if (node->branch[i].rect.mindist > Nearest.rect.dist) {
			for (int j = i; j < node->count; j ++)
				node->branch[i].pruned_flag = true;
			break;
		}
	}
}


/* The recursive function implemented as the paper's pseudo code
 * first decide what kind of node it is(L for LEAF and N for NON-LEAF)
 */
void nearestNeighborSearch(struct Node* node, struct Point given_point) {
	// to decide what kind of node it is
	char* sql;
	int rc;
	sqlite3_stmt* stmt;

	sql = "SELECT COUNT(*) \
			FROM poi_rtree_parent \
			WHERE parentnode = ?;";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (rc) {
		fprintf(stderr, "Fail to prepare\n");
		return;
	}

	sqlite3_bind_int(stmt, 1, node->node_no);
	sqlite3_step(stmt);
	if (rc) {
		fprintf(stderr, "Fail to execute in NNS\n");
		return;
	}
	int if_parent = sqlite3_column_int(stmt, 0);
	if (if_parent == 0)
		node->type = 'L';
	else
		node->type = 'N';


	// if the node is LEAF
	if (node->type == 'L') {

		// generate active branch list
		genBranchList(given_point, node);

		for (int i = 0; i < node->count; i ++) {
			node->branch[i].type = 'P';
			node->branch[i].rect.dist = objectDIST(node->branch[i].rect, given_point);

			if (node->branch[i].rect.dist < Nearest.rect.dist) {
				Nearest = node->branch[i];
			}
		}

	}
	else {
		// generate active branch list
		genBranchList(given_point, node);

		// sort ABL based on ordering metric values
		sortBranchList(node);

		// perform downward pruning
		// may discard all branches
		// notice that Node has a indicate bit!
		pruneBranchList_downward(node, given_point);

		// iterate through the active branch list
		for (int i = 0; i < node->count; i ++) {
			if (!node->branch[i].pruned_flag) {
				struct Node new_node = node->branch[i];
				
				// recursively visit child nodes
				nearestNeighborSearch(&new_node, given_point);

				// perform upward pruning
				pruneBranchList_upward(node, given_point);
			}
		}
	}

	sqlite3_finalize(stmt);
}


int main() {

	/* declare and initialize variables */
	struct Point given_point;
	Nearest.rect.dist = LONG_MAX;
	int rc;
	struct Node root_node;


	/* prompt to input the point(x, y) */
	printf("%s\n", "Please input point(x,y) Eg. 3.2 3.4");
	if (scanf("%lf", &given_point.x) && scanf("%lf", &given_point.y))
		printf("\n");
	else {
		printf("%s\n", "Input Error");
		return -1;
	}


	// open the database
	rc = sqlite3_open("assignment2.db", &db);
	if (rc) {
		fprintf(stderr, "Can't open database\n");
		return -1;
	}

	
	// fill the info of root_node
	root_node.pruned_flag = false;
	root_node.node_no = 1;

	nearestNeighborSearch(&root_node, given_point);

	printf("Node Number: %ld\n", Nearest.node_no);
	printf("Distance: %lf\n", Nearest.rect.dist);
	printf("s.x: %lf\n", Nearest.rect.s.x);
	printf("s.y: %lf\n", Nearest.rect.s.y);
	printf("t.x: %lf\n", Nearest.rect.t.x);
	printf("t.y: %lf\n", Nearest.rect.t.y);

	sqlite3_close(db);
	return 0;
}
