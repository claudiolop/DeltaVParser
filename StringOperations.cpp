#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <conio.h> // For _kbhit and _getch
#include <vector>
#include "StringOperations.h"
#include <map>
#include <fstream>
#include <sstream>
#include <locale>
#include <iomanip>
using namespace std; // Import entire std namespace.

void deleteFilesInFolder(const string& folderName){
	WIN32_FIND_DATA fileData;
    HANDLE hFind;
	char buffer[MAX_PATH];
    DWORD length = GetModuleFileName(NULL, buffer, MAX_PATH);
	string folderPath;
	string searchPath = string(buffer, length);
	folderPath=searchPath.substr(0,searchPath.rfind('\\')+1)+folderName+"\\";
	searchPath=folderPath+'\\'+"*.csv";
	hFind = FindFirstFile(searchPath.c_str(), &fileData);
    if (hFind == INVALID_HANDLE_VALUE) return;
	cout<<"Deleting old files...\r";
    do {
        if (strcmp(fileData.cFileName, ".") == 0 || strcmp(fileData.cFileName, "..") == 0) continue;
        string fullPath = folderPath + fileData.cFileName;
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) DeleteFile(fullPath.c_str());
    } while (FindNextFile(hFind, &fileData));

    FindClose(hFind);
}

string swapChars(const string& str){
	string result;
	int int_char;
	for (int i=0;i<str.size();i++){
		int_char=static_cast<unsigned int>(static_cast<unsigned char>(str[i]));
		if (int_char!=0 and int_char!=255 and int_char!=254 and str[i]!='\r') result+=str[i];
	}
	return result;
}

void createOutTable(const vector<string>& headers,string file_name){
	string text;
	
	string file_path="OutputTables/"+file_name+".csv";
	ifstream file_check(file_path);
    if (file_check.good()) return;
	ofstream file(file_path, ios::app | ios::binary);     // Open Source Filesda		
	for (const auto& header : headers) text+=escapeCSV(header)+",";
	text.pop_back();
	file<<text<<"\n";
	file.close();
}

string loadWordList(string file_name){
	string loadWordList;
	string line;
	ifstream file("Config/"+file_name);
	if (!file.is_open()) return loadWordList;
    while (getline(file, line)) loadWordList+=" "+line;
    loadWordList+=" ";
	file.close();
    return loadWordList;
	
}

vector<vector<string>> readCSVFile(string file_name){
    string line;
	string current;
	vector<vector<string>> result;
	vector<string> row;
		
	ifstream file(file_name);	
	if (!file.is_open()){
		cerr<<"\nCan't open the file: "<<file_name;
		exit (400);
	}
	getline(file, line); //Skips the header line
	while (getline(file, line)) {
		row.clear();
		current="";
		for (char c : line){
			if (c==','){
				if (current=="") current=" ";
				row.push_back(current);
				current="";
			}
			else{
				current+=c;
			}
		}
		for (int i=row.size()-1;i>=0;i--){
			if (row[i]==" "){
				row.pop_back();
			}else{
				break;
			}
		}
		result.push_back(row);
	}
	file.close();
	return result;
}

map<string, TableConfig> loadTypeConfig(string file_name){
	map<string, TableConfig> config_info;
	
	
	string first_level_action;
	string first_level_table;
	string data_action;
	string data_table;
	vector<string> headers;
    string type;
    
	vector<vector<string>> csv_data =readCSVFile("Config/"+file_name+".csv");

    int col_count;
    for (const auto& row : csv_data){
		col_count=0;
		for (const auto& cell : row){
			col_count++;
			switch (col_count){
			case 1:	type=cell; break;
			case 2:	config_info[type].first_level_action=cell; break;
			case 3:	config_info[type].first_level_table=cell; break;
			case 4:	config_info[type].data_action=cell; break;
			case 5:	config_info[type].data_table=cell; break;
			default: config_info[type].headers.push_back(cell);
			}
		}
	}
	
	
	return config_info;
}

map<string, TableConfig> loadTableConfig(string file_name){
	map<string, TableConfig> config_info;
	string first_level_action;
	string first_level_table;
	string data_action;
	string data_table;
	vector<string> headers;
    string type;
    
	vector<vector<string>> csv_data =readCSVFile("Config/"+file_name+".csv");
    
    int col_count=0;
    for (const auto& row : csv_data){
		col_count=0;
		for (const auto& cell : row){
			col_count++;
			switch (col_count){
			case 1:	type=cell; break;
			default: config_info[type].headers.push_back(cell);
			}
		}
	}
	return config_info;
}

