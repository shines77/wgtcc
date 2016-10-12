#ifndef _WGTCC_EVALUATOR_H_
#define _WGTCC_EVALUATOR_H_

#include "ast.h"
#include "error.h"
#include "visitor.h"

#include "code_gen.h"
#include "token.h"


class Expr;

template<typename T>
class Evaluator: public Visitor
{
public:
  Evaluator() {}

  virtual ~Evaluator() {}

  virtual void VisitBinaryOp(BinaryOp* binary);
  virtual void VisitUnaryOp(UnaryOp* unary);
  virtual void VisitConditionalOp(ConditionalOp* cond);
  
  virtual void VisitFuncCall(FuncCall* funcCall) {
    Error(funcCall, "expect constant expression");
  }

  virtual void VisitEnumerator(Enumerator* enumer) {
    val_ = static_cast<T>(enumer->Val());
  }

  virtual void VisitIdentifier(Identifier* ident) {
    Error(ident, "expect constant expression");
  }

  virtual void VisitObject(Object* obj) {
    Error(obj, "expect constant expression");
  }

  virtual void VisitConstant(Constant* cons) {
    if (cons->Type()->IsFloat()) {
      val_ = static_cast<T>(cons->FVal());
    } else if (cons->Type()->IsInteger()) {
      val_ = static_cast<T>(cons->IVal());
    } else {
      assert(false);
    }
  }

  virtual void VisitTempVar(TempVar* tempVar) {
    assert(false);
  }

  // We may should assert here
  virtual void VisitDeclaration(Declaration* init) {}
  virtual void VisitIfStmt(IfStmt* ifStmt) {}
  virtual void VisitJumpStmt(JumpStmt* jumpStmt) {}
  virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
  virtual void VisitLabelStmt(LabelStmt* labelStmt) {}
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
  virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
  virtual void VisitFuncDef(FuncDef* funcDef) {}
  virtual void VisitTranslationUnit(TranslationUnit* unit) {}

  T Eval(Expr* expr) {
    expr->Accept(this);
    return val_;
  }

private:
  T val_;
};

template<typename T>
void Evaluator<T>::VisitBinaryOp(BinaryOp* binary)
{
#define L   Evaluator<T>().Eval(binary->lhs_)
#define R   Evaluator<T>().Eval(binary->rhs_)
#define LL  Evaluator<long>().Eval(binary->lhs_)
#define LR  Evaluator<long>().Eval(binary->rhs_)

  if (binary->Type()->ToPointer()) {
    auto val = Evaluator<Addr>().Eval(binary);
    if (val.label_.size()) {
      Error(binary, "expect constant integer expression");
    }
    val_ = static_cast<T>(val.offset_);
    return;
  }

  switch (binary->op_) {
  case '+': val_ = L + R; break; 
  case '-': val_ = L - R; break;
  case '*': val_ = L * R; break;
  case '/': {
    auto l = L, r = R;
    if (r == 0)
      Error(binary, "division by zero");
    val_ = l / r;
  } break;
  case '%': {
    auto l = LL, r = LR;
    if (r == 0)
      Error(binary, "division by zero");
    val_ = l % r;
  } break;
  // Bitwise operators that do not accept float
  case '|': val_ = LL | LR; break;
  case '&': val_ = LL & LR; break;
  case '^': val_ = LL ^ LR; break;
  case Token::LEFT: val_ = LL << LR; break;
  case Token::RIGHT: val_ = LL >> LR; break;

  case '<': val_ = L < R; break;
  case '>': val_ = L > R; break;
  case Token::LOGICAL_AND: val_ = L && R; break;
  case Token::LOGICAL_OR: val_ = L || R; break;
  case Token::EQ: val_ = L == R; break;
  case Token::NE: val_ = L != R; break;
  case Token::LE: val_ = L <= R; break;
  case Token::GE: val_ = L >= R; break;
  case '=': case ',': val_ = R; break;
  case '.': {
    auto addr = Evaluator<Addr>().Eval(binary);
    if (addr.label_.size())
      Error(binary, "expect constant expression");
    val_ = addr.offset_;
  } 
  default: assert(false);
  }

#undef L
#undef R
#undef LL
#undef LR
}


