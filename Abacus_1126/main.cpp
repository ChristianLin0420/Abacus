#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <vector>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <fstream>

using namespace std;

FILE *filename, *nodes_file, *coordinates_file, *RowInfos_file;
FILE *solution_file;

const char *file_path;
char nodes_file_name[50], coordinates_file_name[50], rowInfos_file_name[50], result_file_name[50];

// Node information
struct nodeInfo {
    int id;
    int original_index;
    int width;
    int height;
    float x_coordinate;
    float y_coordinate;
    int new_x;
    int new_y;
};

struct terminal {
    int id;
    int width;
    int x_coordinate;
    terminal(int _id, int _width, int _x) : id(_id), width(_width), x_coordinate(_x) {}
};

struct cluster {
    int x_start;
    int total_width;
    double total_cost;
    vector<nodeInfo> node_collection;
};

// Row information
struct rowInfo {
    int y_coordinate;
    vector<terminal> terminals;
};

struct subRow {
    int x_coordinate;
    int y_coordinate;
    int width;
    nodeInfo tmp;
    vector<cluster> clusters;
};

struct efficient_row {
    int y;
    vector<int> row_index;
};

int MaxDisplacement;

nodeInfo* nodes;
vector<nodeInfo> node_vec;
int NumNodes;
int NumTerminals;

rowInfo* rows;
int NumRow;
int rowX;
int rowHeight;
int rowWidth;
int siteWidth;
int siteOriented;
int siteSpacing;
int siteSymmetry;

vector<subRow> subRows;
efficient_row* e_rows;

double current_best_cost = 0;

char fileTitle;

double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time,NULL)) return 0;
    
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

string get_file_path(const string &s) {
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
    size_t i = s.rfind(sep, s.length());
    if (i != string::npos)
        return(s.substr(0, i+1));
    return("");
}

bool check_input(int argc, char** argv) {
    if(argc != 2 || !strstr(argv[1], ".aux")) {
        cerr << " Usage: " << argv[0] << " <aux file>\n";
        cerr << " Caution:\n";
        cerr << "  aux file should be .aux\n";
        return 0;
    }

    if(!(filename = fopen(argv[1], "r"))) {
        cerr << " Can not open .aux file\n";
        return 0;
    } else {
        string aux_file_path(argv[1]);
        file_path = get_file_path(aux_file_path).c_str();
        aux_file_path = aux_file_path.substr(aux_file_path.rfind('/', aux_file_path.length()) + 1, aux_file_path.length());
        aux_file_path = aux_file_path.substr(0, aux_file_path.rfind('.', aux_file_path.length()));
        strcpy(result_file_name, "../output/");
        strcat(result_file_name, aux_file_path.c_str());
        strcat(result_file_name, ".result");
        cout << "result file name: " << result_file_name << endl;
        if(!(solution_file = fopen(result_file_name, "w+"))) {
            cerr << " Can not open .result file\n";
            return 0;
        }
        return 1;
    }
}

string updateName(char* target) {
    string tmp = "";
    int i = 0;
    
    while (target[i] != '\0') {
        tmp = tmp + target[i++];
    }
    
    return tmp;
}

void parseFileName(FILE* file) {
    char node_file_name_tmp[50], pl_file_name_tmp[50], scl_file_name_tmp[50];
    fscanf(file, " RowBasedPlacement : %s %s %s\n", node_file_name_tmp, pl_file_name_tmp, scl_file_name_tmp);
    fscanf(file, "MaxDisplacement :  %d\n", &MaxDisplacement);
    fclose(file);
        
//    strcpy(nodes_file_name, file_path);
//    strcat(nodes_file_name, node_file_name_tmp);
//    strcpy(coordinates_file_name, file_path);
//    strcat(coordinates_file_name, pl_file_name_tmp);
//    strcpy(rowInfos_file_name, file_path);
//    strcat(rowInfos_file_name, scl_file_name_tmp);
//
//    cout << nodes_file_name << endl;
//    cout << coordinates_file_name << endl;
//    cout << rowInfos_file_name << endl;
    
//    if(!(RowInfos_file = fopen(rowInfos_file_name, "r"))) {
//        cerr << " Can not open .scl file\n";
//    }
//    else if(!(nodes_file = fopen(nodes_file_name, "r"))) {
//        cerr << " Can not open .nodes file\n";
//    }
//    else if(!(coordinates_file = fopen(coordinates_file_name, "r"))) {
//        cerr << " Can not open .pl file\n";
//    }
}

