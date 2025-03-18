#pragma once
#include "Desc.h"
#include "Node.h"
#include "Config.h"

namespace Wizard
{
    class DescriptionVisitor : public NodeVisitor
    {
    public:
        Description description;

        const RenderConfig& config;

        DescriptionVisitor(const RenderConfig& config) : config(config) {}

        void populate(const Template& tpl) {
            clear();
            visit(tpl.root);
        }

        void clear() {
            description.clear();
        }
    protected:

        bool check_variable_name(const std::string& name) {
            // already added
            const auto itvar = description.variables.find(name);
            if(itvar != description.variables.end()) {
                return false;
            }
            // ignore loop variables
            auto full_loop_name = config.loop_variable_name + ".";
            if(name.starts_with(full_loop_name)) {
                return false;
            }
            return  true;
        }

        void visit(const BlockNode& node) {
            for (auto &n : node.nodes) {
                n->accept(*this);
            }
        }

        void visit(const LiteralNode &) {
        }
        void visit(const TextNode &){
        }

        void visit(const CommentNode &){
        }
        void visit(const ExpressionNode &) {
        }

        void visit(const DataNode& node){
            if(check_variable_name(node.name)) {
                description.variables[node.name] = {node.name};
            }
        }
        
        void visit(const FunctionNode& node) {
            for (auto &n : node.arguments) {
                n->accept(*this);
            }
        }

        void visit(const ExpressionWrapperNode& node) {
            node.root->accept(*this);
        }

        void visit(const StatementNode &) {
        }
        void visit(const ForStatementNode &) {
        }

        void visit(const ForArrayStatementNode& node){
            node.condition.accept(*this);
            node.body.accept(*this);
        }

        void visit(const ForObjectStatementNode& node)
        {
            node.condition.accept(*this);
            node.body.accept(*this);
        }

        void visit(const IfStatementNode& node)
        {
            node.condition.accept(*this);
            node.true_statement.accept(*this);
            node.false_statement.accept(*this);
        }

        void visit(const FileStatementNode& node) {
            node.filename.accept(*this);
            node.body.accept(*this);
        }

        void visit(const ApplyTemplateStatementNode& node) {
            if(check_variable_name(node.field_name)) {
                description.variables[node.field_name] = {node.field_name};
            }
            description.nested.insert(node.template_name);
        }

        void visit(const SetStatementNode& node) {
            node.expression.accept(*this);
        }

    };
};