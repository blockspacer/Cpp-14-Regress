
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

#include "strongly_typed_enum.h"
#include "utils.h"
#include <vector>

namespace cpp14regress {

    using namespace std;
    using namespace clang;
    using namespace llvm;

    bool ForwardDeclaredEnumReplacer::VisitEnumDecl(clang::EnumDecl *enumDecl) {
        if (!fromUserFile(enumDecl, f_sourceManager))
            return true;
        if (auto *enumDef = enumDecl->getDefinition()) {
            if (enumDef != enumDecl) {
                SourceRange removeRange = enumDecl->getSourceRange();
                removeRange.setEnd(findTokenEndAfterLoc(removeRange.getEnd(),
                                                        tok::TokenKind::semi, f_astContext));
                if (removeRange.isInvalid()) {
                    f_rewriter->InsertTextBefore(enumDecl->getLocStart(), Comment::line(
                            replacement::info(type(), replacement::result::found)) + "\n");
                    return true;
                }
                if (find(f_replaced.begin(), f_replaced.end(), enumDef) == f_replaced.end()) {
                    f_replaced.push_back(enumDef);
                    SourceRange insertRange = enumDef->getSourceRange();
                    insertRange.setEnd(findTokenEndAfterLoc(insertRange.getEnd(),
                                                            tok::TokenKind::semi, f_astContext));
                    if (removeRange.isInvalid()) {
                        f_rewriter->InsertTextBefore(enumDef->getLocStart(), Comment::line(
                                replacement::info(type(), replacement::result::found)) + "\n");
                        return true;
                    }
                    f_rewriter->ReplaceText(removeRange, insertRange);
                    f_rewriter->InsertTextBefore(removeRange.getBegin(), Comment::line(
                            replacement::info(type(), replacement::result::replaced)) + "\n");
                    f_rewriter->InsertTextBefore(enumDef->getLocStart(), Comment::block::begin());
                    f_rewriter->InsertTextAfterToken(enumDef->getLocEnd(), Comment::block::end());
                } else {
                    f_rewriter->InsertTextBefore(removeRange.getBegin(), Comment::block::begin());
                    f_rewriter->InsertTextAfterToken(removeRange.getEnd(), Comment::block::end());
                }
            }
        }
        return true;
    }

    bool ImprovedEnumReplacer::VisitEnumDecl(clang::EnumDecl *enumDecl) {
        if (!fromUserFile(enumDecl, f_sourceManager))
            return true;
        if (auto *enumDef = enumDecl->getDefinition()) {

            SourceRange typeRange = enumDef->getIntegerTypeRange();
            SourceLocation nameEnd = Lexer::getLocForEndOfToken(enumDef->getLocation(), 0,
                                                                *f_sourceManager, *f_langOptions);
            if (nameEnd.isInvalid())
                return true;

            if (typeRange.isValid()) {
                typeRange.setBegin(findTokenBeginAfterLoc(nameEnd, tok::TokenKind::colon,
                                                          1, f_astContext));
                if (typeRange.isInvalid()) {
                    f_rewriter->InsertTextAfterToken(typeRange.getEnd(), Comment::block(
                            replacement::info(type(), replacement::result::found)));
                } else {
                    f_rewriter->InsertTextAfterToken(typeRange.getEnd(), Comment::block::end());
                    string before = Comment::block::begin() +
                                    replacement::info(type(), replacement::result::removed) + " ";
                    f_rewriter->InsertTextBefore(typeRange.getBegin(), before);
                }
            }

            if (enumDef->isScopedUsingClassTag()) {
                f_rewriter->ReplaceText(SourceRange(enumDef->getLocStart(), nameEnd),
                                        string("struct " + enumDef->getNameAsString() + " {\n"));
                f_rewriter->InsertTextAfterToken(nameEnd, string("enum " + nameForReplace()));
                f_rewriter->InsertTextBefore(enumDef->getLocStart(), Comment::line(
                        replacement::begin(type(), replacement::result::replaced)) + "\n");
                f_rewriter->InsertTextBefore(enumDef->getLocEnd(), Comment::line(
                        replacement::end(type(), replacement::result::replaced)) + "\n");
                f_rewriter->InsertTextBefore(enumDef->getLocEnd(), "};");
            }
        }
        return true;
    }

    bool ImprovedEnumReplacer::VisitEnumTypeLoc(clang::EnumTypeLoc typeLoc) {
        if (!fromUserFile(&typeLoc, f_sourceManager))
            return true;
        //cout << "Enum type loc: " << typeLoc.getLocStart().printToString(*f_sourceManager) << " -- "
        //     << toString(typeLoc.getSourceRange(), f_astContext) << endl;

        if (auto *enumDecl = typeLoc.getDecl()) {
            if (!fromUserFile(enumDecl, f_sourceManager))
                return true;
            //if (auto *enumDef = enumDecl->getDefinition())  //TODO is necessary?
            if (enumDecl->isScopedUsingClassTag()) {
                replacement::result res = replacement::result::inserted;
                SourceLocation insertLoc = Lexer::getLocForEndOfToken(
                        typeLoc.getLocStart(), 0, *f_sourceManager, *f_langOptions);
                Token token;
                do {
                    if (Lexer::getRawToken(insertLoc, token, *f_sourceManager, *f_langOptions, true)) {
                        res = replacement::result::found;
                        break;
                    }
                    insertLoc = token.getEndLoc();
                } while (token.is(tok::TokenKind::comment));
                if (res == replacement::result::inserted) {
                    if (token.isNot(tok::TokenKind::coloncolon)) {
                        f_rewriter->InsertTextAfterToken(typeLoc.getLocEnd(),
                                                         string("::" + nameForReplace()));
                    } else {
                        return true;
                    }
                }
                f_rewriter->InsertTextBefore(typeLoc.getLocStart(),
                                             Comment::block(replacement::info(type(), res)));
            }
        }
        return true;
    }
}