void parseNodes(FILE* file) {
    char tmp[1000];
    fscanf(file, "%[^\n]\n", tmp);
    while (fscanf(file, "#%[^\n]\n", tmp));
    
    fscanf(file, "NumNodes : %d\n", &NumNodes);
    fscanf(file, "NumTerminals : %d\n", &NumTerminals);
    
    cout << NumNodes << " " << NumTerminals << endl;
    
    nodes = new nodeInfo[NumNodes]();
    
    for (int i = 0; i < NumNodes; i++) {
        if (i < NumNodes - NumTerminals) {
            fscanf(file, "%c%d %d %d\n", &fileTitle, &nodes[i].id, &nodes[i].width, &nodes[i].height);
            nodes[i].width /= siteWidth;
        } else {
            char tmp[10];       // u can ignore it
            fscanf(file, "%c%d %d %d %s\n", &fileTitle, &nodes[i].id, &nodes[i].width, &nodes[i].height, tmp);
            nodes[i].width /= siteWidth;
        }
        nodes[i].original_index = i;
    }
}

void parseCoordinates(FILE* file) {
    char tmp[1000];
    fscanf(file, "%[^\n]\n", tmp);
    for (int i = 0; i < NumNodes; i++) {
        char tmp[20];
        fscanf(file, "%c%d %f %f %[^\n]\n", &fileTitle, &nodes[i].id, &nodes[i].x_coordinate, &nodes[i].y_coordinate, tmp);
    }
}

void parseRowInfo(FILE* file) {
    char tmp[1000];
    fscanf(file, "%[^\n]\n", tmp);
    while (fscanf(file, "#%[^\n]\n", tmp));
    
    fscanf(file, "NumRows : %d\n", &NumRow);
    fscanf(file, "\n");
        
    rows = new rowInfo[NumRow]();
    
    for (int i = 0; i < NumRow; i++) {
        fscanf(file, "CoreRow Horizontal\n");
        fscanf(file, "Coordinate : %d\n", &rows[i].y_coordinate);
        fscanf(file, "Height : %d\n", &rowHeight);
        fscanf(file, "Sitewidth : %d\n", &siteWidth);
        fscanf(file, "Sitespacing : %d\n", &siteSpacing);
        fscanf(file, "Siteorient : %d\n", &siteOriented);
        fscanf(file, "Sitesymmetry : %d\n", &siteSymmetry);
        fscanf(file, "SubrowOrigin : %d NumSites : %d\n", &rowX, &rowWidth);
        fscanf(file, "End\n");
    }

    cout << "rowX before: " << rowX << endl;
    rowX /= siteWidth;
    cout << "rowX after: " << rowX << endl;
}

