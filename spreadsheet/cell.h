#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>
#include <deque>
#include <optional>
#include <functional>

class Sheet;

class Cell : public CellInterface {
public:
    using CellParents = std::optional<std::unordered_set<Position, PositionHasher>>;

    Cell(Position pos, SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    std::unordered_set<Position, PositionHasher> GetParents() const;
private:
    class Impl {
    public:
        Impl();
        virtual ~Impl() = default;
        virtual void Set(std::string text) = 0;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual bool IsReferenced() const = 0;
        virtual std::unordered_set<Position, PositionHasher> GetParents() const = 0;
        virtual void SetParent(const Position& parent_pos) = 0;
    };

    // class EmptyImpl : public Impl { };

    class TextImpl : public Impl {
    public:
        TextImpl(CellParents& parents);
        void Set(std::string text);
        CellInterface::Value GetValue() const;
        std::string GetText() const;
        bool IsReferenced() const;
        std::unordered_set<Position, PositionHasher> GetParents() const;
        void SetParent(const Position& parent_pos);
    private:
        std::string text_;
        bool is_referenced_ = false;
        std::unordered_set<Position, PositionHasher> parents_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(Position& pos, const SheetInterface& sheet, CellParents& parents);
        void Set(std::string text);
        CellInterface::Value GetValue() const;
        std::string GetText() const;
        std::vector<Position> GetReferencedCells() const;
        bool IsReferenced() const;
        std::unordered_set<Position, PositionHasher> GetParents() const;
        void SetParent(const Position& parent_pos);
    private:
        std::unique_ptr<FormulaInterface> formula_;   
        FormulaInterface::Value value_ = 0.0;      
        Position pos_;
        const SheetInterface& sheet_;
        std::vector<Position> referenced_cells_;
        bool is_referenced_ = false;       
        std::unordered_set<Position, PositionHasher> visited_;
        std::deque<Impl*> stack_;
        std::unordered_set<Position, PositionHasher> parents_;
        
        void DFS(std::deque<Impl*>& stack, Position pos, Impl* impl_ref, std::unordered_set<Position, PositionHasher>& visited);
    };

    std::unique_ptr<Impl> impl_;
    Position pos_;
    const SheetInterface& sheet_;
    bool is_referenced_ = false;
    Impl* GetImplRef();
};