# A Distributed State Management Library

Part 1: Simple pub/sub

- `create(), register(), get(), set()`
- Single writer, many readers
- Static config file defining all shared variables

Future Extensions:
- Distributed locking/signaling
- Multiple writers (still single source of truth, but "readers" can make a request to change a variable).
- Dynamic variable definition? (I don't like)

Design decisions:
