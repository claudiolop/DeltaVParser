// UnicodeFileReader.h
#ifndef UNICODE_FILE_READER_H
#define UNICODE_FILE_READER_H

#include <fstream>
#include <string>
using namespace std;

class UnicodeFileReader {
public:
    UnicodeFileReader(const string& filename);
    bool is_open() const;
    bool readLine(string& line);
    void close();
	string m_encoding;
private:
    bool m_isOpen;
    
    bool m_bomPresent;
    wifstream m_wfile;
    ifstream m_file;
    bool getline_cr_lf(istream& is, string& line);
    bool getline_cr_lf(wistream& is, wstring& line);
};

#endif // UNICODE_FILE_READER_H