#ifndef CPP14REGRESS_SCANNER_H
#define CPP14REGRESS_SCANNER_H

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/ParentMap.h"

#include "base_types.h"
#include "utils.h"
#include <vector>

namespace cpp14regress {

    class Cpp14scanner : public clang::RecursiveASTVisitor<Cpp14scanner> {
    private:
        clang::ASTContext *f_context;
        cpp14features_stat *f_stat;

    public:

        Cpp14scanner(clang::ASTContext *context, cpp14features_stat *stat)
                : f_context(context), f_stat(stat) {}

        virtual void EndFileAction() {};

        //range_based_for
        virtual bool VisitCXXForRangeStmt(clang::CXXForRangeStmt *forLoop);

        //lambda_function
        virtual bool VisitLambdaExpr(clang::LambdaExpr *lambdaExpr);

        //auto_keyword
        // decltype_keyword
        virtual bool VisitDeclaratorDecl(clang::DeclaratorDecl *valueDecl);

        //constexpr keyword //TODO by FunctionDecl::isConstexpr()
        virtual bool VisitFunctionDecl(clang::FunctionDecl *functionDecl);

        //default_specifier //TODO by CXXMethodDecl::isExplicitlyDefaulted()
        //delete_specifier //TODO by CXXMethodDecl::isDeleted()
        virtual bool VisitCXXMethodDecl(clang::CXXMethodDecl *methodDecl);

        //explicit_specifier //TODO by CXXConversionDecl::isExplicitSpecified()
        virtual bool VisitCXXConversionDecl(clang::CXXConversionDecl *conversionMethod);

        //constuctor_delegation //TODO by CXXConstructorDecl::isDelegatingConstructor
        //explicit_specifier //TODO by CXXConstructorDecl::isExplicitSpecified()
        virtual bool VisitCXXConstructorDecl(clang::CXXConstructorDecl *constructorDecl);

        //null_pointer_constant TODO by isNullPointerConstant()
        virtual bool VisitExpr(clang::Expr *expr);

        //noexcept_keyword
        virtual bool VisitCXXNoexceptExpr(clang::CXXNoexceptExpr *noexceptExpr);

        //enum_class //TODO by EnumDecl::isScoped() and EnumDecl::isScopedUsingClassTag()
        virtual bool VisitEnumDecl(clang::EnumDecl *enumDecl);

        //unrestricted_unions //TODO
        virtual bool VisitCXXRecordDecl(clang::CXXRecordDecl *recordDecl);

        //unicode_string_literals //TODO
        virtual bool VisitStringLiteral(clang::StringLiteral *literal);

        //digit_separators //TODO
        virtual bool VisitIntegerLiteral(clang::IntegerLiteral *literal);

        //digit_separators //TODO
        virtual bool VisitFloatingLiteral(clang::FloatingLiteral *literal);

        //alias_template //TODO check if TypeAliasTemplateDecl inherits from TypeAliasDecl
        virtual bool VisitTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl *aliasTemplateDecl);

        //alias_template //TODO
        //alias_typedef //TODO
        virtual bool VisitTypeAliasDecl(clang::TypeAliasDecl* aliasTypeDecl);

        virtual bool VisitSizeOfPackExpr(clang::SizeOfPackExpr* sizeofPackExpr);

        virtual bool VisitUnaryExprOrTypeTraitExpr(clang::UnaryExprOrTypeTraitExpr* sizeofOrAlignof);
    };

}

#endif /*CPP14REGRESS_SCANNER_H*/