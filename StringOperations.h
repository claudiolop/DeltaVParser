#ifndef FILEUTILS_H
#define FILEUTILS_H
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <chrono>


//#include <ctime>
//#include <conio.h> 
//#include <sstream>

using namespace std; // Import entire std namespace.

struct TableConfig{
	string first_level_action;
	string first_level_table;
	string data_action;
	string data_table;
	vector<string> headers;	//Name of the column in the order that needs to appear in the OutCsv
};


struct thousands_separator : numpunct<char> {
    char do_thousands_sep() const { return '.'; } // Use dot as separator
    string do_grouping() const { return "\3"; } // Group every 3 digits
};

void deleteFilesInFolder(const string& folderName);
string swapChars(const string& str);
map<string, TableConfig> loadTableConfig(string file_name);
map<string, TableConfig> loadTypeConfig(string file_name);
void progressBar(int current, int total);
string loadWordList(string file_name);
void createOutTable(const vector<string>& headers,string file_name);
void insertHeader(TableConfig& table,string new_header);
map<string, TableConfig> loadOutputConfig(string file_name);
vector<string> splitString(const string& str,int qoute_count, int& comment_count);
void updateProgress(uint64_t current_line,uint64_t total_lines, double update_rate,const chrono::steady_clock::time_point& start_time, chrono::steady_clock::time_point& last_update);
string escapeCSV(const string& data);
uint64_t countLines(const string& filename);
chrono::steady_clock::time_point printCurrentTime(chrono::steady_clock::time_point start_time);
string trim(const string &str);
void playEndSound();
vector<vector<string>> readCSVFile(string file_name);
#endif