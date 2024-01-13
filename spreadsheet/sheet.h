#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <deque>
#include <unordered_map>
#include <array>
#include <optional>

class GetCacheFunc;

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    std::optional<CellInterface::Value> GetCache(Position pos);

private:
    std::unordered_map<Position, std::unique_ptr<CellInterface>, PositionHasher> pos_to_cell_;
    Size printable_size_;
    std::array<std::optional<uint16_t>, Position::MAX_ROWS> max_in_col_;
    std::array<std::optional<uint16_t>, Position::MAX_COLS> max_in_row_;

    std::unordered_map<Position, std::optional<CellInterface::Value>, PositionHasher> cache_;
    std::unordered_set<Cell*> visited_;
    std::deque<Cell*> stack_;

    void InvalidateCache(Position pos);
};

class GetCacheFunc {
public:
    GetCacheFunc(SheetInterface& sheet) : sheet_(sheet) { }
    std::optional<CellInterface::Value> operator()(Position pos){
        return ((Sheet&)sheet_).GetCache(pos);
    }
private:
    SheetInterface& sheet_;
};