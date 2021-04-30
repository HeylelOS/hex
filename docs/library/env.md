# env

Process' environment variables manipulation functions.

### env.clear ()

Clear's the calling process' environment variable.
Note: On the Glibc-based implementation, `clearenv(3)` is called,
on other platforms, `environ` is set to `NULL`, which doesn't necessarily
guarantee this behaviour.

### env.fill (tables...)

For every table in **tables**, `env.set` every key/value pair, overwriting any previous value.

### env.get (name)

Acquires the environment variable value for key **name**.
Returns the associated value if found, `nil` if not. Raises an error on failure.

### env.set (name, value[, nooverwrite])

Sets the environment variable's value identified by **name** to **value**.
If **nooverwrite** is specified, it casts to a boolean indicating whether any previous value is overwritten.
Returns nothing on success, raises an error on failure.

