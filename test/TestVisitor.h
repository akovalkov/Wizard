#pragma once
#include "../library/Node.h"
#include "../library/Template.h"

using namespace Wizard;

class TestVisitor : public NodeVisitor
{
public:
    struct NodeInfo
    {
        std::string name;
        int indent{0};
        // NodeInfo() = default;
        // NodeInfo(const std::string& name, int indent) : name(name), indent(indent) {}
        constexpr bool operator ==(const NodeInfo& other) const {
            return name == other.name && indent == other.indent;
        }
    };
    
    int indent{0};
    std::vector<NodeInfo> nodes;

    explicit TestVisitor() {}
    
    void process(Template& tpl)
    {
        visit(tpl.root);
    }
protected:
    void visit(const BlockNode& node) {
        nodes.emplace_back("Block", indent);
        ++indent;
        for (auto &n : node.nodes) {
            n->accept(*this);
        }
        --indent;
    }

    void visit(const LiteralNode &) {
        nodes.emplace_back("Literal", indent);
    }
    void visit(const CommentNode &){
        nodes.emplace_back("Comment", indent);
    }
    void visit(const TextNode &){
        nodes.emplace_back("Text", indent);
    }
    void visit(const ExpressionNode &) {
        nodes.emplace_back("Expression", indent);
    }
    void visit(const DataNode &){
        nodes.emplace_back("Data", indent);
    }
    void visit(const FunctionNode& node) {
        nodes.emplace_back("Function", indent);
        ++indent;
        for (auto &n : node.arguments) {
            n->accept(*this);
        }
        --indent;
    }

    void visit(const ExpressionWrapperNode& node) {
        nodes.emplace_back("ExpressionWrapper", indent);
        ++indent;
        node.root->accept(*this);
        --indent;
    }

    void visit(const StatementNode &) {
        nodes.emplace_back("Statement", indent);
    }
    void visit(const ForStatementNode &) {
        nodes.emplace_back("ForStatement", indent);
    }

    void visit(const ForArrayStatementNode& node){
        nodes.emplace_back("ForArrayStatement", indent);
        ++indent;
        node.condition.accept(*this);
        node.body.accept(*this);
        --indent;
    }

    void visit(const ForObjectStatementNode& node)
    {
        nodes.emplace_back("ForObjectStatement", indent);
        ++indent;
        node.condition.accept(*this);
        node.body.accept(*this);
        --indent;
    }

    void visit(const IfStatementNode& node)
    {
        nodes.emplace_back("IfStatement", indent);
        ++indent;
        node.condition.accept(*this);
        node.true_statement.accept(*this);
        node.false_statement.accept(*this);
        --indent;
    }

    void visit(const FileStatementNode& node) {
        nodes.emplace_back("FileStatement", indent);
        ++indent;
        node.filename.accept(*this);
        node.body.accept(*this);
        --indent;
    }

    void visit(const ApplyTemplateStatementNode &) {
        nodes.emplace_back("ApplyTemplateStatement", indent);
    }

    void visit(const SetStatementNode& node) {
        nodes.emplace_back("SetStatement", indent);
        ++indent;
        node.expression.accept(*this);
        --indent;
    }

};
