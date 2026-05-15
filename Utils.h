#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <chrono>
#include <fstream>
#include <filesystem>

const std::string TYPE_CONFIG_FILE  = "TypeConfig.csv";
const std::string TABLE_CONFIG_FILE = "TableConfig.csv";

struct TableConfig {
    int header_count;
    std::string first_level_action;
    std::string first_level_table;
    std::string data_action;
    std::string data_table;
    std::vector<std::string> headers;
};

struct thousands_separator : std::numpunct<char> {
    char do_thousands_sep() const { return '.'; }
    std::string do_grouping() const { return "\3"; }
};

// Move to DeltaV Object...
std::vector<std::string> splitString(const std::string& str, int quote_count, int& comment_count);

// File Handling
void deleteFilesInFolder(const std::string& folder_name);
std::string extractFileName(const std::string& full_path);
std::string joinPath(const std::vector<std::string>& path);

// String Handling
std::string trim(const std::string& str);
std::string escapeCSV(const std::string& data);
std::string formatNumberWithSeparators(long long number);

// Configuration and Output
std::vector<std::vector<std::string>> readCSVFile(std::string file_name);
std::map<std::string, TableConfig> loadTypeConfig();
std::map<std::string, TableConfig> loadTableConfig();
void updateConfig(const std::string& type, std::vector<std::string>& headers, int& header_count);
std::string loadWordList(const std::string file_name);
void createOutTable(const std::vector<std::string>& headers, std::string file_name, std::string folder_name);
void createOutTables(const std::map<std::string, TableConfig>& table_config,
                     const std::map<std::string, TableConfig>& type_config,
                     const std::string& output_folder);

// Log and Trace
std::string getTimestamp();
void initLogFiles();
void logMessage(std::string severity, std::string message);
void logTraceLine(long long line_number, const std::string& line, int level);

// Update Status
long long countLines(const std::string& filename);
void updateProgress(long long current_line, long long total_lines, double update_rate,
                    const std::chrono::steady_clock::time_point& start_time,
                    std::chrono::steady_clock::time_point& last_update,
                    bool silent_mode);
std::chrono::steady_clock::time_point printCurrentTime(std::chrono::steady_clock::time_point start_time);

#endif