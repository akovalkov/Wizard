#pragma once
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
namespace json = boost::json;

#include "FunctionStorage.h"

namespace Wizard
{
    class BlockNode;
    class LiteralNode;
    class TextNode;
    class CommentNode;
    class ExpressionNode;
    class FunctionNode;
    class DataNode;
    class StatementNode;
    class ExpressionWrapperNode;
    class IfStatementNode;
    class ForStatementNode;
    class ForArrayStatementNode;
    class ForObjectStatementNode;
    class FileStatementNode;
    class ApplyTemplateStatementNode;
    class SetStatementNode;

    class NodeVisitor
    {
    public:
        virtual ~NodeVisitor() = default;

        virtual void visit(const BlockNode &node) = 0;
        virtual void visit(const LiteralNode& node) = 0;
        virtual void visit(const TextNode& node) = 0;
        virtual void visit(const CommentNode& node) = 0;
        virtual void visit(const ExpressionNode& node) = 0;
        virtual void visit(const DataNode& node) = 0;
        virtual void visit(const FunctionNode& node) = 0;
        virtual void visit(const StatementNode& node) = 0;
        virtual void visit(const ExpressionWrapperNode& node) = 0;
        virtual void visit(const IfStatementNode& node) = 0;
        virtual void visit(const ForStatementNode& node) = 0;
        virtual void visit(const ForArrayStatementNode& node) = 0;
        virtual void visit(const ForObjectStatementNode& node) = 0;
        virtual void visit(const FileStatementNode& node) = 0;
        virtual void visit(const ApplyTemplateStatementNode& node) = 0;
        virtual void visit(const SetStatementNode& node) = 0;
    };

    class AstNode
    {
    public:
        virtual void accept(NodeVisitor &v) const = 0;

        size_t pos;

        AstNode(size_t pos) : pos(pos) {}
        virtual ~AstNode() {}
    };

    class BlockNode : public AstNode
    {
    public:
        std::vector<std::shared_ptr<AstNode>> nodes;

        explicit BlockNode() : AstNode(0) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class CommentNode : public AstNode
    {
    public:
        const size_t length;

        explicit CommentNode(size_t pos, size_t length) : AstNode(pos), length(length) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };


    class TextNode : public AstNode
    {
    public:
        const size_t length;