void writeSolution(FILE* file) {
    fprintf(file, "UCLA pl 1.0\n");
    fprintf(file, "\n");
    
    cout << "rowX before: " << rowX << endl;
    rowX *= siteWidth;
    cout << "rowX after: " << rowX << endl;
    
    for (int m = 0; m < NumNodes - NumTerminals; m++) {
        bool isWritten = false;
        for (int i = 0; i < subRows.size(); i++) {
            for (int j = 0; j < subRows[i].clusters.size(); j++) {
                for (int k = 0; k < subRows[i].clusters[j].node_collection.size(); k++) {
                    if (nodes[m].id == subRows[i].clusters[j].node_collection[k].id) {
                        fprintf(file, "%c%d %d %d : N\n", fileTitle, subRows[i].clusters[j].node_collection[k].id, subRows[i].clusters[j].node_collection[k].new_x, subRows[i].clusters[j].node_collection[k].new_y);
                        if (subRows[i].clusters[j].node_collection[k].new_x < rowX) {
                            cout << "smaller" << endl;
                        } else if (subRows[i].clusters[j].node_collection[k].new_x + subRows[i].clusters[j].node_collection[k].width > rowX + rowWidth * siteWidth) {
                            cout << "index: " << i << endl;
                            cout << "new_x: " << subRows[i].clusters[j].node_collection[k].new_x << endl;
                            cout << "width: " << subRows[i].clusters[j].node_collection[k].width * siteWidth << endl;
                            cout << "x: " << subRows[i].x_coordinate << endl;
                            cout << "wid: " << subRows[i].width << endl;
                        }
                        subRows[i].clusters[j].node_collection.erase(subRows[i].clusters[j].node_collection.begin() + k);
                        isWritten = true;
                    }
                    if (isWritten) break;
                }
                if (isWritten) break;
            }
            if (isWritten) break;
        }
    }
    
    for (int i = NumNodes - NumTerminals; i < NumNodes; i++) {
        fprintf(file, "%c%d %d %d : N /FIXED\n", fileTitle, nodes[i].id, (int)(nodes[i].x_coordinate), (int)(nodes[i].y_coordinate));
    }
}

/* ------------ Abacus ------------ */
void quicksort(int L, int R) {
    int i, j, mid;
    float piv;
    i = L;
    j = R;
    mid = L + (R - L) / 2;
    piv = node_vec[mid].x_coordinate;
    
    while (i < R || j > L) {
        while (node_vec[i].x_coordinate < piv) i++;
        while (node_vec[j].x_coordinate > piv) j--;

        if (i <= j) {
            swap(node_vec[i], node_vec[j]);
            i++;
            j--;
        } else {
            if (i < R) quicksort(i, R);
            if (j > L) quicksort(L, j);
            return;
        }
    }
}

vector<nodeInfo> createVector(nodeInfo* array) {
    vector<nodeInfo> vec;
    for (int i = 0; i < NumNodes - NumTerminals; i++) {
        nodeInfo tmp;
        tmp.id = nodes[i].id;
        tmp.width = nodes[i].width;
        tmp.x_coordinate = (int)(nodes[i].x_coordinate);
        tmp.y_coordinate = (int)(nodes[i].y_coordinate);
        tmp.original_index = i;
        vec.push_back(tmp);
    }
    return vec;
}

// Put all terminals to row, and need to notice that terminals cannot be moved
void initialTerminalToRows() {
    for (int i = NumNodes - NumTerminals; i < NumNodes; i++) {
        for (int j = 0; j < NumRow; j++) {
            if (nodes[i].y_coordinate <= rows[j].y_coordinate && nodes[i].y_coordinate + nodes[i].height > rows[j].y_coordinate) {
                rows[j].terminals.push_back(terminal(nodes[i].id, nodes[i].width, nodes[i].x_coordinate));
            }
        }
    }
}

void sortTerminal(int index) {
    if (rows[index].terminals.size() == 1) return;
    
    // Using Bubble sort to arrange x_coordinate value in terminals
    for (int i = 0; i < rows[index].terminals.size(); i++) {
        for (int j = i + 1; j < rows[index].terminals.size(); j++) {
            if (rows[index].terminals[i].x_coordinate > rows[index].terminals[j].x_coordinate)
                swap(rows[index].terminals[i], rows[index].terminals[j]);
        }
    }
}

