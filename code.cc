// `pls run` does the job.

#include <iostream>
#include <memory>
#include <functional>

struct KeepsLambdaByCopy final {
  std::function<void()> const f_;
  KeepsLambdaByCopy(std::function<void()> const f) : f_(f) {
  }
  void Call() const {
    f_();
  }
};

struct KeepsLambdaByMove final {
  std::function<void()> const f_;
  KeepsLambdaByMove(std::function<void()> f) : f_(std::move(f)) {
  }
  void Call() const {
    f_();
  }
};

template <class F>
struct KeepsGenericFunctionByMove final {
  F const f_;
  KeepsGenericFunctionByMove(F&& f) : f_(std::move(f)) {
  }
  void Call() const {
    f_();
  }
};

template <class F>
KeepsGenericFunctionByMove<F> Wrap(F&& f) {
  return KeepsGenericFunctionByMove<F>(std::move(f));
}

// Production-grade definition.

struct ProductionGradeCallable {
  virtual ~ProductionGradeCallable() = default;
  virtual void DoCall() = 0;
};

template <class F>
struct ProductionGradeCallableImpl final : ProductionGradeCallable {
  F const f_;
  ProductionGradeCallableImpl(F&& f) : f_(std::move(f)) {
  }
  virtual void DoCall() {
    f_();
  }
};

struct ProductionGradeFunction final {
  std::unique_ptr<ProductionGradeCallable> const f_;
  ProductionGradeFunction(std::unique_ptr<ProductionGradeCallable> f) : f_(std::move(f)) {
  }
  void Call() const {
    f_->DoCall();
  }
};

template <class F>
ProductionGradeFunction ProductionGradeWrap(F&& f) {
  return ProductionGradeFunction(std::make_unique<ProductionGradeCallableImpl<F>>(std::move(f)));
}

int main() {
  {
    KeepsLambdaByCopy t1([]() { std::cout << "A" << std::endl; });
    KeepsLambdaByCopy t2([]() { std::cout << "B" << std::endl; });
    t1.Call();
    t2.Call();
  }
  {
    std::shared_ptr<int> s1 = std::make_shared<int>(1);
    KeepsLambdaByCopy t1([s = s1]() { std::cout << "C=" << *s << std::endl; });
    KeepsLambdaByCopy t2([s = s1]() { std::cout << "D=" << *s << std::endl; });
    t1.Call();
    t2.Call();
  }
  {
    std::shared_ptr<int> s2 = std::make_shared<int>(2);
    KeepsLambdaByCopy t1([q = s2]() { std::cout << "E=" << *q << std::endl; });
    KeepsLambdaByCopy t2([q = s2]() { std::cout << "F=" << *q << std::endl; });
    t1.Call();
    t2.Call();
  }
  {
    std::unique_ptr<int> u = std::make_unique<int>(3);

    // std::function<void()> f = [w=std::move(u)]{ std::cout << "G=" << *w << std::endl; };
    auto f = [w = std::move(u)] { std::cout << "G=" << *w << std::endl; };
    f();

    // THIS DOES NOT WORK! Because the lambda "owns" an object that can not be copied, only moved.
    // KeepsLambdaByCopy t1([w=std::move(u)]{ std::cout << "G=" << *w << std::endl; });
    // t1.Call();

    // THIS DOES NOT WORK! Because `std::function<>` is not friendly with moving what it has captured.
    // My understanting is that `std::function<>`-s are meant to be copyable, at least as of C++17.
    // KeepsLambdaByMove t2(std::move([w=std::move(u)]{ std::cout << "G=" << *w << std::endl; }));
    // t2.Call();

    // This works.
    auto t3 = Wrap(std::move(f));
    t3.Call();
  }
  {
    std::unique_ptr<int> u2 = std::make_unique<int>(4);
    // And this is "production-grade" works.
    ProductionGradeFunction t4 = ProductionGradeWrap([w2 = std::move(u2)]() { std::cout << "H=" << *w2 << std::endl; });
    t4.Call();
  }
}
