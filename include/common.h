#ifndef COMMON_H_
#define COMMON_H_

#define SWAP(T, a, b) \
    do {              \
        T t = a;      \
        a = b;        \
        b = t         \
    } while (0)

#define UNIMPLEMENTED(...)                                                      \
    do {                                                                        \
        printf("%s:%d: UNIMPLEMENTED: %s \n", __FILE__, __LINE__, __VA_ARGS__); \
        exit(1);                                                                \
    } while (0)

#define UNUSED(x) (void)(x+1)

typedef int Errno;

#define return_defer(value) \
    do {                    \
        result = (value);   \
        goto defer;         \
    } while (0)

#endif
