
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
#include "llvm/ADT/DenseMap.h"

#include "user_literals.h"
#include "utils.h"

namespace cpp14regress {

    using namespace std;
    using namespace clang;
    using namespace llvm;

    //TODO coincidence with the system literal
    bool UserLiteralReplacer::VisitUserDefinedLiteral(UserDefinedLiteral *literal) {
        if (!fromUserFile(literal, f_sourceManager))
            return true;
        if (FunctionDecl *funcDecl = literal->getDirectCallee()) {
            if (!fromUserFile(funcDecl, f_sourceManager))
                return true;

            SourceRange paramsRange(literal->getLocStart(), literal->getUDSuffixLoc());
            SourceRange suffixRange(literal->getUDSuffixLoc(), literal->getLocEnd());
            string params = toString(paramsRange, f_astContext, false);
            replacement::result res = replacement::result::replaced;
            switch (literal->getLiteralOperatorKind()) {
                case UserDefinedLiteral::LiteralOperatorKind::LOK_String : {
                    params += ", ";
                    StringLiteralMeter meter;
                    meter.TraverseStmt(literal->getArg(0));
                    if (!meter.found()) { //TODO
                        res = replacement::result::found;
                        break;
                    }
                    params += to_string(meter.size());
                    break;
                }
                case UserDefinedLiteral::LiteralOperatorKind::LOK_Raw : {
                    params = string("\"" + params + "\"");
                    break;
                }
                case UserDefinedLiteral::LiteralOperatorKind::LOK_Template : {
                    res = replacement::result::found;
                    break;
                }
                default: {
                    break;
                }
            }
            f_rewriter->InsertTextAfterToken(literal->getLocEnd(), Comment::block(
                    replacement::info(type(), res)));
            if (res == replacement::result::replaced) {
                string call = operatorFuncName(toString(suffixRange, f_astContext));
                call += string("(" + params + ")");
                f_rewriter->ReplaceText(literal->getSourceRange(), call);
            }
        }
        return true;
    }

    bool UserLiteralReplacer::VisitFunctionDecl(clang::FunctionDecl *funcDecl) {
        if (!fromUserFile(funcDecl, f_sourceManager))
            return true;
        if (auto ident = funcDecl->getLiteralIdentifier()) { //TODO check
            SourceRange opRange;
            opRange.setBegin(funcDecl->getLocation());
            opRange.setEnd(findTokenBeginAfterLoc(opRange.getBegin(),
                                                  tok::TokenKind::l_paren, 2, f_astContext));
            replacement::result res = replacement::result::found;
            if (opRange.isValid()) {
                string funName = operatorFuncName(ident->getName());
                f_rewriter->ReplaceText(opRange, funName);
                res = replacement::result::replaced;
            }
            f_rewriter->InsertTextBefore(funcDecl->getLocStart(), Comment::line(
                    replacement::info(type(), res)) + "\n");
        }
        return true;
    }
}