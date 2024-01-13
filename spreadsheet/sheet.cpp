#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("No such cell"s);
    }

    if (pos_to_cell_.count(pos)) {
        InvalidateCache(pos);
        ((Cell*)(pos_to_cell_.at(pos).get()))->Set(text);
    }
    else {
        std::unique_ptr<CellInterface> cell = std::make_unique<Cell>(pos, *this);
        ((Cell*)(cell.get()))->Set(text);

        cache_[pos] = cell.get()->GetValue();

        pos_to_cell_[pos] = std::move(cell);

        if (printable_size_.rows == 0 && printable_size_.cols == 0) {
            printable_size_ = { pos.row + 1, pos.col + 1};

            max_in_row_[pos.row] = pos.col;
            max_in_col_[pos.col] = pos.row;

        } else {
            auto max_col_in_printable_area = printable_size_.cols - 1;
            auto max_row_in_printable_area = printable_size_.rows - 1;

            if (pos.col > max_col_in_printable_area) {
                printable_size_.cols = pos.col + 1;
            }       
            if (pos.row > max_row_in_printable_area) {
                printable_size_.rows = pos.row + 1;
            }

            if (!max_in_col_[pos.col].has_value()) {
                max_in_col_[pos.col] = pos.row;
            } else if (pos.row > max_in_col_[pos.col]) {
                max_in_col_[pos.col] = pos.row;
            }

            if (!max_in_row_[pos.row].has_value()) {
                max_in_row_[pos.row] = pos.col;
            } else if (pos.col > max_in_row_[pos.row]) {
                max_in_row_[pos.row] = pos.col;
            }
        }

    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("No such cell"s);
    }

    if (pos_to_cell_.count(pos)) {
        return pos_to_cell_.at(pos).get();
    } else {
        return nullptr;
    }  
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("No such cell"s);
    }

    if (pos_to_cell_.count(pos)) {
        return pos_to_cell_.at(pos).get();
    } else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("No such cell"s);
    }

    if (pos_to_cell_.count(pos)) {
        pos_to_cell_.erase(pos);

        if (printable_size_.rows == 1 && printable_size_.cols == 1) {
            printable_size_ = {0, 0};

            max_in_row_[pos.row] = std::nullopt;
            max_in_col_[pos.col] = std::nullopt;

        } else {
            auto max_col_in_printable_area = printable_size_.cols - 1;
            auto max_row_in_printable_area = printable_size_.rows - 1;

            auto find_max = [this] (auto first, auto last, auto except, int max) {
                for (auto it = first; it != last; ++it)
                    if (it->has_value() && it != except)
                        if (it->value() > max)
                            max = it->value();
                return max;
            };

            if (pos.row == max_row_in_printable_area) { // bottom row
                auto beg = max_in_col_.begin();
                int max_row = find_max(beg, beg + max_col_in_printable_area + 1, beg + pos.col, -1);

                max_row_in_printable_area = max_row;
                printable_size_.rows = max_row + 1;

                if (max_in_col_.at(pos.col).value() == 0) {
                    max_in_col_[pos.col] = std::nullopt;
                } else {
                    max_in_col_[pos.col] = max_row_in_printable_area;
                }
            }

            if (pos.col == max_col_in_printable_area) { // right col
                auto beg = max_in_row_.begin();
                int max_col = find_max(beg, beg + max_row_in_printable_area + 1, beg + pos.row, -1);

                max_col_in_printable_area = max_col;
                printable_size_.cols = max_col + 1;

                if (max_in_row_.at(pos.row).value() == 0) {
                    max_in_row_[pos.row] = std::nullopt;
                } else {
                    max_in_row_[pos.row] = max_col_in_printable_area;
                }
            }

            if (pos.row < max_row_in_printable_area && pos.col < max_col_in_printable_area) { // center
                for (int col = max_col_in_printable_area; col >= 0; col--) {
                    if (pos_to_cell_.count({ pos.row, col})) {
                        max_in_row_[pos.row] = col;
                        break;
                    }
                }
                if (max_in_row_.at(pos.row).value() == pos.col) {
                    max_in_row_[pos.row] = std::nullopt;
                }

                for (int row = max_row_in_printable_area; row >= 0; row--) {
                    if (pos_to_cell_.count({ row, pos.col})) {
                        max_in_col_[pos.col] = row;
                        break;
                    }
                }
                if (max_in_col_.at(pos.col).value() == pos.row) {
                    max_in_col_[pos.col] = std::nullopt;
                }
            }
        }

    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int row = 0; row < printable_size_.rows; ++row) {
        for (int col = 0; col < printable_size_.cols; ++col) {
            auto pos = Position{row, col};
            if (pos_to_cell_.count(pos)) {
                if (cache_.at(pos).has_value()) {
                    output << cache_.at(pos).value();
                } else {
                    output << pos_to_cell_.at(pos)->GetValue();
                }
            }
            if (col == (printable_size_.cols -1)) {
                output << '\n';
            } else {
                output << '\t';
            }
        }
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int row = 0; row < printable_size_.rows; ++row) {
        for (int col = 0; col < printable_size_.cols; ++col) {
            auto pos = Position{row, col};
            if (pos_to_cell_.count(pos)) {
                output << pos_to_cell_.at(pos)->GetText();
            }
            if (col == (printable_size_.cols -1)) {
                output << '\n';
            } else {
                output << '\t';
            }
        }
    }
}

void Sheet::InvalidateCache(Position pos) {
    cache_[pos] = std::nullopt;
    for (const auto& parent : ((Cell*)GetCell(pos))->GetParents()) {
        cache_[parent] = std::nullopt;
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}