#include "DeltaVObject.h"
#include "Utils.h"
#include <stack>
#include <fstream>
#include <set>

using namespace std; // Import entire std namespace

map<string, set<string>> typeToAttributes;
map<string, set<string>> typeToChildTypes;

string output_folder;
string object_user;
string object_time;
unique_ptr<DeltaVObject> deltav_object = nullptr;
unique_ptr<DeltaVObject> top_object = nullptr;
stack<DeltaVObject*> object_stack;

uint64_t line_count=0;
map<string, TableConfig> type_config;
map<string, TableConfig> table_config;
map<string, TableConfig> default_tables;
string type;
vector<Attribute> attrs;
string avoid_types;
string skip_types;
string skip_attributes;
vector<int> skip_count;

void collectSchema(DeltaVObject* obj) {
    if (!obj) return;
    string t = trim(obj->type);
    if (t.empty()) return;
    
    for (const auto& attr : obj->attributes) {
        string attr_name = trim(attr.name);
        if (!attr_name.empty()) {
            typeToAttributes[t].insert(attr_name);
        }
    }
    
    for (const auto& child : obj->children) {
        string child_t = trim(child->type);
        if (!child_t.empty()) {
            typeToChildTypes[t].insert(child_t);
        }
        collectSchema(child.get());
    }
}

void printSchema(){
	ofstream schema_file("schema.csv");
if (!schema_file.is_open()) {
    cerr << "Error opening schema.csv" << endl;
    return; // or handle error
}
schema_file << "Type,ItemType,ItemName\n";

for (const auto& kv : typeToAttributes) {
    const string& t = kv.first;
    if (t.empty()) continue;

    bool has_items = false;

    for (const auto& a : kv.second) {
        schema_file << escapeCSV(t) << ",Attribute," << escapeCSV(a) << "\n";
        has_items = true;
    }

    auto child_it = typeToChildTypes.find(t);
    if (child_it != typeToChildTypes.end()) {
        for (const auto& c : child_it->second) {
            schema_file << escapeCSV(t) << ",SubObject," << escapeCSV(c) << "\n";
            has_items = true;
        }
    }

    // Optional: if no items, output the type alone
    // if (!has_items) {
    //     schema_file << escapeCSV(t) << ",Type,\n";
    // }
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
	DeltaVObject::table_file = std::move(file);
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
				cerr<<"\n\nERROR Duplicate attribute name found. Type: "<<top_object->type<<" Name: "<<attr.name<<" Closing Line: "<<line_count<<"\n";
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
		//if (type!="") object_stack.top()->addAttribute("TYPE",type);		//Add previous line type as an attribute to the last object
		if (attrs.size()!=0) object_stack.top()->addAttributes(attrs);						//Add the previous line attributes to the last object
		type="";
		attrs.clear();
		return;
	} else {
		cerr<<"\n\nERROR file_line:"<<line_count<<" Trying to add attributes without parent object.\n";
		cerr<<"type: "<<type<<"\n";
		for (auto attr : attrs) cerr<<"name:"<<attr.name<<" value:"<<attr.value<<"\n";
		exit(100);
	}
}


void openBranch() {
	if (type=="" and attrs.size()==0) {
		cerr<<"\n\nERROR file_line:"<<line_count<<" Tying to create an object without type or attributes.\n";
		exit(200);
	}

	size_t find_type=skip_types.find(" "+type+" ");
	if (find_type!=string::npos and type!="") {
		type="";
		skip_count.push_back(1);
		return;
	}

	if (skip_count.size()>0 )skip_count.back()++;

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
			vector<string> headers;
			for (const auto& attribute : top_object->attributes) headers.push_back(attribute.name);
			updateConfig(top_object->type,headers);
			type_config=loadTypeConfig();
			table_config=loadTableConfig();
		} 
		printFirstLevel();
		printAllLevels();
		collectSchema(top_object.get());
	}
	object_stack.pop();
}


int main() {
	output_folder="OutputTables/";
	cout << "\033[?25l" << flush; // Hide cursor
	cout<<"Start: ";
	std::chrono::steady_clock::time_point start_time=printCurrentTime(std::chrono::steady_clock::time_point{});
	int qoute_count=0;
	int comment_count=0;
	
	vector<string> deltav_line;
	string file_line;
	vector<string> deltav_lines;
	string prev_value;
	ofstream log_file("log.txt", std::ios::binary);

	//CHEQUEAR QUE PASA CUANDO UN TYPO REPITE EL NOMBRE DEL ATTRIBUTO CON DISTINTOS VALORES
	//EJEMPLO: DOMAIN

	//CHECK ELECTRONIC SIGNATURE, EL NAME DE

	//string SourceFile="fhx/Test.fhx";
	//string SourceFile="fhx/SJC2020.fhx";
	//string SourceFile="fhx/CAMP_DeltaV_System1.fhx";
	string SourceFile="fhx/LLDVGIA.fhx";
	//string SourceFile="fhx/PCL3.fhx";


	deleteFilesInFolder(output_folder);

	type_config=loadTypeConfig();
	table_config=loadTableConfig();

	avoid_types=loadWordList("AvoidTypes.csv");
	skip_types=loadWordList("SkipTypes.csv");
	skip_attributes=loadWordList("SkipAttributes.csv");

	cout<<"Using: "<<SourceFile<<"\n";

	uint64_t total_lines = countLines(SourceFile);
	cout<<"Opening Source File\r";

	ifstream fhx_file(SourceFile, std::ios::binary);     // Open Source File

	if (!fhx_file.is_open()) {
		cerr << "ERROR: Can't open the fhx file: "+SourceFile << endl;
		return 100;
	}

	auto start = chrono::steady_clock::now();
	auto last_update = start;
	while (getline(fhx_file, file_line)) {
		line_count++;

		file_line=skipNull(file_line);
		file_line=trim(file_line);

		if (file_line.empty()) continue;
		log_file<<line_count<<"\t"<<file_line<<"\n";
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
	log_file.close();
	printSchema();
	cout<<"\nEnd: ";
	printCurrentTime(start_time);
	playEndSound();
	return 0;
}
