#ifndef DELTAVOBJECT_H
#define DELTAVOBJECT_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "StringOperations.h"
using namespace std; // Import entire std namespace.

// Struct for attributes
struct Attribute {
    string name;  // Attribute name (e.g., "Name", "Breed")
    string value; // Attribute value (e.g., "Fido", "Labrador")
};

// Class for Animal with dynamic attributes
class DeltaVObject {
public:
	string type;  // Dynamic species read from file
	string user;
	string time;
    vector<Attribute> attributes; // Dynamic attributes
    vector<unique_ptr<DeltaVObject>> children;

    // Constructor
    DeltaVObject(const string& types, const vector<Attribute>& attrs);

	static TableConfig type_config;
	static TableConfig table_config;
	static string avoid_types;
	static string skip_attributes;
	static ofstream& table_file;
	static pair<string, vector<Attribute>> ParseLine(string line,int& qoute_count,string& prev_value);
    // Method to add attribute dynamically
    void addAttribute(const string& name, const string& value);

    // Method to add child
    void addChild(unique_ptr<DeltaVObject> child);

    // Print method
    void print(int& depth,string preceding,string following) const;

    // Pre-order traversal
    void preOrder(int& depth,string preceding,string following) const;

    void addAttributes(const vector<Attribute>& attr_list);
};

#endif 