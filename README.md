# Antigo

Welcome to Antigo! This is a simple and user-friendly C++ library for debugging, inspired by the context paradigm from Go. The idea behind Antigo is to combine strengths of stacktraces and core dumps.

🚧 **Heads up:** Antigo is still in development, so the exact API is not set in stone just yet. Feel free to suggest new ideas and follow the updates.

## Highlights

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

## Goal of Antigo

**Problem:** you have a bug in production, but there isn't enough information to understand its cause.

Some examples:
* You are greeted with a proud error message that says just `map::at` or `bad_variant_access`
* You look at a stacktrace but have no idea where that nullptr came from
* Your stacktrace ends with a thread's main, but you don't know what exactly started that thread
* You have a core dump, but it's not enough. Something just corrupted the state, but you don't know what
* Your application crashed on Windows, but you run Linux. Or vice-versa. You don't want to run VMs
* You want something like a core dump, but not as heavy. You also want to get it at any time without crashing the program

**Solution:** Antigo was created to ease this pain. You just run `ctx.Resolve()` and get a stacktrace-like dump with the values that you logged. The caveat? You need to log those values. The benefit? You can log them however you want! You won't have to write gdb scripts or parse cryptic values and remember what that private field was. Plus, Antigo is platform independent, as it only depends on core C++ features!

## Under the Hood

1. Context is passed automatically by storing a 'parallel' stack in the thread-local storage. It has debug information like function names, source locations, as well as the informataion you logged.
2. The logging mechanism is designed to be as lightweight as possible. And it **is** lightweight, leading to a negligible impact on performance (but it still has space for improval).
    * When you log a string, a `char*` pointer is saved. This is possible because the standard guarantees that these addresses will be valid throughout the execution. And this is also why the strings you log must be static.
    * To log more complex values, you can write custom lambdas that return `std::string`. The idea behind this is to give you a robust way to provide any custom logic you might want, while not executing it right away. Antigo lets you specify lambda's lifetime to ensure that references don't expire by the time of evaluation.
3. `ctx.Resolve()` call walks through the 'parallel' stack, copies data, and calls your custom lambdas. This gives you a `ResolvedContext` instance that you can then call `.ToString()` on, not necessarily immediately.
4. `Context` detects exceptions by calling [`std::uncaught_exceptions()`](https://en.cppreference.com/w/cpp/error/exception/uncaught_exception.html) in its constructor and destructor and comparing these values. If they differ, that means that the current scope had an exception that flew outside its boundaries. Antigo provides functions to collect `ResolvedContext` as soon as an exception is detected. I called them 'exception witnesses'. This allows you to get additional info about the exception you just caught without affecting the way you (or third-party code) catch them.

More detailed documentation is coming soon. In the meantime you can check [unit tests](https://github.com/nic11/antigo/blob/master/unit/src/ContextTest.cpp).
