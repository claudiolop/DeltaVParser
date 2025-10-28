#include "DeltaVObject.h"
#include "Utils.h"
#include "UnicodeFileReader.h"
#include <windows.h>
#include <stack>
#include <fstream>
#include <set>
#include <iostream>
#include <algorithm>

using namespace std; // Import entire std namespace

	
map<string, set<string>> typeToAttributes;
map<string, set<string>> typeToChildTypes;

string output_folder;
string object_user;
string object_time;
unique_ptr<DeltaVObject> deltav_object = nullptr;
unique_ptr<DeltaVObject> top_object = nullptr;
stack<DeltaVObject*> object_stack;

long long line_count=0;
map<string, TableConfig> type_config;
map<string, TableConfig> table_config;
map<string, TableConfig> default_tables;
string type;
vector<Attribute> attrs;
string avoid_types;
string skip_types;
string skip_attributes;
vector<int> skip_count;



void collectSchema(DeltaVObject* obj, vector<string>& path) {
    if (!obj) return;
    string t = trim(obj->type);
    if (t.empty()) return;

    // Add current type to the path
    path.push_back(t);
    string path_key = joinPath(path);

    // Collect attributes for the current type path
    for (const auto& attr : obj->attributes) {
        string attr_name = trim(attr.name);
        if (!attr_name.empty()) {
            typeToAttributes[path_key].insert(attr_name);
        }
    }

    // Collect child types for the current type path
    for (const auto& child : obj->children) {
        string child_t = trim(child->type);
        if (!child_t.empty()) {
            typeToChildTypes[path_key].insert(child_t);
        }
        // Recursively collect schema for child, passing the updated path
        collectSchema(child.get(), path);
    }

    // Remove current type from path after processing
    path.pop_back();
}

void printSchema() {
    ofstream schema_file(output_folder+"\\schema.csv");
    if (!schema_file.is_open()) {
        logMessage("ERROR", "Failed to open file: schema.csv");
        return;
    }
    schema_file << "Level,Type,ItemType,ItemName\n";

    for (const auto& kv : typeToAttributes) {
        const string& path_key = kv.first;
        if (path_key.empty()) continue;

        // Write attributes for this path
        for (const auto& a : kv.second) schema_file << "Attribute," << escapeCSV(a)<< ","<< path_key<<"\n";

        // Write child types for this path
        auto child_it = typeToChildTypes.find(path_key);
        if (child_it != typeToChildTypes.end()) for (const auto& c : child_it->second)  schema_file << "SubObject," << escapeCSV(c) <<","<< path_key<< "\n";
    }
    schema_file.close();
}

void printAllLevels() {
	if (type_config[top_object->type].data_action=="SKIP") return;
	createOutTable(table_config[type_config[top_object->type].data_table].headers,type_config[object_stack.top()->type].data_table,output_folder);
	string file_path=output_folder+type_config[object_stack.top()->type].data_table+".csv";
	ofstream file(file_path, ios::app | ios::binary);
	DeltaVObject::type_config=type_config[top_object->type];
	DeltaVObject::table_config=table_config[type_config[object_stack.top()->type].data_table];
	DeltaVObject::table_file = move(file);
	int depth=0;
	string text=escapeCSV(top_object->type)+",";
	for (int i=0;i<3;i++){
		if (top_object->attributes.size()>i) text+=escapeCSV(top_object->attributes[i].value);
		text+=",";
	}
	top_object->preOrder(depth,text,"");
}

