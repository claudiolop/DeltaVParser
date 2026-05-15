// UnicodeFileReader.h
#ifndef UNICODE_FILE_READER_H
#define UNICODE_FILE_READER_H

#include <fstream>
#include <string>


class UnicodeFileReader {
public:
    UnicodeFileReader(const std::string& filename);
    
	bool is_open() const;
    bool readLine(std::string& line);
    void close();
	std::string m_encoding;
private:
    bool m_isOpen;
    bool readUtf16Line(std::string& line);
    bool m_bomPresent;
    std::ifstream m_file;
    bool getline_cr_lf(std::istream& is, std::string& line);
};

#endif // UNICODE_FILE_READER_H