template<typename T>
void Evaluator<T>::VisitUnaryOp(UnaryOp* unary)
{
#define VAL     Evaluator<T>().Eval(unary->operand_)
#define LVAL    Evaluator<long>().Eval(unary->operand_)

  switch (unary->op_) {
  case Token::PLUS: val_ = VAL; break;
  case Token::MINUS: val_ = -VAL; break;
  case '~': val_ = ~LVAL; break;
  case '!': val_ = !VAL; break;
  case Token::CAST:
    if (unary->Type()->IsInteger())
      val_ = static_cast<long>(VAL);
    else
      val_ = VAL;
    break;
  case Token::ADDR: {
    auto addr = Evaluator<Addr>().Eval(unary->operand_);
    if (addr.label_.size())
      Error(unary, "expect constant expression");
    val_ = addr.offset_;
  } break;
  default: Error(unary, "expect constant expression");
  }

#undef LVAL
#undef VAL
}


template<typename T>
void Evaluator<T>::VisitConditionalOp(ConditionalOp* condOp)
{
  bool cond;
  auto condType = condOp->cond_->Type();
  if (condType->IsInteger()) {
    auto val = Evaluator<long>().Eval(condOp->cond_);
    cond = val != 0;
  } else if (condType->IsFloat()) {
    auto val = Evaluator<double>().Eval(condOp->cond_);
    cond  = val != 0.0;
  } else if (condType->ToPointer()) {
    auto val = Evaluator<Addr>().Eval(condOp->cond_);
    cond = val.label_.size() || val.offset_;
  } else {
    assert(false);
  }

  if (cond) {
    val_ = Evaluator<T>().Eval(condOp->exprTrue_);
  } else {
    val_ = Evaluator<T>().Eval(condOp->exprFalse_);
  }
}


struct Addr
{
  std::string label_;
  int offset_;
};

template <>
class Evaluator<Addr>: public Visitor
{
  
public:
  Evaluator() {}
  
  virtual ~Evaluator() {}

  virtual void VisitBinaryOp(BinaryOp* binary);
  virtual void VisitUnaryOp(UnaryOp* unary);
  virtual void VisitConditionalOp(ConditionalOp* cond);
  
  virtual void VisitFuncCall(FuncCall* funcCall) {
    Error(funcCall, "expect constant expression");
  }

  virtual void VisitEnumerator(Enumerator* enumer) {
    addr_.offset_ = enumer->Val();
  }

  virtual void VisitIdentifier(Identifier* ident) {
    addr_.label_ = ident->Name();
    addr_.offset_ = 0;
  }

  virtual void VisitObject(Object* obj) {
    if (!obj->IsStatic()) {
      Error(obj, "expect static object");
    }
    addr_.label_ = obj->Label();
    addr_.offset_ = 0;
  }

  virtual void VisitConstant(Constant* cons);

  virtual void VisitTempVar(TempVar* tempVar) {
    assert(false);
  }

  // We may should assert here
  virtual void VisitDeclaration(Declaration* init) {}
  virtual void VisitIfStmt(IfStmt* ifStmt) {}
  virtual void VisitJumpStmt(JumpStmt* jumpStmt) {}
  virtual void VisitReturnStmt(ReturnStmt* returnStmt) {}
  virtual void VisitLabelStmt(LabelStmt* labelStmt) {}
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt) {}
  virtual void VisitCompoundStmt(CompoundStmt* compStmt) {}
  virtual void VisitFuncDef(FuncDef* funcDef) {}
  virtual void VisitTranslationUnit(TranslationUnit* unit) {}

  Addr Eval(Expr* expr) {
    expr->Accept(this);
    return addr_;
  }

private:
  Addr addr_;
};

#endif
