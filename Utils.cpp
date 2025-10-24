#include "Utils.h"
#include <windows.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <fstream>

using namespace std; // Import entire std namespace.

ofstream logFile;
ofstream traceFile;


vector<string> splitString(const string& str,int qoute_count, int& comment_count) {
	vector<string> result;
    string new_line="";			//Whatever is to the left of { or }
    string delimiter="";
	int char_count=-1;
	for (char c : str) {
        char_count++;
		if (c=='"' and comment_count%2==0) qoute_count++;
		if (qoute_count % 2 ==0){
			if (c=='/' and char_count<=str.size() and comment_count%2==0) if (str[char_count+1]=='*') comment_count++;
			if (c=='/' and comment_count%2!=0) if (str[char_count-1]=='*'){
				comment_count--;
				continue;
			} 
			if (comment_count%2!=0) continue;	
			if ((c == '{') or (c == '}')) {		//if I find a { or } when I'm not between qoutes 	
				if (!new_line.empty()) result.push_back(new_line);
				delimiter=c;
				result.push_back(delimiter);										//and I push the { or } as another line	
				new_line.clear();
				continue;
			}
			new_line += c;										//clear new_line to continue with the following part of the string
		} else {
			if (c=='{' or c=='}')new_line += ' ';						//If I"m between qoutes and I find a { or } I have to add a space to make sure is not interpreted as a delimiter
			new_line += c;
		}
	}
	if (!new_line.empty()) result.push_back(new_line); 											//When I'm done, if there is somenthing left,is pushed.

	return result;
}

void deleteFilesInFolder(const string& folderName){
	logMessage("EVENT","Deleting old output files.");
	WIN32_FIND_DATA fileData;
    HANDLE hFind;
	char buffer[MAX_PATH];
    DWORD length = GetModuleFileName(NULL, buffer, MAX_PATH);
	string folderPath;
	string searchPath = string(buffer, length);
	folderPath=searchPath.substr(0,searchPath.rfind('\\')+1)+folderName;
	searchPath=folderPath+'\\'+"*.csv";
	hFind = FindFirstFile(searchPath.c_str(), &fileData);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(fileData.cFileName, ".") == 0 || strcmp(fileData.cFileName, "..") == 0) continue;
        string fullPath = folderPath + fileData.cFileName;
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			DeleteFile(fullPath.c_str());
			logMessage("EVENT","Deleting: " + fullPath);
		} 
    } while (FindNextFile(hFind, &fileData));
    FindClose(hFind);
}

string trim(const string &str) {
    size_t first = str.find_first_not_of(" \t\r\n\0"); // Find first non-whitespace
    size_t last = str.find_last_not_of(" \t\r\n\0"); // Find last non-whitespace
    if (first == string::npos) {return "";} // String is all whitespace
	return str.substr(first, (last - first + 1));
}

string skipNull(const string& str){
	string result;
	int int_char;
	for (int i=0;i<str.size();i++){
		int_char=static_cast<unsigned int>(static_cast<unsigned char>(str[i]));
		if (int_char!=0 and int_char!=255 and int_char!=254 and str[i]!='\r') result+=str[i];
	}
	return result;
}

string escapeCSV(const string& data) {
    if (data=="") return data;
	if (data.find(',') == string::npos && data.find('"') == string::npos) return data; // No commas or quotes, no escaping needed
    
    string escaped = "\"";
    for (char c : data) {
        if (c == '"') escaped += "\"\""; // Escape quotes by doubling
        else escaped += c;
    }
    escaped += "\"";
    return escaped;
}

