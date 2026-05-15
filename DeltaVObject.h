#ifndef DELTAVOBJECT_H
#define DELTAVOBJECT_H
#include "Utils.h"
#include <memory>
#include <fstream>


// Struct for attributes
struct Attribute {
    std::string name;  
    std::string value;
};


class DeltaVObject {
public:
	std::string type; 
	std::string user;
	std::string time;
    std::vector<Attribute> attributes; 
    int header_count=0;
    std::vector<std::unique_ptr<DeltaVObject>> children;


    DeltaVObject(const std::string& types, const std::vector<Attribute>& attrs);

	static TableConfig type_config;
	static TableConfig table_config;
	static std::string avoid_types;
	static std::string skip_attributes;
	static std::ofstream& table_file;
	static std::pair<std::string, std::vector<Attribute>> ParseLine(std::string line,int& qoute_count,std::string& prev_value);
    
    void addAttribute(const std::string& name, const std::string& value);
	void addAttributes(const std::vector<Attribute>& attr_list);

    void addChild(std::unique_ptr<DeltaVObject> child);

    void print(int& depth,std::string preceding,std::string following) const;
    void preOrder(int& depth,std::string preceding,std::string following) const;

    
};

#endif 