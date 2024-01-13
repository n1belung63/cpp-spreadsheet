#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Impl::Impl() {}

Cell::TextImpl::TextImpl(CellParents& parents) {
    if (parents.has_value()) {
        for (const auto & parent : parents.value()) {
            parents_.emplace(parent);
        }
    }
}

void Cell::TextImpl::Set(std::string text)  {
    text_ = std::move(text);
}

CellInterface::Value Cell::TextImpl::GetValue() const {
    if (text_ == "") {
        return 0.0;
    }

    double number = std::atoi(text_.c_str());
    if (!(number == 0 && text_ != "0")) {
        return number;
    }

    if (text_.at(0) == '\'') {
        return text_.substr(1);
    } else {
        return text_;
    } 
          
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

bool Cell::TextImpl::IsReferenced() const {
    return is_referenced_;
}

void Cell::TextImpl::SetParent(const Position& parent_pos) {
    parents_.emplace(parent_pos);
}

std::unordered_set<Position, PositionHasher> Cell::TextImpl::GetParents() const {
    return parents_;
}

Cell::FormulaImpl::FormulaImpl(Position& pos, const SheetInterface& sheet, CellParents& parents) 
: pos_(pos), sheet_(sheet) {
    if (parents.has_value()) {
        for (const auto & parent : parents.value()) {
            parents_.emplace(parent);
        }
    }
}

void Cell::FormulaImpl::Set(std::string text)  {
    formula_ = ParseFormula(text);
    value_ = formula_->Evaluate(sheet_);
    referenced_cells_ = formula_->GetReferencedCells();
    if (!referenced_cells_.empty()) {
        is_referenced_ = true;
    }

    stack_.push_back((Impl*)this);
    DFS(stack_, pos_, stack_.front(), visited_);

    if (is_referenced_) {
        for (const auto child : referenced_cells_) {
            auto child_cell = (Cell*)(sheet_.GetCell({ child.row, child.col }));
            auto impl_ref_child = child_cell->GetImplRef();
            impl_ref_child->SetParent(pos_);
        }
    }
}

void Cell::FormulaImpl::DFS(std::deque<Impl*>& stack, Position pos, Impl* impl_ref, std::unordered_set<Position, PositionHasher>& visited) {
    if (visited.count(pos) > 0) {
        throw CircularDependencyException("The circle here");
    } else {
        visited.emplace(pos);
    }
    
    stack.pop_front();
    if (impl_ref->IsReferenced()) {
        for (const auto ref : ((FormulaImpl*)impl_ref)->GetReferencedCells()) {
            auto cell = (Cell*)(sheet_.GetCell({ ref.row, ref.col }));
            auto impl_ref_internal = cell->GetImplRef();
            stack.push_front(impl_ref_internal);
            DFS(stack, { ref.row, ref.col }, stack.front(), visited);
        }
    }   
}

void Cell::FormulaImpl::SetParent(const Position& parent_pos) {
    parents_.emplace(parent_pos);
}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    } else  {
        return std::get<FormulaError>(value_);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return '=' + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return referenced_cells_;
}

bool Cell::FormulaImpl::IsReferenced() const {
    return is_referenced_;
}

std::unordered_set<Position, PositionHasher> Cell::FormulaImpl::GetParents() const {
    return parents_;
}


// Реализуйте следующие методы
Cell::Cell(Position pos, SheetInterface& sheet)
: pos_(pos), sheet_(sheet) { }

Cell::~Cell() {
    impl_.release();
}

void Cell::Set(std::string text) {
    CellParents parents;
    if (impl_ != nullptr) {
        parents = impl_->GetParents();
    }

    if (text.size() > 1 && text.at(0) == '=') {
        std::unique_ptr<FormulaImpl> form(new FormulaImpl(pos_, sheet_, parents));
        std::unique_ptr<Impl> temp = std::move(form);
        try {
            temp->Set(text.substr(1));
        } catch (const CircularDependencyException&) {
            temp.release();
            throw CircularDependencyException("The circle here");
        }
        impl_ = std::move(temp);
    } else {
        std::unique_ptr<TextImpl> txt(new TextImpl(parents));
        impl_ = std::move(txt);
        impl_->Set(text);
    }  
}

void Cell::Clear() {
    impl_.release();
}

Cell::Value Cell::GetValue() const {
    return impl_.get()->GetValue();
}

std::string Cell::GetText() const {
    return impl_.get()->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (impl_.get()->IsReferenced()) {
        return ((Cell::FormulaImpl*)(impl_.get()))->GetReferencedCells();
    } else {
        return std::vector<Position>{};
    }
}

bool Cell::IsReferenced() const {
    return impl_.get()->IsReferenced();
}

Cell::Impl* Cell::GetImplRef() {
    return impl_.get();
}

std::unordered_set<Position, PositionHasher> Cell::GetParents() const {
    return impl_.get()->GetParents();
}