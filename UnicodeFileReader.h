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

private:
    bool m_isOpen;
    string m_encoding;
    bool m_bomPresent;
    wifstream m_wfile;
    ifstream m_file;
};

#endif // UNICODE_FILE_READER_H