vector<subRow> createSubRow() {
    vector<subRow> result;
    for (int i = 0; i < NumRow; i++) {
        if (!rows[i].terminals.empty()) {
            sortTerminal(i);
            int start_x = rowX;
            for (int k = 0; k < rows[i].terminals.size(); k++) {
                if (rows[i].terminals[k].x_coordinate - start_x != 0) {
                    subRow tmp;
                    tmp.x_coordinate = start_x;
                    tmp.y_coordinate = rows[i].y_coordinate;
                    tmp.width = rows[i].terminals[k].x_coordinate - start_x;
                    result.push_back(tmp);
                }
                start_x = rows[i].terminals[k].x_coordinate + rows[i].terminals[k].width;
            }
            
            unsigned long int last = rows[i].terminals.size() - 1;
            if (rowWidth + rowX - rows[i].terminals[last].x_coordinate - rows[i].terminals[last].width > 0) { //
                subRow tmp;
                tmp.x_coordinate = rows[i].terminals[last].x_coordinate + rows[i].terminals[last].width;
                tmp.y_coordinate = rows[i].y_coordinate;
                tmp.width = rowWidth + rowX - rows[i].terminals[last].x_coordinate - rows[i].terminals[last].width;
                result.push_back(tmp);
            }
        } else {
            subRow tmp;
            tmp.x_coordinate = rowX;
            tmp.y_coordinate = rows[i].y_coordinate;
            tmp.width = rowWidth;
            result.push_back(tmp);
        }
    }
    return result;
}

void insertNodeToSubRow(int node_index, int row_index) {
    subRows[row_index].tmp = node_vec[node_index];
    
    if (node_vec[node_index].x_coordinate + node_vec[node_index].width < subRows[row_index].tmp.x_coordinate + subRows[row_index].tmp.width) {
        subRows[row_index].tmp.x_coordinate = subRows[row_index].width + subRows[row_index].x_coordinate - node_vec[node_index].width;
    } else if (node_vec[node_index].x_coordinate < subRows[row_index].x_coordinate) {
        subRows[row_index].tmp.x_coordinate = subRows[row_index].x_coordinate;
    } else {
    }
}

void Collapse(int row_index) {
    unsigned long int last_index = subRows[row_index].clusters.size() - 1;
    if (subRows[row_index].clusters[last_index].x_start < subRows[row_index].x_coordinate)
        subRows[row_index].clusters[last_index].x_start = subRows[row_index].x_coordinate;

    if (subRows[row_index].clusters[last_index].x_start + subRows[row_index].clusters[last_index].total_width > subRows[row_index].x_coordinate + subRows[row_index].width)
        subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].x_start = subRows[row_index].x_coordinate;
        
    if (subRows[row_index].clusters.size() > 1 && subRows[row_index].clusters[last_index].x_start < subRows[row_index].clusters[last_index - 1].x_start + subRows[row_index].clusters[last_index - 1].total_width) {
        
        subRows[row_index].clusters[last_index - 1].total_width += subRows[row_index].clusters[last_index].total_width;
        
        for (int i = 0; i < subRows[row_index].clusters[last_index].node_collection.size(); i++)
            subRows[row_index].clusters[last_index - 1].node_collection.push_back(subRows[row_index].clusters[last_index].node_collection[i]);
        
        subRows[row_index].clusters.erase(subRows[row_index].clusters.begin() + subRows[row_index].clusters.size() - 1);
        
        Collapse(row_index);
    }
}

bool isFitted(int row_index, int width) {
    if (subRows[row_index].clusters.empty()) {
        if (width <= subRows[row_index].width)
            return true;
    } else {
        int total_width = 0;
        
        for (int i = 0; i < subRows[row_index].clusters.size(); i++) {
            total_width += subRows[row_index].clusters[i].total_width;
        }
                
        if (subRows[row_index].width - total_width >= width)
            return true;
    }
    
    return false;
}

double displacement(int node_id, int new_x, int new_y) {
    double tmp = 0.0;
    int original_x = nodes[node_id].x_coordinate / siteWidth;
    int original_y = nodes[node_id].y_coordinate;
    int delta_x = (original_x - new_x > 0) ? (original_x - new_x) : (new_x - original_x);
    int delta_y = (original_y - new_y > 0) ? (original_y - new_y) : (new_y - original_y);
    int pow_x = delta_x * delta_x;
    int pow_y = delta_y * delta_y;

//    cout << pow_x << " " << pow_y << endl;
    
    if (pow_x < 0 || pow_y < 0) return double(10000000);
    
    tmp = sqrt(pow_x + pow_y);
    return tmp;
}

