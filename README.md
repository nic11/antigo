# Antigo

Welcome to Antigo! This is a simple and user-friendly C++ library for debugging, inspired by the context paradigm from Go. The idea behind Antigo is to combine strenghs of stacktraces and core dumps.

🚧 **Heads up:** Antigo is still in development and might receive a lot of breaking changes. Check out the roadmap below if you’re curious about what’s coming up!

## What’s Cool About Antigo

Let me jump straight to the action:

```cpp
void MyCoolFunc(...)
{
  ANTIGO_CONTEXT_INIT(ctx);  // <<< 1. This is all you need to have all features available in your cool function!

  // vvv 2. Minimalist Yet Powerful Interface
  ctx.AddSigned(123456); // any numbers
  ctx.AddMessage("any compile-time string to help you orient");
  ctx.AddLambdaWithOwned([some_vector] {
    return std::to_string(some_vector.size()) + " elements in the vector";
    // or basically ANYTHING else! It won't be called unless needed
  });

  if (/* something worth debugging */) {
    your_logger::error("Whoops! Take a look: {}", ctx.Resolve().ToString());
    // ^^^ 3. Get everything logged above in this log... or do this:
    this_guy_has_broke_the_state = ctx.Resolve();
    // and log that later
  }

  SomeOtherFunctionThatThrows(...);
  // ^^^ 4. Will detect exception from here! You'll get all the messages just as with the manual ctx.Resolve() call

  // Nothing happened? Nothing evaluated!
}

// ...
MyCoolFunc(...);
```

## Why was it created?

Ever found yourself in a situation where your users face a critical issue, but your logs lack the necessary insight, and you end up spending days trying to figure out what’s going on? And that's assuming you're dealing with backend - but what if your code runs as a desktop app? Well, there’s a solution now, and it works in both environments!

## What's under the hood?

1. Context is passed automatically by storing a 'parallel' stack in the thread-local storage. It has debug information like function names, source locations, as well as the informataion you logged.
2. The logging mechanism is designed to be as lightweight as possible. And it **is** lightweight, leading to a negligible impact on performance (but it still has space for improval).
    * When you log a string, a `char*` pointer is saved. This is possible because the standard guarantees that these addresses will be valid throughout the execution. And this is also why the strings you logged must be static.
    * To log more complex values, you can write custom lambdas that return `std::string`. The idea behind this is to give you a robust way to provide any custom logic you might want, while not executing it right away. Antigo lets you specify lambda's lifetime to ensure that references don't expire by the time of evaluation.
3. `ctx.Resolve()` call walks through the 'parallel' stack, copies data, and calls your custom lambdas. This gives you a `ResolvedContext` instance that you can then call `.ToString()` on, not necessarily immediately.
4. `Context` detects exceptions by calling [`std::uncaught_exceptions()`](https://en.cppreference.com/w/cpp/error/exception/uncaught_exception.html) in its constructor and destructor and comparing these values. If these values differ, that means that the current scope had an exception that flew outside its boundaries. Antigo provides functions to collect `ResolvedContext` as soon as an exception is detected. I called them 'exception witnesses'.

More detailed documentation is coming soon. In the meantime you can check [unit tests](https://github.com/nic11/antigo/blob/master/unit/src/ContextTest.cpp).
