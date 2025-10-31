#ifndef FILEUTILS_H
#define FILEUTILS_H
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <chrono>
#include <fstream>

using namespace std; // Import entire std namespace.


const string type_config_file="TypeConfig.csv";
const string table_config_file="TableConfig.csv";

struct TableConfig{
	int header_count;
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

//Move to DeltaV Object...
vector<string> splitString(const string& str,int qoute_count, int& comment_count);

//File Handling
void deleteFilesInFolder(const string& folderName);
string extractFileName(const string& fullPath);
string joinPath(const vector<string>& path);

//String Handling
string trim(const string &str);
string escapeCSV(const string& data);
string formatNumberWithSeparators(long long number);

//Configuration and Output 
vector<vector<string>> readCSVFile(string file_name);
map<string, TableConfig> loadTypeConfig();
map<string, TableConfig> loadTableConfig();
void updateConfig(string& type,vector<string>& headers,int& header_count);
string loadWordList(string file_name);
void createOutTable(const vector<string>& headers,string file_name,string folder_name);
void createOutTables(const map<string, TableConfig>& table_config,const map<string, TableConfig>& type_config,const string& output_folder);

//Log and Trace
string getTimestamp();
void initLogFiles();
void logMessage(string severity,string message);
void logTraceLine(long long lineNumber, const string& line,int level);

//Update Stauts
long long countLines(const string& filename);
void updateProgress(long long current_line,long long total_lines, double update_rate,const chrono::steady_clock::time_point& start_time, chrono::steady_clock::time_point& last_update);
chrono::steady_clock::time_point printCurrentTime(chrono::steady_clock::time_point start_time);
string GetFileVersion();
#endif