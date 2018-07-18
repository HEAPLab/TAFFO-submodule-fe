
class EPExpr {
protected:
  EPExpr() = default;
};

class EPLeaf : public EPExpr {
public:
  EPLeaf(Value *V)
    : V(V) {}
protected:
  Value *V;
};

class EPBin : public EPExpr {
public:
  
protected:
  std::unique_ptr<EPExpr> Left;
  std::unique_ptr<EPExpr> Right;
};