        explicit TextNode(size_t pos, size_t length) : AstNode(pos), length(length) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class ExpressionNode : public AstNode
    {
    public:
        explicit ExpressionNode(size_t pos) : AstNode(pos) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class LiteralNode : public ExpressionNode
    {
    public:
        const json::value value;

        explicit LiteralNode(std::string_view data_text, size_t pos) : ExpressionNode(pos), value(json::parse(data_text)) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class DataNode : public ExpressionNode
    {
    public:
        const std::string name;
        //const std::string path;
        const std::vector<std::string> parts;

        explicit DataNode(std::string_view ptr_name, size_t pos) 
            : ExpressionNode(pos), name(ptr_name)/*, 
              path(convert_dot_to_ptr(ptr_name))*/ {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class FunctionNode : public ExpressionNode
    {
        using Op = FunctionStorage::Operation;
        using Associativity = FunctionStorage::Associativity;

    public:

        unsigned int precedence; // 1 - low, 8 - high
        Associativity associativity;
        Op operation;

        std::string name;
        int number_args; // Can also be negative -> -1 for unknown number
        std::vector<std::shared_ptr<ExpressionNode>> arguments;
        CallbackFunction callback;

        // Op => {number_args, precedence, associativity}
      	const std::map<Op, std::tuple<int, int, Associativity>> operation_info = {
            {Op::Not, {1, 4, Associativity::Left}},
            {Op::And, {2, 1, Associativity::Left}},
            {Op::Or,  {2, 1, Associativity::Left}},
            {Op::In,  {2, 2, Associativity::Left}},
            {Op::Equal, {2, 2, Associativity::Left}},
            {Op::NotEqual, {2, 2, Associativity::Left}},
            {Op::Greater, {2, 2, Associativity::Left}},
            {Op::GreaterEqual, {2, 2, Associativity::Left}},
            {Op::Less, {2, 2, Associativity::Left}},
            {Op::LessEqual, {2, 2, Associativity::Left}},
            {Op::Add, {2, 3, Associativity::Left}},
            {Op::Subtract, {2, 3, Associativity::Left}},
            {Op::Multiplication, {2, 4, Associativity::Left}},
            {Op::Division, {2, 4, Associativity::Left}},
            {Op::Power, {2, 5, Associativity::Right}},
            {Op::Modulo, {2, 4, Associativity::Left}},
            {Op::AtId, {2, 8, Associativity::Left}}
        };


        explicit FunctionNode(std::string_view name, size_t pos)
            : ExpressionNode(pos), precedence(8), associativity(Associativity::Left), operation(Op::Callback), name(name), number_args(0) {}
            
        explicit FunctionNode(Op operation, size_t pos) : ExpressionNode(pos), operation(operation), number_args(1)
        {
            precedence = 1;
            associativity = Associativity::Left;
            auto it = operation_info.find(operation);
            if(it != operation_info.end()) {
                number_args = std::get<0>(it->second); 
                precedence = std::get<1>(it->second); 
                associativity = std::get<2>(it->second); 
            }
        }

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    // Wrapper around ExpressionNode
    // wrapper always existed (because the "visit(&)" method), but wrapped expression (property "root") may be empty
    class ExpressionWrapperNode : public AstNode
    {
    public:
        std::shared_ptr<ExpressionNode> root;

        explicit ExpressionWrapperNode() : AstNode(0) {}
        explicit ExpressionWrapperNode(size_t pos) : AstNode(pos) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class StatementNode : public AstNode 
    {
    public:
        StatementNode(size_t pos) : AstNode(pos) {}
        virtual void accept(NodeVisitor &v) const = 0;
    };

    class IfStatementNode : public StatementNode
    {
    public:
        ExpressionWrapperNode condition;
        BlockNode true_statement;
        BlockNode false_statement;
        BlockNode *const parent;

        const bool is_nested;
        bool has_false_statement{false};

        explicit IfStatementNode(BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent), is_nested(false) {}
        explicit IfStatementNode(bool is_nested, BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent), is_nested(is_nested) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class ForStatementNode : public StatementNode
    {
    public:
        ExpressionWrapperNode condition;
        BlockNode body;
        BlockNode *const parent;

        ForStatementNode(BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent) {}

        virtual void accept(NodeVisitor &v) const = 0;
    };

    // for value in array
    class ForArrayStatementNode : public ForStatementNode
    {
    public:
        const std::string value;

        explicit ForArrayStatementNode(const std::string &value, BlockNode *const parent, size_t pos) : ForStatementNode(parent, pos), value(value) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    // for key, value in object
    class ForObjectStatementNode : public ForStatementNode
    {
    public:
        const std::string key;
        const std::string value;

        explicit ForObjectStatementNode(const std::string &key, const std::string &value, BlockNode *const parent, size_t pos)
            : ForStatementNode(parent, pos), key(key), value(value) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class FileStatementNode : public StatementNode
    {
    public:
        ExpressionWrapperNode filename;
        BlockNode body;
        BlockNode *const parent;

        explicit FileStatementNode(BlockNode* const parent, size_t pos) : StatementNode(pos), parent(parent) {}
        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class ApplyTemplateStatementNode : public StatementNode
    {
    public:
        const std::string template_name;
        const std::string field_name;
        const std::string field_path;

        explicit ApplyTemplateStatementNode(const std::string& name, const std::string& field, size_t pos) 
                                        : StatementNode(pos), template_name(name), field_name(field),
                                            field_path(convert_dot_to_ptr(field)) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

    class SetStatementNode : public StatementNode
    {
    public:
        const std::string key;
        ExpressionWrapperNode expression;

        explicit SetStatementNode(const std::string &key, size_t pos) : StatementNode(pos), key(key) {}

        void accept(NodeVisitor &v) const
        {
            v.visit(*this);
        }
    };

} // namespace Wizard