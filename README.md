# Antigo

Welcome to Antigo! This is a simple and user-friendly C++ library for debugging, inspired by the context paradigm from Go.

🚧 **Heads up:** Antigo is still in development and might receive a lot of breaking changes. Check out the roadmap below if you’re curious about what’s coming up!

## What’s Cool About Antigo

Ever found yourself in a situation where your users face a critical issue, but your logs lack the necessary insight, and you end up spending days trying to figure out what’s going on? And that's assuming you're dealing with backend - but what if your code runs as a desktop app? Well, there’s a solution now, and it works in both environments!

Antigo makes use of C++ features to automatically link to the previous context and detect in-flight exceptions, giving you some handy functionality:

1. **Automatic Context Passing:** No need to pass context through every function! Just declare it at the start of your function, and it’ll be passed along automatically through thread-local storage. This is super helpful when dealing with stack frames that you don’t control (like other C++ libraries or even different programming languages).

2. **Minimalist Yet Powerful Interface:** Antigo was designed to have little impact on performance. Log strings, numbers, or even lambdas! Everything is kept in leightweight form by default and only evaluated when needed.

3. **Stacktrace-Like Format:** Call `ctx.Resolve()` whenever! It resolves logged values, and presents them in a nice, readable (as much as possible) stacktrace-like format. This also happens automatically if an exception occurs.

4. **Automatic Exception Detection:** Antigo detects exceptions for you! You’ll know exactly where an exception comes from and also get all the messages logged from lower frames. This mechanism doesn't interfere with normal exception flow, you just get more information using the so-called 'exception witness' mechanism.

5. **Key-Value Storage:** _(Coming soon)_ Store values related to the current execution scope to easily log important info or even pass callbacks.

For more examples of how to use Antigo, check out this README and [unit tests](https://github.com/nic11/antigo/blob/master/unit/src/ContextTest.cpp).

## How To Use

```cpp
void MyFunction(int a, int b) {
  ANTIGO_CONTEXT_INIT(ctx);
  ctx.AddSigned(a);
  ctx.AddSigned(b);

  if (a == 0 || b == 0) {
    ctx.AddMessage("one of params is zero");
    DoSomethingSpecial(a, b);
    return;
  }

  ctx.AddMessage("normal case");
  // ...
}

void Magic() {
  // ...
  try {
    MyFunction(a, b);
  } catch (const std::exception& ex) {
    std::cerr << "Caught exception: " << ex.what() << "\n";
    while (antigo::HasExceptionWitness()) {
      auto w = antigo::PopExceptionWitness();
      std::cerr << "resolved witness: " << w.ToString() << "\n";
    }
  }
}
```

Now, if an exception occurs inside `MyFunction`, you would see it in the output. This would work even if `Magic` doesn't call `MyFunction` directly.

More detailed documentation is coming soon. In the meantime you can check [unit tests](https://github.com/nic11/antigo/blob/master/unit/src/ContextTest.cpp).