double updateClusterCost(int row_index, int index) {
    double cost = 0.0;
    int start_x = subRows[row_index].clusters[index].x_start;
    
    for (int i = 0; i < subRows[row_index].clusters[index].node_collection.size(); i++) {
        cost += displacement(subRows[row_index].clusters[index].node_collection[i].id, start_x, subRows[row_index].y_coordinate);
        start_x += subRows[row_index].clusters[index].node_collection[i].width;
    }
    
    
    return cost;
}

void PlaceRow(int row_index) {
    if (!subRows[row_index].clusters.empty()) {
        cluster c = subRows[row_index].clusters[subRows[row_index].clusters.size() - 1];
        if (subRows[row_index].tmp.x_coordinate >= c.x_start + c.total_width) {
            if (subRows[row_index].tmp.x_coordinate + subRows[row_index].tmp.width > subRows[row_index].x_coordinate + subRows[row_index].width) {
                cluster new_c;
                new_c.node_collection.push_back(subRows[row_index].tmp);
                new_c.x_start = subRows[row_index].tmp.x_coordinate;
                new_c.total_width = subRows[row_index].tmp.width;
                new_c.total_cost = displacement(subRows[row_index].tmp.id, new_c.x_start, subRows[row_index].y_coordinate);
                
                if (new_c.x_start + new_c.total_width > subRows[row_index].x_coordinate + subRows[row_index].width)
                    new_c.x_start = subRows[row_index].x_coordinate + subRows[row_index].width - new_c.total_width;
                subRows[row_index].clusters.push_back(new_c);
                
                subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_x = new_c.x_start;
                subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_y = subRows[row_index].y_coordinate;
                Collapse(row_index);
            } else {
                cluster new_c;
                new_c.node_collection.push_back(subRows[row_index].tmp);
                new_c.x_start = subRows[row_index].tmp.x_coordinate;
                new_c.total_width = subRows[row_index].tmp.width;
                new_c.total_cost = displacement(subRows[row_index].tmp.id, new_c.x_start, subRows[row_index].y_coordinate);
                
                subRows[row_index].clusters.push_back(new_c);
                
                subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_x = new_c.x_start;
                subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_y = subRows[row_index].y_coordinate;
                return;
            }
        } else {
            unsigned long int last_index = subRows[row_index].clusters.size() - 1;
            subRows[row_index].clusters[last_index].node_collection.push_back(subRows[row_index].tmp);
            subRows[row_index].clusters[last_index].total_cost += displacement(subRows[row_index].tmp.id, subRows[row_index].clusters[last_index].x_start + subRows[row_index].clusters[last_index].total_width, subRows[row_index].y_coordinate);
            subRows[row_index].clusters[last_index].total_width += subRows[row_index].tmp.width;
            Collapse(row_index);
        }
    } else {
        cluster new_c;
        cout << row_index << endl;
        cout << subRows[row_index].tmp.original_index << endl;
        cout << subRows[row_index].tmp.x_coordinate << " " << subRows[row_index].tmp.width << " " << subRows[row_index].y_coordinate << endl;
        new_c.node_collection.push_back(subRows[row_index].tmp);
        cout << "a" << endl;
        new_c.x_start = subRows[row_index].tmp.x_coordinate;
        cout << "b" << endl;
        new_c.total_width = subRows[row_index].tmp.width;
        cout << "c" << endl;
        new_c.total_cost = displacement(subRows[row_index].tmp.original_index, new_c.x_start, subRows[row_index].y_coordinate);
        cout << new_c.total_cost << endl;
        if (new_c.x_start + new_c.total_width > subRows[row_index].x_coordinate + subRows[row_index].width)
            new_c.x_start = subRows[row_index].x_coordinate + subRows[row_index].width - new_c.total_width;
        
        if (new_c.x_start < subRows[row_index].x_coordinate)
            new_c.x_start = subRows[row_index].x_coordinate;
        
        subRows[row_index].clusters.push_back(new_c);
        
        subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_x = new_c.x_start;
        subRows[row_index].clusters[subRows[row_index].clusters.size() - 1].node_collection[0].new_y = subRows[row_index].y_coordinate;
        return;
    }
    
    int row_y = subRows[row_index].y_coordinate;
        
    for (int i = 0; i < subRows[row_index].clusters.size(); i++) {
        int x = subRows[row_index].clusters[i].x_start;
        subRows[row_index].clusters[i].total_cost = updateClusterCost(row_index, i);
        for (int j = 0; j < subRows[row_index].clusters[i].node_collection.size(); j++) {
            subRows[row_index].clusters[i].node_collection[j].new_x = x;
            subRows[row_index].clusters[i].node_collection[j].new_y = row_y;
            x += subRows[row_index].clusters[i].node_collection[j].width;
        }
    }
}

