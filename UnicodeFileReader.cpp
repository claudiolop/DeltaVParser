#include "UnicodeFileReader.h"

#include <vector>
#include <cstdint>
using namespace std;


bool UnicodeFileReader::readUtf16Line(string& line) {
    line.clear();
    uint16_t cp;
    unsigned char bytes[2];
    while (m_file.read(reinterpret_cast<char*>(bytes), 2)) {
        cp = bytes[0] | (bytes[1] << 8);  // explicit little-endian assembly

        if (cp == 0x000D) {  // \r
            unsigned char next_bytes[2];
            if (m_file.read(reinterpret_cast<char*>(next_bytes), 2)) {
                uint16_t next = next_bytes[0] | (next_bytes[1] << 8);
                if (next != 0x000A)  // if not \n, put it back
                    m_file.seekg(-2, ios::cur);
            }
            return true;
        }
        if (cp == 0x000A) return true;  // \n

        // Convert UTF-16 code point to UTF-8
        if (cp < 0x80) {
            line += static_cast<char>(cp);
        } else if (cp < 0x800) {
            line += static_cast<char>(0xC0 | (cp >> 6));
            line += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            line += static_cast<char>(0xE0 | (cp >> 12));
            line += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            line += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return !line.empty();
}

UnicodeFileReader::UnicodeFileReader(const string& filename)
    : m_isOpen(false), m_encoding(""), m_bomPresent(false) {

    // Read BOM using a temporary file handle
    ifstream bomFile(filename, ios::binary);
    if (!bomFile) return;
    vector<unsigned char> bom(4, 0);
    bomFile.read(reinterpret_cast<char*>(bom.data()), 4);
    bomFile.close();

    if (bom[0] == 0xFF && bom[1] == 0xFE) {
        m_encoding = "UTF-16LE";
        m_bomPresent = true;
    } else if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        m_encoding = "UTF-8";
        m_bomPresent = true;
    } else {
        m_encoding = "UTF-8";
    }

    // Single binary ifstream for both encodings
    m_file.open(filename, ios::binary);
    if (!m_file) return;

    if (m_encoding == "UTF-16LE") {
        if (m_bomPresent) m_file.seekg(2); // Skip 2-byte UTF-16 BOM
    } else {
        if (m_bomPresent) m_file.seekg(3); // Skip 3-byte UTF-8 BOM
    }

    m_isOpen = true;
}

bool UnicodeFileReader::is_open() const {
    return m_isOpen;
}


void UnicodeFileReader::close(){
	m_file.close();
}

bool UnicodeFileReader::getline_cr_lf(istream& is, string& line) {
    line.clear();
    char ch;
    while (is.get(ch)) {
        if (ch == '\r') {
            //if (is.peek() == '\n') is.get();
            return true;
        }
        if (ch == '\n') {
            return true;
        }
        line.push_back(ch);
    }
    return !line.empty();
}

bool UnicodeFileReader::readLine(string& line) {
    if (!m_isOpen) return false;
    if (m_encoding == "UTF-16LE")
        return readUtf16Line(line);
    else
        return getline_cr_lf(m_file, line);
}