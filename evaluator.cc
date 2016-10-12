#include "evaluator.h"

void Evaluator<Addr>::VisitBinaryOp(BinaryOp* binary)
{
#define LR   Evaluator<long>().Eval(binary->rhs_)
#define R   Evaluator<Addr>().Eval(binary->rhs_)
  
  auto l = Evaluator<Addr>().Eval(binary->lhs_);
  
  int width = 1;
  auto pointerType = binary->Type()->ToPointer();
  if (pointerType)
    width = pointerType->Derived()->Width();

  switch (binary->op_) {
  case '+':
    assert(pointerType);
    addr_.label_ = l.label_;
    addr_.offset_ = l.offset_ + LR * width;
    break;
  case '-':
    assert(pointerType);
    addr_.label_ = l.label_;
    addr_.offset_ = l.offset_ + LR * width;
    break;
  case '.': {
    addr_.label_ = l.label_;
    auto type = binary->lhs_->Type()->ToStruct();
    auto offset = type->GetMember(binary->rhs_->tok_->str_)->Offset();
    addr_.offset_ = l.offset_ + offset;
    break;
  }
  default: assert(false);
  }
#undef LR
#undef R
}

void Evaluator<Addr>::VisitUnaryOp(UnaryOp* unary)
{
  auto addr = Evaluator<Addr>().Eval(unary->operand_);

  switch (unary->op_) {
  case Token::CAST:
  case Token::ADDR:
  case Token::DEREF:
    addr_ = addr; break;
  default: assert(false);
  }
}

void Evaluator<Addr>::VisitConditionalOp(ConditionalOp* condOp)
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
    addr_ = Evaluator<Addr>().Eval(condOp->exprTrue_);
  } else {
    addr_ = Evaluator<Addr>().Eval(condOp->exprFalse_);
  }
}

void Evaluator<Addr>::VisitConstant(Constant* cons) 
{
  if (cons->Type()->IsInteger()) {
    addr_ = {"", static_cast<int>(cons->IVal())};
  } else if (cons->Type()->ToArray()) {
    Generator().ConsLabel(cons); // Add the literal to rodatas_.
    addr_.label_ = Generator::rodatas_.back().label_;
    addr_.offset_ = 0;
  } else {
    assert(false);
  }
}