void printNodesCount() {
    int total_count = 0;
    for (int i = 0; i < subRows.size(); i++) {
        int count = 0;
        for (int j = 0; j < subRows[i].clusters.size(); j++) {
            for (int k = 0; k < subRows[i].clusters[j].node_collection.size(); k++) {
                count++;
                total_count++;
            }
        }
    }
    cout << "total_count: " << total_count << endl;
}

double certainRowCalculation(int row_index) {
    double cost = 0.0;
    
    for (int i = 0; i < subRows[row_index].clusters.size(); i++) {
        for (int j = 0; j < subRows[row_index].clusters[i].node_collection.size(); j++) {
            cost += displacement(subRows[row_index].clusters[i].node_collection[j].id, subRows[row_index].clusters[i].node_collection[j].new_x, subRows[row_index].clusters[i].node_collection[j].new_y);
        }
    }
    
    return cost;
}

int findClose(int node_index) {
    int node_y = node_vec[node_index].y_coordinate;
    int mid = 0;
    int l = 0;
    int r = NumRow - 1;
        
    while (l <= r) {
        mid = l + (r - l) / 2;
        if (e_rows[mid].y == node_y) {
            return mid;
        } else if (e_rows[mid].y < node_y) {
            l = mid + 1;
        } else {
            r = mid - 1;
        }
    }
        
    return mid;
}

void finalPositionUpdate() {
    for (int row_index = 0; row_index < subRows.size(); row_index++) {
        for (int i = 0; i < subRows[row_index].clusters.size(); i++) {
            for (int j = 0; j < subRows[row_index].clusters[i].node_collection.size(); j++) {
                subRows[row_index].clusters[i].node_collection[j].new_x *= siteWidth;
            }
        }
    }
}

int tmp_count = 0;

