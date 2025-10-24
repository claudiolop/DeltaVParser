#ifndef DELTAVOBJECT_H
#define DELTAVOBJECT_H
#include "Utils.h"
#include <memory>
#include <fstream>

using namespace std; 

// Struct for attributes
struct Attribute {
    string name;  
    string value; 
};


class DeltaVObject {
public:
	string type; 
	string user;
	string time;
    vector<Attribute> attributes; 
    vector<unique_ptr<DeltaVObject>> children;


    DeltaVObject(const string& types, const vector<Attribute>& attrs);

	static TableConfig type_config;
	static TableConfig table_config;
	static string avoid_types;
	static string skip_attributes;
	static ofstream& table_file;
	static pair<string, vector<Attribute>> ParseLine(string line,int& qoute_count,string& prev_value);
    
    void addAttribute(const string& name, const string& value);
	void addAttributes(const vector<Attribute>& attr_list);

    void addChild(unique_ptr<DeltaVObject> child);

    void print(int& depth,string preceding,string following) const;
    void preOrder(int& depth,string preceding,string following) const;

    
};

#endif 