vector<vector<string>> readCSVFile(string file_name){
    string line;
	string current;
	vector<vector<string>> result;
	vector<string> row;
	logMessage("EVENT","Reading: "+file_name);
	ifstream file(file_name);	
	if (!file.is_open()){
		logMessage(ERROR,"Can't open the file: "+file_name);
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

map<string, TableConfig> loadTypeConfig(){
	map<string, TableConfig> config_info;
	string first_level_action;
	string first_level_table;
	string data_action;
	string data_table;
	vector<string> headers;
    string type;
    
	vector<vector<string>> csv_data =readCSVFile("Config/"+type_config_file);

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

map<string, TableConfig> loadTableConfig(){
	map<string, TableConfig> config_info;
	string first_level_action;
	string first_level_table;
	string data_action;
	string data_table;
	vector<string> headers;
    string type;
    
	vector<vector<string>> csv_data =readCSVFile("Config/"+table_config_file);
    
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


void updateConfig(string& type,vector<string>& headers){
	bool file_exist=false;
	string text;
	
	//Update the Type_Config Table
	string file_path="Config/"+type_config_file;
	ifstream file_check(file_path);
    if (file_check.good()) file_exist=true;
	file_check.close();
	ofstream file(file_path, ios::app | ios::binary); 
	if (!file_exist) file<<"TYPE,FIRST LEVEL ACTION,FIRST LEVEL TABLE,DATA ACTION,DATA TABLE,Column1,Column2\n";	//If the file is not found, create the headers list of the config file
	file<<escapeCSV(type)<<",INDIVIDUAL,"<<escapeCSV(type)<<",INDIVIDUAL,"<<escapeCSV(type+"_data");
	for (const auto& header : headers){
		file<<","<<escapeCSV(header);
	}
	file<<"\n";
	file.close();
	
	//Update the Table Config Table
	file_exist=false;
	file_path="Config/"+table_config_file;
	file_check.open(file_path);
    if (file_check.good()) file_exist=true;
	file_check.close();
	file.open(file_path, ios::app | ios::binary); 
	if (!file_exist) file<<"TABLE,Column1,Column2,Column3\n";	//If the file is not found, create the headers list of the config file
	file<<escapeCSV(type)<<",TYPE";								//First Level Table
	for (const auto& header : headers){
		file<<","<<escapeCSV(header);
	}
	file<<"\n";
	//Data Table
	
	text=escapeCSV(type+"_data")+",TYPE,";
	for (int i=0;i<3;i++){
		if (headers.size()>i) text+=escapeCSV(headers[i]);
		text+=",";
	}
	
	file<<text<<"ATTRIBUTE NAME,ATTRIBUTE VALUE,PARENT OBJECT 1,PARENT OBJECT 1 NAME,PARENT OBJECT 2,PARENT OBJECT2 NAME,PARENT OBJECT 3,PARENT OBJECT3 NAME\n";
	file.close();
}



string loadWordList(string file_name){
	logMessage("EVENT","Reading: "+file_name);
	string loadWordList;
	string line;
	ifstream file("Config/"+file_name);
	if (!file.is_open()) return loadWordList;
    while (getline(file, line)) loadWordList+=" "+line;
    loadWordList+=" ";
	file.close();
    return loadWordList;
}

void createOutTable(const vector<string>& headers,string file_name,string folder_name){
	string text;
	string file_path=folder_name+file_name+".csv";
	ifstream file_check(file_path);
    if (file_check.good()) return;
    logMessage("EVENT","Creating: "+file_name);
	ofstream file(file_path, ios::app | ios::binary);   
	for (const auto& header : headers) text+=escapeCSV(header)+",";
	text.pop_back();
	file<<text<<"\n";
	file.close();
}

string getTimestamp() {
    auto now = time(nullptr);
    stringstream ss;
    ss << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Function to initialize log files with timestamped names
void initLogFiles() {
    string timestamp = getTimestamp();
    // Replace spaces and colons with underscores for filename
    replace(timestamp.begin(), timestamp.end(), ' ', '_');
    replace(timestamp.begin(), timestamp.end(), ':', '-');

    string logFilePath = "parser_events_.log";//+ timestamp 
    string traceFilePath = "parser_trace_.txt";

    logFile.open(logFilePath, ios::out);
    if (!logFile.is_open()) {
        cerr << "ERROR: Failed to open log file: " << logFilePath << endl;
    }

    traceFile.open(traceFilePath, ios::out);
    if (!traceFile.is_open()) {
        cerr << "ERROR: Failed to open trace file: " << traceFilePath << endl;
    }

    // Log initialization event
    if (logFile.is_open()) {
        logFile << "[" << getTimestamp() << "] EVENT: Log file initialized: " << logFilePath << endl;
        logFile.flush();
    }
}

// Function to log events, warnings, or errors
void logMessage(string severity, const string& message) {
    string entry = "[" + getTimestamp() + "] " + severity + ": " + message;
    if (logFile.is_open()) {
        logFile << entry << endl;
        logFile.flush();
    }
    // Also output to console for debugging
    if (severity == "ERROR") cerr << entry << endl;
}

// Function to log a file line (trace)
void logTraceLine(uint64_t lineNumber, const string& line) {
    if (traceFile.is_open()) {
        traceFile << "Line " << lineNumber << ": " << line << endl;
        traceFile.flush();
    }
}

uint64_t countLines(const string& filename) {
	logMessage("EVENT","Counting lines");
	std::ifstream file(filename);
    if (!file.is_open()) {
        logMessage("ERROR","Failed to open file: "+filename);
        return 100;
    }
    cout.imbue(std::locale(cout.getloc(), new thousands_separator));
	uint64_t line_count = 0;
    string line;
	
    char frames[] = {'|', '/', '-', '\\'};
	int frame_count=0;
	
	while (getline(file, line)) {
		line_count++;
		if (line_count % 20000==0){
			cout<<"\rCounting lines: "<<frames[frame_count]<<flush;
			frame_count++;
			if (frame_count>=sizeof(frames)) frame_count=0;
		}	
	}
    file.close();
	cout<<"\r";
    logMessage("EVENT","Finished counting lines: " + to_string(line_count));
	return line_count;
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
        if (i < pos) cout << "=";
        else if (i == pos) cout << ">";
        else cout << " ";
    }
    cout << "] " << fixed << setprecision(1) << (progress * 100.0) << " %  \r";
    cout<<flush;
    last_update = now;
}

chrono::steady_clock::time_point printCurrentTime(chrono::steady_clock::time_point start_time) {
	auto now = time(nullptr);
	if (start_time == std::chrono::steady_clock::time_point{}){
		cout << put_time(localtime(&now), "%H:%M:%S")<<"\n";
		return chrono::steady_clock::now();
	}else{
		auto duration = std::chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now()-start_time);
		long long seconds =  duration.count();
    	long long minutes = seconds / 60;
    	seconds %= 60;
		cout << put_time(localtime(&now), "%H:%M:%S")<<"\n";
		cout<<"Total Time: "<< minutes << " min " << seconds << " sec\n";
		return chrono::steady_clock::now();
	}
}

void playEndSound() {
	Beep(1245,300);
	Sleep(100);
	Beep(1245,300);
}

