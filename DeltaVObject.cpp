#include "DeltaVObject.h"
#include "Utils.h"
#include <fstream>
using namespace std;

TableConfig DeltaVObject::type_config;
TableConfig DeltaVObject::table_config;
string DeltaVObject::avoid_types;
string DeltaVObject::skip_attributes;
ofstream& DeltaVObject::table_file = *(new ofstream()); 

DeltaVObject::DeltaVObject(const string& type,const vector<Attribute>& attrs) : type(type),attributes(attrs) {}

//Takes one DeltaV line and returns type and vector<Attributes>. 
pair<string, vector<Attribute>> DeltaVObject::ParseLine(string line,int& qoute_count,string& prev_value) {
	string type;
	string name;
	string value;
	vector<Attribute> attrs;
	bool type_complete=false;
  	string current;
  	
  	size_t qu_pos=0;
  	prev_value="";
  		
	if (qoute_count%2!=0 or line[0]=='"'){						//If I'm in qoute, everything until the next " is part of the value of the last attribute	
  		for (qu_pos=0;qu_pos<line.size();qu_pos++){
			if (line[qu_pos]=='"'){
				if (qu_pos<line.size()-1){
					if (line[qu_pos+1]=='"'){
					qu_pos++;
					prev_value+="\"";
					continue;
					}
				}
				qoute_count++;
			}else{
				prev_value+=line[qu_pos];
			} 
  		}
	  line=line.substr(qu_pos);
	} 
	//If I'm here, is eityher TYPE, or TYPE HEADER1=VAR1 or HEADER1=VAR1 OR 00 00 00 00
    size_t eq_pos = line.find('=');	
	size_t sp_pos = line.find(' ');	
	if (eq_pos==line.size()-1 and line.size()>0){
		line.pop_back();
		size_t eq_pos = line.find('=');	
	}
	
	if(eq_pos==string::npos and sp_pos!=string::npos ){		//If there are spaces and no equals, I consider everything a type..
		value=line;
		attrs.push_back({name,value});
		return {type, attrs};
	}
	
	for (const char c : line){
		if (c==' ' and qoute_count%2==0) {					//And I find a space
			if (!type_complete){							//If this is the first space, and no = before
				type=current;								//Everythig to the left is type
				type_complete=true;
				current="";
			}else{											//If not, eveything to the left is value
				value=current;
				current="";
				attrs.push_back({name,value});
			}
		}else if (c=='=' and qoute_count%2==0) {										//And I find a = everything to the left is name
			name=current;
			type_complete=true;
			current="";
		}else if (c=='"'){
			qoute_count++;
		}else{
			current+=c;	
		}
	}
	if (!type_complete){
		type=current;
		return {type, attrs};
	}else if (name.size()>0){
		value=current;
		attrs.push_back({name,value});
	}
	return {type, attrs};
}

void DeltaVObject::addAttribute(const string& name, const string& value) {
    // Check for duplicate attribute name
    for (const auto& attr : attributes) {
        if (attr.name == name) {
            const_cast<Attribute&>(attr).value = value;
            return;
        }
    }
    // Add new attribute
    attributes.push_back({name, value});
}

void DeltaVObject::addAttributes(const vector<Attribute>& attr_list) {
    attributes.insert(attributes.end(),attr_list.begin(),attr_list.end());
}

void DeltaVObject::addChild(unique_ptr<DeltaVObject> child) {
    children.push_back(move(child));
}

void DeltaVObject::print(int& depth,string preceding,string following) const {
		int att_count=0;
		following.pop_back();
		for (const auto& attr : attributes){
			if (att_count>0 or attr.name=="user" or attributes.size()==1){
				if (skip_attributes=="" or skip_attributes.find(" "+attr.name+" ")==string::npos){
					string line=escapeCSV(trim(attr.name))+","+escapeCSV(trim(attr.value))+",";	
					table_file<<preceding<<line<<following<<"\n";
				}
			}
			att_count++;
		} 
}

void DeltaVObject::preOrder(int& depth,string preceding,string following) const {
	if (type_config.first_level_action=="SKIP") return;
	if (type!="") if (avoid_types.find(" "+type+" ")!=string::npos)	return;
	if (depth>0 or type_config.first_level_action=="COMBINE"){
		if (attributes.size()>0){
			if (attributes[0].name!="user"){
				following=escapeCSV("["+trim(type)+"]")+","+escapeCSV(trim(attributes[0].value))+","+following;
			}else{
				following=escapeCSV("["+trim(type)+"]")+",,"+following;
			}
		}else{
				following=escapeCSV("["+trim(type)+"]")+",,"+following;
		}
		print(depth,preceding,following);
	}
	depth++;
	for (const auto& child : children) child->preOrder(depth,preceding,following);	
	depth--;
}
