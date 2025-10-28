#include "UnicodeFileReader.h"

#include <locale>
#include <codecvt>
#include <vector>

using namespace std;

UnicodeFileReader::UnicodeFileReader(const string& filename) 
    : m_isOpen(false), m_encoding(""), m_bomPresent(false) {
    ifstream bomFile(filename, ios::binary);
    if (!bomFile) return;

    vector<unsigned char> bom(4, 0);
    bomFile.read(reinterpret_cast<char*>(bom.data()), 4);

    if (bom[0] == 0xFF && bom[1] == 0xFE) {
        m_encoding = "UTF-16LE";
        m_bomPresent = true;
    } else if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        m_encoding = "UTF-8";
        m_bomPresent = true;
    } else {
        m_encoding = "UTF-8"; // Assume UTF-8 if no BOM
    }

    if (m_encoding == "UTF-16LE") {
        m_wfile.open(filename, ios::binary);
        if (!m_wfile) return;
        if (m_bomPresent) m_wfile.seekg(2); // Skip BOM
        m_wfile.imbue(locale(m_wfile.getloc(),
            new codecvt_utf16<wchar_t, 0x10ffff, static_cast<codecvt_mode>(consume_header | little_endian)>));
    } else {
        m_file.open(filename);
        if (!m_file) return;
        if (m_bomPresent) m_file.seekg(3); // Skip BOM
    }

    m_isOpen = true;
}

bool UnicodeFileReader::is_open() const {
    return m_isOpen;
}

bool UnicodeFileReader::readLine(string& line) {
    if (!m_isOpen) return false;

    if (m_encoding == "UTF-16LE") {
        wstring wline;
        if (getline(m_wfile, wline,L'\r')) {
            wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
            line = converter.to_bytes(wline);
            return true;
        }
    } else {
        if (getline(m_file, line)) {
            return true;
        }
    }
    return false;
}

void UnicodeFileReader::close(){
	if (m_encoding == "UTF-16LE"){
		m_wfile.close();
	}else{
		m_file.close();
	}
		
}