void Abacus() {
    node_vec = createVector(nodes);
    quicksort(0, NumNodes - NumTerminals - 1); // Sorting all movable nodes by quicksort nlog(n)
    initialTerminalToRows(); // put all terminals in every row and sort them with increment order
    subRows = createSubRow(); // In original Abacus algo, it will insert node to row, now we insert it to subrow and follow same algo to find the minimum cost method.
        
    e_rows = new efficient_row[NumRow];
    
    for (int i = 0; i < NumRow; i++)
        e_rows[i].y = rows[i].y_coordinate;
    
    for (int i = 0; i < subRows.size(); i++) {
        for (int j = 0; j < NumRow; j++) {
            if (e_rows[j].y == subRows[i].y_coordinate) {
                e_rows[j].row_index.push_back(i);
                break;
            }
        }
    }
        
    int NumMovableNodes = NumNodes - NumTerminals;
    for (int i = 0; i < 8; i++) {
        int best_cost = INT_MAX;
        int best_row = -1;
        
        cout << i << "--------------" << endl;
        
        int start_index = findClose(i);
         
//        cout << "start_index: " << start_index << endl;
                        
        for (int l = start_index; l < NumRow; l++) {
            for (int k = 0; k < e_rows[l].row_index.size(); k++) {
                int j = e_rows[l].row_index[k];
//                cout << j << endl;
                subRow tmp_row = subRows[j];
                insertNodeToSubRow(i, j);
//                cout << "search" << endl;
                if (isFitted(j, node_vec[i].width)) {
//                    cout << "det1" << endl;
                    PlaceRow(j); // mode is trail
//                    cout << "det2" << endl;
                    double new_cost = certainRowCalculation(j);
//                    cout << "first_new_cost: " << new_cost << endl;
                    if (new_cost < best_cost && new_cost != -1) {
                        best_cost = new_cost;
                        best_row = j;
                    }
                }
                
                subRows[j] = tmp_row;
            }
            
            if (best_cost != INT_MAX) break;
        }
                
        for (int m = start_index - 1; m >= 0; m--) {
            bool detect_smaller = false;
            for (int n = 0; n < e_rows[m].row_index.size(); n++) {
                int j = e_rows[m].row_index[n];
                subRow tmp_row = subRows[j];
                insertNodeToSubRow(i, j);
                if (isFitted(j, node_vec[i].width)) {
                    PlaceRow(j); // mode is trail
                    double new_cost = certainRowCalculation(j);
//                    cout << "second_new_cost: " << new_cost << endl;
                    if (new_cost < best_cost) {
                        best_cost = new_cost;
                        best_row = j;
                    } else {
                        detect_smaller = true;
                    }
                }
                
                subRows[j] = tmp_row;
                if (detect_smaller) break;
            }
            if (detect_smaller) break;
        }
        
        if (best_row != -1) {
//            cout << "valid: " << best_row << endl;
            current_best_cost -= certainRowCalculation(best_row);
            insertNodeToSubRow(i, best_row);
            PlaceRow(best_row); // mode is final
            current_best_cost += certainRowCalculation(best_row);
            tmp_count++;
        } else {
            cout << "cannot find any solution *****************" << endl;
            return;
        }
    }
}

int main(int argc, char * argv[]) {
//    if(!check_input(argc, argv))
//        return 0;
    
    // Open files and read data
    filename = fopen("ibm01-cu85.aux", "r");
    nodes_file = fopen("ibm01.nodes", "r");
    coordinates_file = fopen("ibm01-cu85.gp.pl", "r");
    RowInfos_file = fopen("ibm01-cu85.scl", "r");
    solution_file = fopen("adaptec1.result", "w");

    parseFileName(filename);
    parseRowInfo(RowInfos_file);
    parseNodes(nodes_file);
    parseCoordinates(coordinates_file);
    fclose(filename);
    fclose(RowInfos_file);
    fclose(nodes_file);
    fclose(coordinates_file);
    
    double wall0 = get_wall_time();
    double cpu0  = get_cpu_time();
    
    cout << "Start Abacus!!!" << endl;
    
//    cout << "NumNodes: " << NumNodes << endl;
//    cout << "NumTerminals: " << NumTerminals << endl;
//    cout << "NumRow: " << NumRow << endl;
//    cout << "rowX: " << rowX << endl;
//    cout << "rowHeight: " << rowHeight << endl;
//    cout << "rowWidth: " << rowWidth << endl;
//    cout << "siteWidth: " << siteWidth << endl;
//    cout << "siteOriented: " << siteOriented << endl;
//    cout << "siteSpacing: " << siteSpacing << endl;
//    cout << "siteSymmetry: " << siteSymmetry << endl;
    
    Abacus();
    finalPositionUpdate();
        
    double wall1 = get_wall_time();
    double cpu1  = get_cpu_time();
    
    cout << "total displacement: " << current_best_cost << endl;
    cout << "\nTime(s) : Abacus : Wall Time : " << wall1 - wall0 << ", CPU Time : " << cpu1  - cpu0 << std::endl;
    cout << "tmp_count: " << tmp_count << endl;
    printNodesCount();
    
    // Write final solution in file
    writeSolution(solution_file);
    fclose(solution_file);
    
    return 0;
}