void printFirstLevel() {
	string text;
	string file_path;
	map<string,string> attribute_map;
	int depth=0;
	if (type_config[top_object->type].first_level_action=="SKIP" or type_config[top_object->type].first_level_action==" ") return;
	file_path=output_folder+type_config[object_stack.top()->type].first_level_table+".csv";
	DeltaVObject::avoid_types=avoid_types;
	DeltaVObject::skip_attributes=skip_attributes;
	DeltaVObject::type_config=type_config[top_object->type];
	DeltaVObject::table_config=table_config[type_config[object_stack.top()->type].first_level_table];

	if (type_config[top_object->type].first_level_action=="INDIVIDUAL") {
		createOutTable(table_config[type_config[top_object->type].first_level_table].headers,type_config[object_stack.top()->type].first_level_table,output_folder);
		ofstream file(file_path, ios::app | ios::binary);    
		text=escapeCSV(object_stack.top()->type)+",";
		for (const auto& attr : top_object->attributes) {
			if (attribute_map.count(trim(attr.name))>0) {
				logMessage("ERROR","Duplicate attribute name found. Type: "+top_object->type+" Name: "+attr.name+" Closing Line: " + to_string(line_count));
				exit(200);
			}
			attribute_map[attr.name]=trim(attr.value);
		}
		for (const auto& header : type_config[top_object->type].headers) text+=escapeCSV(attribute_map[header])+",";
		text.pop_back();
		file<<text<<"\n";
		file.close();
		return;
	}
	if (type_config[top_object->type].first_level_action=="COMBINE") {
		createOutTable(table_config[type_config[top_object->type].first_level_table].headers,type_config[object_stack.top()->type].first_level_table,output_folder);
		ofstream file(file_path, ios::app | ios::binary);    
		DeltaVObject::table_file=move(file);
		text=escapeCSV(object_stack.top()->type)+",";
		object_stack.top()->preOrder(depth,text,"");
	}

}

void processPrevLine() {
	if (type=="" and attrs.size()==0) return;

	if (object_stack.size()>0) {
		if (attrs.size()!=0) object_stack.top()->addAttributes(attrs);						//Add the previous line attributes to the last object
		type="";
		attrs.clear();
		return;
	} else {
		logMessage("ERROR","Trying to add attributes without parent object in line: "+to_string(line_count));
		exit(100);
	}
}


void openBranch() {
	if (type=="" and attrs.size()==0) {
		logMessage("ERROR","Tying to create an object without type or attributes in line: "+to_string(line_count));
		exit(200);
	}

	size_t find_type=skip_types.find(" "+type+" ");
	if (find_type!=string::npos and type!="") {
		type="";
		skip_count.push_back(1);
		return;
	}

	if (skip_count.size()>0) skip_count.back()++;

	deltav_object = make_unique<DeltaVObject>(type, attrs);								//Creating the new object with the information of the previous line.
	DeltaVObject* new_object=deltav_object.get();										//Pointer to latter add the object to the stack.

	if (object_stack.empty()) top_object=move(deltav_object);							//If this is the first object, move it to the top
	if (!object_stack.empty()) object_stack.top()->addChild(move(deltav_object));		//If there is a parent, move it as a child

	object_stack.push(new_object);														//Push it in the stack

	//Add user and time if available
	if (object_user!="") object_stack.top()->addAttribute("user",object_user);
	if (object_time!="") object_stack.top()->addAttribute("time",object_time);
	object_user="";
	object_time="";

	//Clear variables
	type="";
	attrs.clear();
}

void closeBranch() {
	processPrevLine();
	if (skip_count.size()>0) {
		skip_count.back()--;
		if (skip_count.back()==0) {
			skip_count.pop_back();
			return;
		}
	}

	if (object_stack.size()==1) {
		if (type_config.find(top_object->type)==type_config.end()){
			logMessage("WARNING","Type: "+top_object->type+" not found in configuration");
			vector<string> headers;
			for (const auto& attribute : top_object->attributes) headers.push_back(attribute.name);
			updateConfig(top_object->type,headers);
			type_config=loadTypeConfig();
			table_config=loadTableConfig();
		} 
		printFirstLevel();
		printAllLevels();
		vector<string> path;
		collectSchema(top_object.get(), path);
	}
	object_stack.pop();
}