vector<string> splitString(const string& str,int qoute_count) {
	vector<string> result;
    string new_line="";			//Whatever is to the left of { or }
    string delimiter="";
	for (char c : str) {
        if (c=='"') qoute_count++;
		if ((qoute_count % 2 ==0) and ((c == '{') or (c == '}'))) {		//if I find a { or } when I'm not between qoutes 	
			if (!new_line.empty()) result.push_back(new_line);
			delimiter=c;
			result.push_back(delimiter);										//and I push the { or } as another line	
			new_line.clear();											//clear new_line to continue with the following part of the string
		} else {
			if (c=='{' or c=='}')new_line += ' ';						//If I"m between qoutes and I find a { or } I have to add a space to make sure is not interpreted as a delimiter
			new_line += c;
		}
	}
	if (!new_line.empty()) result.push_back(new_line); 											//When I'm done, if there is somenthing left,is pushed.

	return result;
}

void updateProgress(uint64_t current_line,uint64_t total_lines, double update_rate,const chrono::steady_clock::time_point& start_time, chrono::steady_clock::time_point& last_update) {
	auto now = chrono::steady_clock::now();
    double elapsed_since_last = chrono::duration<double>(now - last_update).count(); 	
	if (elapsed_since_last < update_rate and total_lines-current_line>100) return;
	string eta_text;
	int barWidth = 40;
    double progress = static_cast<double>(current_line) / total_lines;
    int pos = barWidth * progress;
    double elapsed = chrono::duration<double>(now - start_time).count();
    double speed = current_line / elapsed;
    double remaining = total_lines - current_line;
    double eta = remaining / speed;    
    int eta_min = eta / 60;
    int eta_sec = (int)eta % 60;
    
    eta_text=to_string(eta_min)+"m "+to_string(eta_sec)+"s";
    if (progress<0.02) eta_text="?m ??s";
	cout <<"Procesing file: " << "ETA:"<<eta_text<<" Line: "<<current_line<<" out of "<<total_lines;
    cout << " [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    cout << "] " << fixed << setprecision(1) << (progress * 100.0) << " %  \r";
    cout<<flush;
    last_update = now;
}

string escapeCSV(const string& data) {
    if (data=="") return data;
	if (data.find(',') == string::npos && data.find('"') == string::npos) {return data;} // No commas or quotes, no escaping needed
    
    string escaped = "\"";
    for (char c : data) {
        if (c == '"') escaped += "\"\""; // Escape quotes by doubling
        else escaped += c;
    }
    escaped += "\"";
    return escaped;
}

uint64_t countLines(const std::string& filename) {
	std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "!" << std::endl;
        return 0;
    }
    cout.imbue(std::locale(cout.getloc(), new thousands_separator));
	uint64_t line_count = 0;
    std::string line;
	
    char frames[] = {'|', '/', '-', '\\'};
	int frame_count=0;
	
	while (std::getline(file, line)) {
		line_count++;
		if (line_count % 20000==0){
			cout<<"\rCounting lines: "<<frames[frame_count]<<flush;
			frame_count++;
			if (frame_count>=sizeof(frames)) frame_count=0;
		}	
	}
    file.close();
	cout<<"\r";
    return line_count;
}

chrono::steady_clock::time_point printCurrentTime(chrono::steady_clock::time_point start_time) {
	auto now = time(nullptr);
	if (start_time == std::chrono::steady_clock::time_point{}){
		cout << put_time(localtime(&now), "%H:%M:%S")<<"\n";
		return chrono::steady_clock::now();
	}else{
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(chrono::steady_clock::now()-start_time);
		long long seconds =  duration.count();
    	long long minutes = seconds / 60;
    	seconds %= 60;
		cout << put_time(localtime(&now), "%H:%M:%S")<<"\n";
		cout<<"Total Time: "<< minutes << " min " << seconds << " sec\n";
		return chrono::steady_clock::now();
	}
}

string trim(const string &str) {
    size_t first = str.find_first_not_of(" \t\r\n\0"); // Find first non-whitespace
    size_t last = str.find_last_not_of(" \t\r\n\0"); // Find last non-whitespace
    if (first == string::npos) {return "";} // String is all whitespace
	return str.substr(first, (last - first + 1));
}

string RemoveComments(bool& MultiLine, string str){
	size_t first = str.find("/*"); 								// Get the position of the opening
	if (!MultiLine and first == string::npos) {return str;} 	// If not multiline and opening not foud, return.
	if (MultiLine){first=0;}									// If multiline, start from first (
	size_t last = str.find("*/"); 		// Get the position of the closing
	if (last == string::npos) {						// If not found, 	
			MultiLine=true;								// Is multiline
			last= str.size();}							// and end with last
		else
		{
			MultiLine=false;
			last=last+2;
		}
	str=str.substr(0,first)+str.substr(last,str.size());
	return str;
}

void insertHeader(TableConfig& table,string new_header){
	bool header_found=false;
	for (const auto header : table.headers){		//Search if the header exists.
		if (header==new_header){
			header_found=true;
			break;
		}
	}
	if (!header_found) table.headers.push_back(new_header);
}

void playEndSound() {
	Beep(1245,300);
	Sleep(100);
	Beep(1245,300);
}

