#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

class GetCell
{
public:
    GetCell(const SheetInterface& sheet) : sheet_(const_cast<SheetInterface&>(sheet)) { }
    CellInterface* operator()(Position pos){
        return sheet_.GetCell(pos);
    }
private:
    SheetInterface& sheet_;
};

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
    : ast_(ParseFormulaAST(std::move(expression))) {
        
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            for (auto cell : ast_.GetCells()) {
                Position pos = { cell.row, cell.col };
                if (sheet.GetCell(pos) == nullptr) {
                    const_cast<SheetInterface&>(sheet).SetCell(pos, "");
                }
            }
            return ast_.Execute(GetCell{sheet});
        } catch (const FormulaError& e) {
            return e;
        }
        
    }

    std::string GetExpression() const override {
        std::stringstream tmp;
        ast_.PrintFormula(tmp);
        return tmp.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::forward_list<Position>& temp = const_cast<std::forward_list<Position>&>(ast_.GetCells());
        std::vector<Position> vec = std::vector<Position>(temp.begin(), temp.end());
        vec.erase( std::unique( vec.begin(), vec.end() ), vec.end() );
        return vec;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (const std::exception& e) {
        throw FormulaException("Incorect formula");
    }
    
}