int main(int argc, char* argv[]) {
	string fhx_path;
	bool merge = false;
    bool trace = false;
	output_folder="OutputTables/";
	int qoute_count=0;
	int comment_count=0;
	vector<string> deltav_line;
	string file_line;
	vector<string> deltav_lines;
	string prev_value;
	
	cout << "\033[?25l" << flush; // Hide cursor
	
	 for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-v" || arg == "--version"){
			cout<<GetFileVersion();
			return 0;
		} 
		if (arg == "-m" || arg == "--merge"){
			logMessage("EVENT","Merge option activated");
			merge = true;
		} 
        if (arg == "-t" || arg == "--trace"){
			logMessage("EVENT","Trace option activated");
			trace = true;
		}
    }

	if (argc < 2) {
    	logMessage("ERROR","A fhx file is required!\n");
    	return 100;
   }
 
	fhx_path=argv[1];

	string console_title="DeltaVParser: "+ extractFileName(fhx_path);
	SetConsoleTitleA(console_title.c_str());

	cout<<GetFileVersion()<<"\n";
	cout<<"Start: ";
	chrono::steady_clock::time_point start_time=printCurrentTime(chrono::steady_clock::time_point{});
	initLogFiles();
	
	logMessage("EVENT",GetFileVersion());
	logMessage("EVENT","Using: "+fhx_path);
	cout<<"Using: "<<fhx_path<<"\n";

	//CHEQUEAR QUE PASA CUANDO UN TYPO REPITE EL NOMBRE DEL ATTRIBUTO CON DISTINTOS VALORES
	//EJEMPLO: DOMAIN

	//CHECK ELECTRONIC SIGNATURE, EL NAME DE

	if (!merge) deleteFilesInFolder(output_folder);
	
	type_config=loadTypeConfig();
	table_config=loadTableConfig();

	avoid_types=loadWordList("AvoidTypes.csv");
	skip_types=loadWordList("SkipTypes.csv");
	skip_attributes=loadWordList("SkipAttributes.csv");

	long long total_lines = countLines(fhx_path);
	cout<<"Opening Source File\r";
	
	logMessage("EVENT","Opening fhx file: "+extractFileName(fhx_path));
	UnicodeFileReader fhx_file(fhx_path);

	if (!fhx_file.is_open()) {
		logMessage("ERROR","Can't open the fhx file: "+fhx_path);
		return 100;
	}

	auto start = chrono::steady_clock::now();
	auto last_update = start;

	while (fhx_file.readLine(file_line)) {
		line_count++;
		file_line=trim(file_line);
		if (line_count==279){
			cout<<"";
		}
		
		if (file_line.empty()) continue;

		if (trace) logTraceLine(line_count,file_line,object_stack.size());
	
		deltav_lines = splitString(file_line,qoute_count,comment_count);						//If there are { or } within the line, splits those lines
		
		for (string& deltav_line : deltav_lines) {
			if (qoute_count%2==0) qoute_count=0;
			if (comment_count%2==0) comment_count=0;

			size_t user_pos = deltav_line.find("user=");			
			if (user_pos == 0) {
				auto [type, attrs] = DeltaVObject::ParseLine(trim(deltav_line),qoute_count,prev_value);	
				object_user=attrs[0].value;
				object_time=attrs[1].value;
				attrs.clear();
				continue;
			}

			if (deltav_line=="{") {
				openBranch();
				continue;
			}
			if (deltav_line=="}") {
				closeBranch();
				continue;
			}

			processPrevLine();

			deltav_line=trim(deltav_line);

			auto parse_result = DeltaVObject::ParseLine(trim(deltav_line),qoute_count,prev_value);
			type=parse_result.first;
			attrs=parse_result.second;

			if(!object_stack.empty() and prev_value!="") {
				if (!object_stack.top()->attributes.empty()) {
					object_stack.top()->attributes.back().value+=" "+prev_value;
					prev_value="";
				}
			}
		}

		updateProgress(line_count, total_lines,2.5,start, last_update);

	}
	fhx_file.close();
	printSchema();
	cout<<"\nEnd: ";
	logMessage("EVENT","Completed processing file: "+extractFileName(fhx_path));
	printCurrentTime(start_time);
	Beep(1245,300);
	return 0;
}
