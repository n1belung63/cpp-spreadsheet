#include "common.h"

#include <cctype>
#include <sstream>
#include <regex>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;
const int ASCII_TABLE_A_INDEX = 65;

using namespace std::string_literals;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return row < rhs.row || col < rhs.col;
}

bool Position::IsValid() const {
    if (row == NONE.row && col == NONE.col) {
        return false;
    } else {
        return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
    }   
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return ""s;
    } else {
        std::vector<int> num;
        num.reserve(5);
        int n = col;

        while (n > 0) {
            num.push_back(n % LETTERS); 
            n = n / LETTERS;
            if (n == LETTERS) {
                num.push_back(LETTERS);
                break;
            }
        }
        if (num.empty()) {
            num.push_back(0);
        }
        ++num.front();

        std::stringstream ss;
        while (num.size() > 0) {
            ss << static_cast<char>(ASCII_TABLE_A_INDEX + num.back() - 1);
            num.pop_back();
        }
        ss << (row + 1);
        return ss.str();
    }
}

Position Position::FromString(std::string_view str) {
    static const std::regex r(R"(^([A-Z]?[A-Z]|[A-W][A-Z]{2}|X[A-E][A-Z]|XF[A-D])([1-9]|[1-9][0-9]{1,3}|1[0-5][0-9]{3}|16[0-2][0-9]{2}|163[0-7][0-9]|1638[0-4])$)");
    if (!std::regex_match(str.data(), r)) {
        return Position::NONE;
    } else {
        Position pos;
        size_t digit_pos;
        for (size_t i = 0; i < str.size(); ++i) {
            if (std::isdigit(static_cast<unsigned char>(str.at(i)))) {
                digit_pos = i;
                break;
            }           
        }

        std::vector<int> num;
        num.reserve(5);
        for (size_t i = 0; i < digit_pos; ++i) {
            num.push_back(static_cast<char>(str.at(i)) - ASCII_TABLE_A_INDEX + 1);
        }

        int value = 0, base = 1; 
        while (num.size() > 0) {    
            value += num.back() * base; 
            base = base * LETTERS; 
            num.pop_back();
        }       
    
        pos.col = value - 1;
        pos.row = std::atoi(str.substr(digit_pos, str.npos).data()) - 1;

        return pos;
    }
}

bool Size::operator==(Size rhs) const {
    return rows == rhs.rows && cols == rhs.cols;
}